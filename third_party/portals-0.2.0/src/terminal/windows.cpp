// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/terminal.hpp>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

namespace portals {
namespace {
TerminalError systemError(DWORD code = GetLastError()) {
  return {TerminalErrorKind::SYSTEM, static_cast<int>(code), 0};
}

TerminalError contractError(TerminalErrorKind kind) {
  return {kind, 0, 0};
}

DWORD waitTime(TerminalDeadline deadline, std::chrono::milliseconds cadence) {
  const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());

  return static_cast<DWORD>(std::clamp(remaining.count(), decltype(remaining.count()){0}, cadence.count()));
}

InputTime inputTime() {
  return static_cast<InputTime>(
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

InputModifiers modifiers(DWORD flags) {
  return {(flags & SHIFT_PRESSED) != 0, (flags & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0,
          (flags & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0, false};
}

bool carriesInput(const KEY_EVENT_RECORD &record) {
  // ConPTY and Alt+numpad deliver otherwise unmappable Unicode on Alt release.
  // Other releases echo prior key-down text and must not produce a second event.
  return record.bKeyDown || (record.wVirtualKeyCode == VK_MENU && record.uChar.UnicodeChar != 0);
}

std::optional<InputKey> specialKey(WORD key) {
  switch (key) {
  case VK_UP:
    return InputKey::UP;
  case VK_DOWN:
    return InputKey::DOWN;
  case VK_LEFT:
    return InputKey::LEFT;
  case VK_RIGHT:
    return InputKey::RIGHT;
  case VK_HOME:
    return InputKey::HOME;
  case VK_END:
    return InputKey::END;
  case VK_INSERT:
    return InputKey::INSERT;
  case VK_DELETE:
    return InputKey::DELETE_KEY;
  case VK_PRIOR:
    return InputKey::PAGE_UP;
  case VK_NEXT:
    return InputKey::PAGE_DOWN;
  default:
    if (key >= VK_F1 && key <= VK_F12) {
      return static_cast<InputKey>(static_cast<int>(InputKey::F1) + key - VK_F1);
    }

    return std::nullopt;
  }
}

std::string utf8(char32_t scalar) {
  std::string result;

  if (scalar <= 0x7f) {
    result += static_cast<char>(scalar);
  } else if (scalar <= 0x7ff) {
    result += static_cast<char>(0xc0 | (scalar >> 6));
    result += static_cast<char>(0x80 | (scalar & 0x3f));
  } else if (scalar <= 0xffff) {
    result += static_cast<char>(0xe0 | (scalar >> 12));
    result += static_cast<char>(0x80 | ((scalar >> 6) & 0x3f));
    result += static_cast<char>(0x80 | (scalar & 0x3f));
  } else {
    result += static_cast<char>(0xf0 | (scalar >> 18));
    result += static_cast<char>(0x80 | ((scalar >> 12) & 0x3f));
    result += static_cast<char>(0x80 | ((scalar >> 6) & 0x3f));
    result += static_cast<char>(0x80 | (scalar & 0x3f));
  }

  return result;
}
} // namespace

struct TerminalSession::Impl {
  HANDLE input{INVALID_HANDLE_VALUE};
  HANDLE output{INVALID_HANDLE_VALUE};
  DWORD input_mode{};
  DWORD output_mode{};
  UINT output_page{};

  bool input_changed{};
  bool output_changed{};
  bool page_changed{};
  CONSOLE_CURSOR_INFO saved_cursor{};
  bool cursor_saved{};

  TerminalOptions options;
  InputDecoder decoder;
  CellSize last_size{};
  std::optional<KEY_EVENT_RECORD> repeated;
  WORD repeat_index{};

  wchar_t high_surrogate{};

  DWORD mouse_buttons{};
  std::array<MouseInputEvent, 3> mouse_pending{};
  std::size_t mouse_count{}, mouse_offset{};
  std::string input_bytes;
  std::size_t input_offset{};

  bool input_failed{};

  std::optional<InputModifiers> character_modifiers;
  std::string repeated_scalar;
  InputModifiers repeated_modifiers;

  mutable COORD viewport_origin{};

  std::string output_bytes;
  std::size_t output_offset{};
  HANDLE writer{};
  DWORD written{};
  DWORD write_error{};

  bool uncertain{};

  Impl(TerminalOptions opts, InputDecoder dec) : options(opts), decoder(std::move(dec)) {
  }

  ~Impl() {
    (void)restore();
  }

  // The worker borrows Impl and its immutable batch until its thread handle signals.
  // The owner cannot release bytes, restore modes or destroy Impl before joining it.
  static DWORD WINAPI writeBatch(void *context) noexcept {
    auto &self = *static_cast<Impl *>(context);
    const auto remaining = self.output_bytes.size() - self.output_offset;
    const BOOL success = WriteFile(self.output, self.output_bytes.data() + self.output_offset,
                                   static_cast<DWORD>(remaining), &self.written, nullptr);
    self.write_error = success ? 0 : GetLastError();

    return 0;
  }

  int stopWriter() noexcept {
    if (!writer) {
      return 0;
    }

    // Cancellation may race entry into WriteFile. Keep Impl/bytes alive until actual completion.
    while (WaitForSingleObject(writer, 0) != WAIT_OBJECT_0) {
      CancelSynchronousIo(writer);
      WaitForSingleObject(writer, static_cast<DWORD>(options.wait_cadence.count()));
    }

    const int error = CloseHandle(writer) ? 0 : static_cast<int>(GetLastError());

    writer = nullptr;

    return error;
  }

  int restore() noexcept {
    int first_error = stopWriter();

    const auto remember = [&first_error](BOOL success) {
      if (!success && first_error == 0) {
        first_error = static_cast<int>(GetLastError());
      }
    };

    if (cursor_saved) {
      remember(SetConsoleCursorInfo(output, &saved_cursor));
      cursor_saved = false;
    }

    // The console host may need to resume draining before these calls complete.

    if (page_changed) {
      remember(SetConsoleOutputCP(output_page));
    }

    if (output_changed) {
      remember(SetConsoleMode(output, output_mode));
    }

    if (input_changed) {
      remember(SetConsoleMode(input, input_mode));
    }

    if (output != INVALID_HANDLE_VALUE) {
      remember(CloseHandle(output));
    }

    if (input != INVALID_HANDLE_VALUE) {
      remember(CloseHandle(input));
    }

    page_changed = output_changed = input_changed = false;
    output = input = INVALID_HANDLE_VALUE;

    return first_error;
  }

  std::expected<CellSize, TerminalError> dimensions() const noexcept {
    CONSOLE_SCREEN_BUFFER_INFO info{};

    if (!GetConsoleScreenBufferInfo(output, &info)) {
      return std::unexpected(systemError());
    }

    viewport_origin = {info.srWindow.Left, info.srWindow.Top};

    return CellSize{info.srWindow.Right - info.srWindow.Left + 1, info.srWindow.Bottom - info.srWindow.Top + 1};
  }

  void character(wchar_t value) {
    if (high_surrogate) {
      if (value >= 0xdc00 && value <= 0xdfff) {
        const auto scalar = char32_t{0x10000} + ((high_surrogate - 0xd800) << 10) + value - 0xdc00;

        input_bytes = utf8(scalar);
        high_surrogate = 0;

        return;
      }

      input_bytes = utf8(0xfffd);
      high_surrogate = 0;
    }

    if (value >= 0xd800 && value <= 0xdbff) {
      high_surrogate = value;
    } else {
      input_bytes += utf8(value >= 0xdc00 && value <= 0xdfff ? 0xfffd : value);
    }
  }

  void keyRecord(KEY_EVENT_RECORD record, std::vector<InputEvent> &events) {
    if (!carriesInput(record)) {
      return;
    }

    if (repeat_index > 0 && !repeated_scalar.empty()) {
      input_bytes = repeated_scalar;
      character_modifiers = repeated_modifiers;

      return;
    }

    if (record.uChar.UnicodeChar || ((record.wVirtualKeyCode == VK_SPACE || record.wVirtualKeyCode == 0x32) &&
                                     (record.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))) {
      if (!decoder.deadline()) {
        character_modifiers = modifiers(record.dwControlKeyState);

        if ((record.dwControlKeyState & RIGHT_ALT_PRESSED) && (record.dwControlKeyState & LEFT_CTRL_PRESSED) &&
            record.uChar.UnicodeChar >= 0x20) {
          character_modifiers->alt = character_modifiers->control = false;
        }
      }

      character(record.uChar.UnicodeChar);
      repeated_scalar = input_bytes;
      repeated_modifiers = character_modifiers.value_or(InputModifiers{});
    } else if (const auto key = specialKey(record.wVirtualKeyCode)) {
      events.emplace_back(KeyEvent{*key, 0, modifiers(record.dwControlKeyState),
                                   repeat_index == 0 ? KeyAction::PRESS : KeyAction::REPEAT});
    }
  }

  void mouseRecord(const MOUSE_EVENT_RECORD &record) {
    constexpr DWORD SUPPORTED = FROM_LEFT_1ST_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED | FROM_LEFT_2ND_BUTTON_PRESSED;

    const DWORD buttons = record.dwButtonState & SUPPORTED;
    MouseInputEvent event{record.dwMousePosition.X - viewport_origin.X, record.dwMousePosition.Y - viewport_origin.Y,
                          MouseButton::NO_BUTTON, MouseAction::MOVE, modifiers(record.dwControlKeyState)};
    mouse_offset = mouse_count = 0;

    if (record.dwEventFlags & (MOUSE_WHEELED | MOUSE_HWHEELED)) {
      const auto delta = static_cast<SHORT>(HIWORD(record.dwButtonState));

      if (delta == 0) {
        return;
      }

      event.button = record.dwEventFlags & MOUSE_HWHEELED
                       ? (delta > 0 ? MouseButton::WHEEL_RIGHT : MouseButton::WHEEL_LEFT)
                       : (delta > 0 ? MouseButton::WHEEL_UP : MouseButton::WHEEL_DOWN);
      event.action = MouseAction::SCROLL;
      mouse_pending[mouse_count++] = event;
    } else {
      constexpr std::array<DWORD, 3> BITS{FROM_LEFT_1ST_BUTTON_PRESSED, FROM_LEFT_2ND_BUTTON_PRESSED,
                                          RIGHTMOST_BUTTON_PRESSED};
      constexpr std::array<MouseButton, 3> IDENTITIES{MouseButton::LEFT, MouseButton::MIDDLE, MouseButton::RIGHT};

      const DWORD changed = buttons ^ mouse_buttons;

      for (std::size_t index = 0; index < BITS.size(); ++index) {
        if (changed & BITS[index]) {
          event.button = IDENTITIES[index];
          event.action = buttons & BITS[index] ? MouseAction::PRESS : MouseAction::RELEASE;
          mouse_pending[mouse_count++] = event;
        }
      }

      if (!changed && (record.dwEventFlags & MOUSE_MOVED)) {
        event.button = buttons & BITS[0]   ? MouseButton::LEFT
                       : buttons & BITS[1] ? MouseButton::MIDDLE
                       : buttons & BITS[2] ? MouseButton::RIGHT
                                           : MouseButton::NO_BUTTON;
        mouse_pending[mouse_count++] = event;
      }
    }

    mouse_buttons = buttons;
  }
};

TerminalSession::TerminalSession(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {
}

TerminalSession::TerminalSession(TerminalSession &&) noexcept = default;
TerminalSession &TerminalSession::operator=(TerminalSession &&) noexcept = default;
TerminalSession::~TerminalSession() noexcept = default;

std::expected<TerminalSession, TerminalError> TerminalSession::open(NativeTerminalHandles handles,
                                                                    TerminalOptions options) {
  auto decoder = InputDecoder::create(options.windows_input);

  if (!decoder || !options.max_input_bytes || !options.max_input_events || !options.max_output_bytes ||
      options.max_output_bytes > MAXDWORD || options.wait_cadence.count() <= 0 ||
      options.wait_cadence.count() >= INFINITE) {
    return std::unexpected(contractError(TerminalErrorKind::INVALID_OPTIONS));
  }

  auto state = std::make_unique<Impl>(options, std::move(*decoder));
  const auto input = reinterpret_cast<HANDLE>(handles.input);
  const auto output = reinterpret_cast<HANDLE>(handles.output);
  // Reject either redirected endpoint before duplicating or changing any mode.

  if (!GetConsoleMode(input, &state->input_mode) || !GetConsoleMode(output, &state->output_mode)) {
    return std::unexpected(TerminalError{TerminalErrorKind::UNSUPPORTED, static_cast<int>(GetLastError()), 0});
  }

  state->output_page = GetConsoleOutputCP();

  const auto rollback = [&state](TerminalError error) -> std::expected<TerminalSession, TerminalError> {
    error.restoration_code = state->restore();

    return std::unexpected(error);
  };

  if (!DuplicateHandle(GetCurrentProcess(), input, GetCurrentProcess(), &state->input, 0, FALSE,
                       DUPLICATE_SAME_ACCESS) ||
      !DuplicateHandle(GetCurrentProcess(), output, GetCurrentProcess(), &state->output, 0, FALSE,
                       DUPLICATE_SAME_ACCESS)) {
    return rollback(systemError());
  }

  const DWORD input_mode = (state->input_mode | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT |
                            ENABLE_VIRTUAL_TERMINAL_INPUT) &
                           ~(ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_QUICK_EDIT_MODE);
  state->input_changed = true;

  if (!SetConsoleMode(state->input, input_mode)) {
    return rollback(systemError());
  }

  state->output_changed = true;

  if (!SetConsoleMode(state->output,
                      state->output_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
    return rollback(systemError());
  }

  state->page_changed = true;

  if (!SetConsoleOutputCP(CP_UTF8)) {
    return rollback(systemError());
  }

  auto dimensions = state->dimensions();

  if (!dimensions) {
    return rollback(dimensions.error());
  }

  state->last_size = *dimensions;

  return TerminalSession(std::move(state));
}

std::expected<CellSize, TerminalError> TerminalSession::size() const noexcept {
  if (!impl_) {
    return std::unexpected(contractError(TerminalErrorKind::CLOSED));
  }

  return impl_->dimensions();
}

TerminalRead TerminalSession::read(TerminalDeadline deadline, std::stop_token stop) {
  TerminalRead result;
  const auto fail = [&result](TerminalError error) {
    result.status = TerminalStatus::FAILED;
    result.error = error;

    return std::move(result);
  };

  if (!impl_) {
    return fail(contractError(TerminalErrorKind::CLOSED));
  }

  auto &state = *impl_;

  if (state.input_failed) {
    return fail(contractError(TerminalErrorKind::INPUT_DECODE));
  }

  std::size_t records = 0;

  bool sample_dimensions = true;

  while (true) {
    if (stop.stop_requested()) {
      result.status = TerminalStatus::STOPPED;

      return result;
    }

    if (std::chrono::steady_clock::now() >= deadline) {
      result.status = result.events.empty() && !result.resized ? TerminalStatus::DEADLINE : TerminalStatus::PROGRESS;

      return result;
    }

    if (result.events.size() >= state.options.max_input_events) {
      return result;
    }

    // Sample every owner call, including calls draining a large repeat count.
    if (sample_dimensions) {
      sample_dimensions = false;

      auto dimensions = state.dimensions();

      if (!dimensions) {
        return fail(dimensions.error());
      }

      if (*dimensions != state.last_size) {
        state.last_size = *dimensions;
        result.resized = *dimensions;
      }
    }

    auto decoded = state.decoder.next(std::string_view(state.input_bytes).substr(state.input_offset), inputTime());

    if (!decoded || decoded->status == InputStatus::FAILED) {
      state.input_failed = true;

      return fail(contractError(TerminalErrorKind::INPUT_DECODE));
    }

    state.input_offset += decoded->consumed;

    if (decoded->event) {
      if (state.character_modifiers) {
        if (auto *text = std::get_if<TextInputEvent>(&*decoded->event)) {
          text->modifiers.shift |= state.character_modifiers->shift;
          text->modifiers.alt |= state.character_modifiers->alt;
          text->modifiers.control |= state.character_modifiers->control;
        }

        if (auto *key = std::get_if<KeyEvent>(&*decoded->event)) {
          key->modifiers.shift |= state.character_modifiers->shift;
          key->modifiers.alt |= state.character_modifiers->alt;
        }

        state.character_modifiers.reset();
      }

      result.events.push_back(std::move(*decoded->event));
    }

    if (state.input_offset == state.input_bytes.size()) {
      state.input_bytes.clear();
      state.input_offset = 0;
    }

    if (result.events.size() >= state.options.max_input_events) {
      return result;
    }

    if (decoded->event || !state.input_bytes.empty()) {
      continue;
    }

    if (state.mouse_offset < state.mouse_count) {
      result.events.emplace_back(state.mouse_pending[state.mouse_offset++]);

      if (result.events.size() >= state.options.max_input_events) {
        return result;
      }

      continue;
    }

    if (state.repeated) {
      if (++records > state.options.max_input_events) {
        return result;
      }

      const bool high = state.repeated->uChar.UnicodeChar >= 0xd800 && state.repeated->uChar.UnicodeChar <= 0xdbff;
      state.keyRecord(*state.repeated, result.events);

      if (high) {
        state.repeated.reset();

        continue;
      }

      if (++state.repeat_index >= state.repeated->wRepeatCount) {
        state.repeated.reset();
      }

      continue;
    }

    DWORD available = 0;

    if (!GetNumberOfConsoleInputEvents(state.input, &available)) {
      return fail(systemError());
    }

    if (!available) {
      if (!result.events.empty() || result.resized) {
        return result;
      }

      sample_dimensions = true;

      const DWORD waited = WaitForSingleObject(state.input, waitTime(deadline, state.options.wait_cadence));

      if (waited == WAIT_FAILED) {
        return fail(systemError());
      }

      continue;
    }

    if (++records > state.options.max_input_events) {
      return result;
    }

    INPUT_RECORD record{};
    DWORD count = 0;

    if (!ReadConsoleInputW(state.input, &record, 1, &count)) {
      return fail(systemError());
    }

    if (!count) {
      continue;
    }

    if (record.EventType == KEY_EVENT && carriesInput(record.Event.KeyEvent) && record.Event.KeyEvent.wRepeatCount) {
      state.repeated = record.Event.KeyEvent;
      state.repeat_index = 0;
      state.repeated_scalar.clear();
    } else if (record.EventType == WINDOW_BUFFER_SIZE_EVENT) {
      sample_dimensions = true;
    } else if (record.EventType == MOUSE_EVENT) {
      state.mouseRecord(record.Event.MouseEvent);
    }

    if (result.events.size() >= state.options.max_input_events) {
      return result;
    }
  }
}

std::expected<void, TerminalError> TerminalSession::submit(std::string bytes) {
  if (!impl_) {
    return std::unexpected(contractError(TerminalErrorKind::CLOSED));
  }

  if (impl_->uncertain) {
    return std::unexpected(contractError(TerminalErrorKind::OUTPUT_UNCERTAIN));
  }

  if (pendingBytes()) {
    return std::unexpected(contractError(TerminalErrorKind::OUTPUT_PENDING));
  }

  if (bytes.size() > impl_->options.max_output_bytes) {
    return std::unexpected(contractError(TerminalErrorKind::OUTPUT_LIMIT));
  }

  impl_->output_bytes = std::string(bytes.data(), bytes.size());
  impl_->output_offset = 0;

  return {};
}

TerminalWrite TerminalSession::flush(TerminalDeadline deadline, std::stop_token stop) {
  TerminalWrite result;
  const auto fail = [&result](TerminalError error) {
    result.status = TerminalStatus::FAILED;
    result.error = error;

    return result;
  };

  if (!impl_) {
    return fail(contractError(TerminalErrorKind::CLOSED));
  }

  auto &state = *impl_;

  result.pending = pendingBytes();

  if (state.uncertain) {
    return fail(contractError(TerminalErrorKind::OUTPUT_UNCERTAIN));
  }

  while (result.pending) {
    const bool stopped = stop.stop_requested();

    if (stopped || std::chrono::steady_clock::now() >= deadline) {
      // Once native I/O has been issued, cancellation does not reveal how many bytes
      // reached the console. Keep the batch alive and prohibit replay until close joins.
      if (state.writer) {
        CancelSynchronousIo(state.writer);
        state.uncertain = true;
        result.error = contractError(TerminalErrorKind::OUTPUT_UNCERTAIN);
      }

      result.status = stopped ? TerminalStatus::STOPPED : TerminalStatus::DEADLINE;

      return result;
    }

    if (!state.writer) {
      state.written = state.write_error = 0;
      state.writer = CreateThread(nullptr, 0, Impl::writeBatch, &state, 0, nullptr);

      if (!state.writer) {
        return fail(systemError());
      }
    }

    const DWORD waited = WaitForSingleObject(state.writer, waitTime(deadline, state.options.wait_cadence));

    if (waited == WAIT_TIMEOUT) {
      continue;
    }

    if (waited != WAIT_OBJECT_0) {
      state.uncertain = true;

      return fail(systemError());
    }

    // WAIT_OBJECT_0 is the completion boundary for the worker's written/error fields.
    // Read them only after that wait and retire the handle before another batch begins.
    CloseHandle(state.writer);
    state.writer = nullptr;

    if (state.write_error || !state.written) {
      state.uncertain = true;

      return fail(systemError(state.write_error ? state.write_error : ERROR_WRITE_FAULT));
    }

    state.output_offset += state.written;
    result.completed += state.written;
    result.pending = pendingBytes();
  }

  state.output_bytes.clear();
  state.output_offset = 0;

  return result;
}

std::size_t TerminalSession::pendingBytes() const noexcept {
  return impl_ ? impl_->output_bytes.size() - impl_->output_offset : 0;
}

std::expected<void, TerminalError> TerminalSession::saveCursorVisibility() noexcept {
  if (!impl_) {
    return std::unexpected(contractError(TerminalErrorKind::CLOSED));
  }

  if (!GetConsoleCursorInfo(impl_->output, &impl_->saved_cursor)) {
    return std::unexpected(systemError());
  }

  impl_->cursor_saved = true;

  return {};
}

std::expected<void, TerminalError> TerminalSession::joinOutputWriter() noexcept {
  if (!impl_) {
    return {};
  }

  const int joined = impl_->stopWriter();

  if (joined == 0 && impl_->uncertain) {
    // A cancelled client write can finish before the console host processes its
    // accepted bytes. Synchronize with the host before budgeting the final batch.
    CONSOLE_SCREEN_BUFFER_INFO info{};

    if (!GetConsoleScreenBufferInfo(impl_->output, &info)) {
      return std::unexpected(systemError());
    }
  }

  return joined == 0 ? std::expected<void, TerminalError>{} : std::unexpected(systemError(static_cast<DWORD>(joined)));
}

std::expected<void, TerminalError> TerminalSession::closeWithOutput(std::string bytes,
                                                                    TerminalDeadline deadline) noexcept {
  if (!impl_) {
    return {};
  }

  std::optional<TerminalError> failure;
  const int joined = impl_->stopWriter();

  if (joined != 0) {
    failure = systemError(static_cast<DWORD>(joined));
  }

  if (bytes.size() > impl_->options.max_output_bytes) {
    failure = contractError(TerminalErrorKind::OUTPUT_LIMIT);
  } else {
    // Never replay the uncertain frame. A distinct final batch may still return to the main screen.
    impl_->output_bytes = std::move(bytes);
    impl_->output_offset = 0;
    impl_->uncertain = false;

    const auto written = flush(deadline);

    if (written.error) {
      failure = written.error;
    } else if (written.pending != 0) {
      failure = systemError(ERROR_TIMEOUT);
    }
  }

  auto closed = close();

  if (!closed) {
    if (!failure) {
      return closed;
    }

    failure->restoration_code = closed.error().native_code;
  }

  return failure ? std::unexpected(*failure) : std::expected<void, TerminalError>{};
}

std::expected<void, TerminalError> TerminalSession::close() noexcept {
  if (!impl_) {
    return {};
  }

  const int error = impl_->restore();
  impl_.reset();

  if (error) {
    return std::unexpected(systemError(static_cast<DWORD>(error)));
  }

  return {};
}
} // namespace portals
