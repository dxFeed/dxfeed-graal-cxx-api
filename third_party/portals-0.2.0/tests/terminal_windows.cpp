// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include "../examples/gallery/form.hpp"
#include <algorithm>
#include <portals/application.hpp>
#include <portals/diff.hpp>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <atomic>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr DWORD CHILD_DEADLINE_MS = 20000;
constexpr DWORD WRITER_OBSERVE_MS = 500;
constexpr DWORD MAXIMUM_DRAIN_PAUSE_MS = 1500;
constexpr DWORD SLOW_DRAIN_PAUSE_MS = 3500;
constexpr auto RESUMED_OUTPUT_TIMEOUT = std::chrono::seconds{10};
constexpr auto SCREEN_MODE_TIMEOUT = std::chrono::seconds{2};
constexpr DWORD PRESSURE_BYTES = 1024 * 1024;
constexpr DWORD WATCHDOG_EXIT = 75;
constexpr COORD FRAME_VIEWPORT{240, 160};
constexpr std::size_t FRAME_OUTPUT_LIMIT = 2 * 1024 * 1024;
constexpr std::size_t FRAME_MINIMUM_PRESSURE_BYTES = 1024 * 1024;
constexpr portals::CellBufferLimits FRAME_BUFFER_LIMITS{240, 160, 38400, 16, 64, 8};
constexpr portals::FrameWriterLimits FRAME_WRITER_LIMITS{FRAME_OUTPUT_LIMIT, 1024 * 1024};
constexpr std::size_t MAXIMUM_PRESSURE_FRAMES = 16;
constexpr DWORD MINIMUM_FRAME_WAIT_MS = 500;
constexpr DWORD MAXIMUM_FRAME_CANCELLATION_MS = 1200;
constexpr DWORD SLOW_FRAME_PREPARATION_MS = 1000;
constexpr DWORD PRESSURE_OBSERVE_POLL_MS = 1;

struct Handle {
  HANDLE value = nullptr;

  Handle() = default;

  explicit Handle(HANDLE h) : value(h) {
  }

  Handle(const Handle &) = delete;
  Handle &operator=(const Handle &) = delete;

  ~Handle() {
    reset();
  }

  void reset(HANDLE h = nullptr) {
    if (value && value != INVALID_HANDLE_VALUE) {
      CloseHandle(value);
    }

    value = h;
  }
};

void require(bool result, const char *operation) {
  if (!result) {
    throw std::runtime_error(std::string(operation) + " error=" + std::to_string(GetLastError()));
  }
}

// A driver may refuse synchronous cancellation. Override only this fixture
// child's import, leaving normal cancellation cases on the real Windows API.
class UncancellableDriver {
public:
  UncancellableDriver() {
    const auto native = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CancelSynchronousIo");

    require(native != nullptr, "native cancellation entry point");
    original_ = std::bit_cast<ULONG_PTR>(native);

    const auto image = reinterpret_cast<unsigned char *>(GetModuleHandleW(nullptr));
    const auto dos = reinterpret_cast<IMAGE_DOS_HEADER *>(image);
    const auto headers = reinterpret_cast<IMAGE_NT_HEADERS *>(image + dos->e_lfanew);
    auto imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(
      image + headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    for (; imports->Name != 0 && !slot_; ++imports) {
      auto entries = reinterpret_cast<IMAGE_THUNK_DATA *>(image + imports->FirstThunk);

      for (; entries->u1.Function != 0; ++entries) {
        if (entries->u1.Function == original_) {
          slot_ = reinterpret_cast<ULONG_PTR *>(&entries->u1.Function);

          break;
        }
      }
    }

    require(slot_ != nullptr, "fixture cancellation import");

    DWORD protection{};

    require(VirtualProtect(slot_, sizeof(*slot_), PAGE_READWRITE, &protection), "fixture import writable");
    *slot_ = std::bit_cast<ULONG_PTR>(&refuseCancellation);

    DWORD ignored{};

    require(VirtualProtect(slot_, sizeof(*slot_), protection, &ignored), "fixture import protection restored");
  }

  UncancellableDriver(const UncancellableDriver &) = delete;
  UncancellableDriver &operator=(const UncancellableDriver &) = delete;

  ~UncancellableDriver() {
    DWORD protection{};

    if (VirtualProtect(slot_, sizeof(*slot_), PAGE_READWRITE, &protection)) {
      *slot_ = original_;

      DWORD ignored{};

      VirtualProtect(slot_, sizeof(*slot_), protection, &ignored);
    }
  }

private:
  static BOOL WINAPI refuseCancellation(HANDLE) noexcept {
    SetLastError(ERROR_NOT_FOUND);

    return FALSE;
  }

  ULONG_PTR *slot_{};
  ULONG_PTR original_{};
};

enum Stage : DWORD {
  INPUT_READY = 1,
  INPUT_DONE,
  PRESSURE_READY,
  CANCEL_RESULT,
  PRESSURE_DONE,
  CHILD_FAILURE,
  WRITER_COMPLETE,
  CONSOLE_CLASSIFICATION,
  RESIZE_READY = 10,
  IDLE_RESIZE_READY = 12,
  PIPE_CLASSIFICATION,
  FRAME_COMMITTED,
  FRAME_POISONED,
  FRAME_JOINED,
  FRAME_RECOVERED,
  FRAME_PRESSURE_OBSERVED,
  SCREEN_RESIZE_READY,
  SCREEN_RESET_READY,
  SCREEN_MODES_RESTORED
};

enum Flag : DWORD {
  BLOCKED = 1,
  WRITE_SUCCEEDED = 2,
  HIDDEN_WINDOW = 4,
  HOST_UNICODE = 8,
  HOST_CTRL_C = 16,
  VIEWPORT_RESIZED = 32,
  HOST_RESIZE_RECORD = 64,
  RESTORED = 128,
  SCREEN_RESTORED = 256
};

struct Message {
  DWORD stage;
  DWORD error;
  DWORD flags;
  DWORD elapsedMs;
  std::uint64_t amount;
  char detail[128];
};

void report(HANDLE resultPipe, Stage stage, DWORD error = 0, DWORD flags = 0, DWORD elapsed = 0,
            std::uint64_t amount = 0, const char *detail = "") {
  Message message{stage, error, flags, elapsed, amount, {}};

  std::snprintf(message.detail, sizeof(message.detail), "%s", detail);

  DWORD written = 0;

  require(WriteFile(resultPipe, &message, sizeof(message), &written, nullptr) && written == sizeof(message), "report");
}

INPUT_RECORD key(wchar_t character, BOOL down = TRUE) {
  INPUT_RECORD record{};

  record.EventType = KEY_EVENT;
  record.Event.KeyEvent.bKeyDown = down;
  record.Event.KeyEvent.wRepeatCount = 1;
  record.Event.KeyEvent.uChar.UnicodeChar = character;

  return record;
}

using namespace std::chrono_literals;
using namespace portals;

struct NativeConsole {
  Handle input{CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, 0, nullptr)};
  Handle output{CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                            OPEN_EXISTING, 0, nullptr)};
  DWORD inputMode{}, outputMode{};
  UINT inputPage{}, outputPage{};

  NativeConsole() {
    require(GetConsoleMode(input.value, &inputMode) && GetConsoleMode(output.value, &outputMode), "snapshot modes");
    inputPage = GetConsoleCP();
    outputPage = GetConsoleOutputCP();
  }

  NativeTerminalHandles handles() const {
    return {reinterpret_cast<std::intptr_t>(input.value), reinterpret_cast<std::intptr_t>(output.value)};
  }

  void checkRestored() const {
    DWORD in = 0, out = 0;

    require(GetConsoleMode(input.value, &in) && GetConsoleMode(output.value, &out) && in == inputMode &&
              out == outputMode && GetConsoleCP() == inputPage && GetConsoleOutputCP() == outputPage,
            "exact modes/pages restoration");
  }

  void inject(const std::vector<INPUT_RECORD> &records) const {
    DWORD count = 0;
    DWORD currentMode = 0;

    require(GetConsoleMode(input.value, &currentMode), "snapshot injection mode");
    require(SetConsoleMode(input.value, currentMode & ~ENABLE_VIRTUAL_TERMINAL_INPUT), "raw-record injection mode");
    require(WriteConsoleInputW(input.value, records.data(), static_cast<DWORD>(records.size()), &count) &&
              count == records.size(),
            "inject native records");
    require(SetConsoleMode(input.value, currentMode), "restore VT mode before session read");
  }
};

std::vector<InputEvent> collect(TerminalSession &session, std::size_t desired) {
  std::vector<InputEvent> events;
  const auto deadline = std::chrono::steady_clock::now() + 2s;

  while (events.size() < desired && std::chrono::steady_clock::now() < deadline) {
    auto batch = session.read(deadline);

    require(batch.status != TerminalStatus::FAILED, "semantic read");
    require(batch.events.size() <= 2 && batch.bytes.empty(), "bounded events-only batch");

    for (auto &event : batch.events) {
      events.push_back(std::move(event));
    }
  }

  if (events.size() != desired) {
    std::string detail = "semantic count " + std::to_string(events.size()) + "/" + std::to_string(desired);

    for (const auto &event : events) {
      detail += " type=" + std::to_string(event.index());

      if (const auto *text = std::get_if<TextInputEvent>(&event)) {
        detail += ":" + std::to_string(text->scalar);
      }
    }

    throw std::runtime_error(detail);
  }

  return events;
}

DWORD inputCase(HANDLE resultPipe) {
  NativeConsole native;

  require(!IsWindowVisible(GetConsoleWindow()), "headless console");

  TerminalOptions options;

  options.max_input_events = 2;
  options.windows_input.max_paste_bytes = 64;

  auto opened = TerminalSession::open(native.handles(), options);

  require(opened.has_value(), "native TerminalSession open");

  TerminalSession session = std::move(*opened);

  require(!opened->size() && opened->close().has_value(), "moved source closed");
  require(FlushConsoleInputBuffer(native.input.value), "clear fixture input");

  DWORD mode = 0;

  require(GetConsoleMode(native.input.value, &mode) && (mode & ENABLE_VIRTUAL_TERMINAL_INPUT) &&
            !(mode & (ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)),
          "VT input and Ctrl+C key policy");

  auto up = key(0);

  up.Event.KeyEvent.wVirtualKeyCode = VK_UP;

  auto mouse = INPUT_RECORD{};

  mouse.EventType = MOUSE_EVENT;
  mouse.Event.MouseEvent.dwMousePosition = {4, 7};
  mouse.Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;

  auto repeat = key(L'r');

  repeat.Event.KeyEvent.wRepeatCount = 3;

  auto ctrlSpace = key(0);

  ctrlSpace.Event.KeyEvent.wVirtualKeyCode = VK_SPACE;
  ctrlSpace.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED;
  native.inject(
    {key(L'\x0416'), key(L'\x0416', FALSE), key(L'\xd83d'), key(L'\xde00'), up, mouse, key(3), repeat, ctrlSpace});

  const auto events = collect(session, 9);

  require(std::get<TextInputEvent>(events[0]).scalar == U'\u0416' &&
            std::get<TextInputEvent>(events[1]).scalar == U'\U0001f600',
          "Unicode surrogate composition");
  require(std::get<KeyEvent>(events[2]).key == InputKey::UP, "native arrow order");
  require(std::get<MouseInputEvent>(events[3]) == MouseInputEvent{4, 7, MouseButton::LEFT, MouseAction::PRESS, {}},
          "native mouse order/payload");
  require(std::get<KeyEvent>(events[4]).scalar == U'C' && std::get<KeyEvent>(events[4]).modifiers.control,
          "Ctrl+C semantic key");

  for (std::size_t index = 5; index < 8; ++index) {
    require(std::get<TextInputEvent>(events[index]).scalar == U'r', "bounded repeat preservation");
  }

  require(std::get<KeyEvent>(events[8]).modifiers.control, "Ctrl+Space NUL preserved");

  auto nativeAlt = key(L'x');

  nativeAlt.Event.KeyEvent.dwControlKeyState = LEFT_ALT_PRESSED | SHIFT_PRESSED;
  nativeAlt.Event.KeyEvent.wRepeatCount = 3;

  auto altGr = key(L'\x20ac');

  altGr.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED;
  native.inject({key(27), key(L'x'), nativeAlt, altGr});

  const auto modified = collect(session, 5);

  require(std::get<TextInputEvent>(modified[0]).modifiers.alt, "VT ESC Alt survives native modifier merge");

  for (std::size_t index = 1; index < 4; ++index) {
    const auto &text = std::get<TextInputEvent>(modified[index]);

    require(text.modifiers.alt && text.modifiers.shift, "native repeated modifiers preserved");
  }

  require(std::get<TextInputEvent>(modified[4]).modifiers == InputModifiers{}, "AltGr character is text");

  // ConPTY uses Alt-release records for text without a keyboard mapping.
  // Ordinary key releases must still be ignored to avoid duplicate text.
  std::vector<INPUT_RECORD> altText;

  for (const wchar_t character : std::wstring_view(L"\u0301\U0001f469\u200d\U0001f4bb")) {
    auto record = key(character, FALSE);

    record.Event.KeyEvent.wVirtualKeyCode = VK_MENU;
    altText.push_back(record);
  }

  native.inject(altText);

  const auto composed = collect(session, 4);
  constexpr std::u32string_view EXPECTED_ALT_TEXT = U"\u0301\U0001f469\u200d\U0001f4bb";

  for (std::size_t index = 0; index < EXPECTED_ALT_TEXT.size(); ++index) {
    const auto &text = std::get<TextInputEvent>(composed[index]);

    require(text.scalar == EXPECTED_ALT_TEXT[index] && text.modifiers == InputModifiers{},
            "Alt-release Unicode text preserves combining marks and surrogate pairs");
  }

  mouse.Event.MouseEvent.dwButtonState = RIGHTMOST_BUTTON_PRESSED;
  native.inject({mouse});

  const auto switched = collect(session, 2);

  require(std::get<MouseInputEvent>(switched[0]).button == MouseButton::LEFT &&
            std::get<MouseInputEvent>(switched[0]).action == MouseAction::RELEASE &&
            std::get<MouseInputEvent>(switched[1]).button == MouseButton::RIGHT &&
            std::get<MouseInputEvent>(switched[1]).action == MouseAction::PRESS,
          "all changed mouse buttons retained");

  auto highRepeat = key(L'\xd83d');
  auto lowRepeat = key(L'\xde00');

  highRepeat.Event.KeyEvent.wRepeatCount = lowRepeat.Event.KeyEvent.wRepeatCount = 3;
  native.inject({highRepeat, lowRepeat});

  for (const auto &event : collect(session, 3)) {
    require(std::get<TextInputEvent>(event).scalar == U'\U0001f600', "surrogate repeated pair");
  }

  auto many = key(L'm');

  many.Event.KeyEvent.wRepeatCount = MAXWORD;
  native.inject({many});

  auto bounded = session.read(std::chrono::steady_clock::now() + 500ms);

  require(bounded.events.size() <= options.max_input_events, "maximum repeat count remains bounded");
  report(resultPipe, RESIZE_READY);

  bool repeatResize = false;
  const auto repeatDeadline = std::chrono::steady_clock::now() + 1s;

  while (!repeatResize && std::chrono::steady_clock::now() < repeatDeadline) {
    auto batch = session.read(repeatDeadline);

    require(batch.events.size() <= options.max_input_events, "repeat and resize stay bounded");
    repeatResize = batch.resized == CellSize{95, 31};
    Sleep(1);
  }

  require(repeatResize, "resize observed while repeat still pending");
  // Close discards the remaining repeat; it never expands a 65535-item queue.
  require(session.close().has_value(), "close pending repeat");
  native.checkRestored();
  opened = TerminalSession::open(native.handles(), options);
  require(opened.has_value(), "reopen after pending input");
  session = std::move(*opened);
  require(FlushConsoleInputBuffer(native.input.value), "clear discarded fixture input");

  std::vector<INPUT_RECORD> paste;

  for (const char ch : std::string("\x1b[A\x1b[200~a\x03\x1b[A\x1b[201~z")) {
    paste.push_back(key(ch));
  }

  native.inject(paste);

  const auto decoded = collect(session, 3);

  require(std::get<KeyEvent>(decoded[0]).key == InputKey::UP, "VT arrow decoder interop");
  require(std::get<PasteInputEvent>(decoded[1]).bytes == "a\x03\x1b[A" &&
            std::get<TextInputEvent>(decoded[2]).scalar == U'z',
          "paste controls remain payload and ordered");

  INPUT_RECORD ignored{};

  ignored.EventType = FOCUS_EVENT;
  native.inject({ignored, key(L'q', FALSE)});

  auto idle = session.read(std::chrono::steady_clock::now() + 30ms);

  require(idle.status == TerminalStatus::DEADLINE, "nontext-only records do not block read");
  native.inject({ignored});
  report(resultPipe, IDLE_RESIZE_READY);

  const auto idleResizeStart = std::chrono::steady_clock::now();
  const auto idleResize = session.read(idleResizeStart + 1s);

  require(idleResize.resized == CellSize{97, 33} && std::chrono::steady_clock::now() - idleResizeStart < 700ms,
          "delayed resize wakes long read after ignored record");

  std::stop_source stopped;

  stopped.request_stop();
  require(session.read(std::chrono::steady_clock::now() - 1ms, stopped.get_token()).status == TerminalStatus::STOPPED,
          "input stop precedes deadline");
  report(resultPipe, INPUT_READY);

  constexpr std::u32string_view EXPECTED_HOST_TEXT = U"\u0416e\u0301\U0001f469\u200d\U0001f4bb\u754c";
  std::u32string hostText;
  bool control = false, resized = false;
  const auto hostDeadline = std::chrono::steady_clock::now() + 3s;

  while ((hostText.size() < EXPECTED_HOST_TEXT.size() || !control || !resized) &&
         std::chrono::steady_clock::now() < hostDeadline) {
    auto batch = session.read(hostDeadline);

    require(batch.status != TerminalStatus::FAILED, "host input read");
    resized |= batch.resized == CellSize{91, 37};

    for (const auto &event : batch.events) {
      if (const auto *text = std::get_if<TextInputEvent>(&event)) {
        hostText += text->scalar;
      }

      if (const auto *pressed = std::get_if<KeyEvent>(&event)) {
        control |= pressed->scalar == U'C';
      }
    }
  }

  require(hostText == EXPECTED_HOST_TEXT && control && resized, "actual ConPTY Unicode/CtrlC/viewport resize");
  require(session.submit("native VT output\r\n").has_value(), "submit output");

  const auto preStopped = session.flush(std::chrono::steady_clock::now() + 1s, stopped.get_token());

  require(preStopped.status == TerminalStatus::STOPPED && !preStopped.error && preStopped.pending == 18,
          "unissued stopped output remains retryable");

  const auto preExpired = session.flush(std::chrono::steady_clock::now() - 1ms);

  require(preExpired.status == TerminalStatus::DEADLINE && !preExpired.error && preExpired.pending == 18,
          "unissued expired output remains retryable");

  const auto output = session.flush(std::chrono::steady_clock::now() + 1s);

  require(output.status == TerminalStatus::PROGRESS && output.completed == 18 && output.pending == 0,
          "actual output completion");
  require(session.close().has_value() && session.close().has_value(), "explicit close idempotence");
  native.checkRestored();

  try {
    auto exceptional = TerminalSession::open(native.handles());

    require(exceptional.has_value(), "exception path open");
    throw std::runtime_error("caught fixture error");
  } catch (const std::runtime_error &) {
  }

  native.checkRestored();

  TerminalOptions invalid;

  invalid.max_input_events = 0;
  require(!TerminalSession::open(native.handles(), invalid), "invalid options rejected");
  native.checkRestored();

  Handle readOnly(
    CreateFileW(L"CONOUT$", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
  auto rollback = TerminalSession::open({native.handles().input, reinterpret_cast<std::intptr_t>(readOnly.value)});

  require(!rollback && rollback.error().kind == TerminalErrorKind::SYSTEM, "output mode error surfaced");
  native.checkRestored();

  auto wrongRole = TerminalSession::open({native.handles().input, native.handles().input});

  require(!wrongRole && wrongRole.error().kind == TerminalErrorKind::SYSTEM, "input handle is not output buffer");
  native.checkRestored();

  HANDLE read = nullptr, write = nullptr;

  require(CreatePipe(&read, &write, nullptr, 4096), "redirected pipe");

  Handle pipeRead(read), pipeWrite(write);

  for (const auto handles : {NativeTerminalHandles{reinterpret_cast<std::intptr_t>(read), native.handles().output},
                             NativeTerminalHandles{native.handles().input, reinterpret_cast<std::intptr_t>(write)}}) {
    auto rejected = TerminalSession::open(handles);

    require(!rejected && rejected.error().kind == TerminalErrorKind::UNSUPPORTED, "redirected endpoint rejected");
    native.checkRestored();
  }

  report(resultPipe, INPUT_DONE, 0, RESTORED);

  return 0;
}

DWORD pressureCase(HANDLE resultPipe, const wchar_t *kind) {
  NativeConsole native;
  auto opened = TerminalSession::open(native.handles());

  require(opened.has_value(), "pressure open");

  std::string bytes(PRESSURE_BYTES, 'x');

  for (std::size_t index = 79; index < bytes.size(); index += 80) {
    bytes[index] = '\n';
  }

  require(opened->submit(std::move(bytes)).has_value(), "pressure submit");

  const bool resume = wcscmp(kind, L"resume") == 0;
  const bool stop = wcscmp(kind, L"stop") == 0;
  std::stop_source source;
  std::jthread stopper;

  if (stop) {
    stopper = std::jthread([&source] {
      Sleep(WRITER_OBSERVE_MS);
      source.request_stop();
    });
  }

  const ULONGLONG start = GetTickCount64();

  report(resultPipe, PRESSURE_READY);

  const auto writeBudget = resume ? RESUMED_OUTPUT_TIMEOUT : stop ? 4s : 500ms;
  const auto written = opened->flush(std::chrono::steady_clock::now() + writeBudget, source.get_token());
  const DWORD elapsed = static_cast<DWORD>(GetTickCount64() - start);

  require(written.status == (resume ? TerminalStatus::PROGRESS
                             : stop ? TerminalStatus::STOPPED
                                    : TerminalStatus::DEADLINE),
          "pressure completion reason");

  if (resume) {
    require(written.completed == PRESSURE_BYTES && written.pending == 0 && elapsed >= 500,
            "paused output completes after host resume");
  } else {
    require(elapsed < 1200 && written.pending > 0 && written.error &&
              written.error->kind == TerminalErrorKind::OUTPUT_UNCERTAIN,
            "flush returns before host resume");

    auto rejected = opened->submit("must not replay");

    require(!rejected && rejected.error().kind == TerminalErrorKind::OUTPUT_UNCERTAIN,
            "cancelled output cannot replay suffix");
    require(opened->flush(std::chrono::steady_clock::now() + 100ms).status == TerminalStatus::FAILED,
            "cancelled flush cannot retry");
  }

  report(resultPipe, WRITER_COMPLETE, 0, BLOCKED, elapsed, written.completed);
  require(opened->close().has_value(), "pressure join and restore");
  native.checkRestored();
  report(resultPipe, PRESSURE_DONE, 0, BLOCKED | RESTORED, static_cast<DWORD>(GetTickCount64() - start),
         written.completed);

  return 0;
}

bool isFrameCase(const wchar_t *kind) {
  return wcscmp(kind, L"frame-resume") == 0 || wcscmp(kind, L"frame-cancel") == 0 || wcscmp(kind, L"frame-stop") == 0;
}

CellBuffer makeFrame(bool recovery, bool alternate = false) {
  auto frame = CellBuffer::from({FRAME_VIEWPORT.X, FRAME_VIEWPORT.Y}, {}, FRAME_BUFFER_LIMITS);

  require(frame.has_value(), "bounded native frame allocation");

  const auto ascii = GraphemeText::from(alternate ? "B" : "A");
  const CellStyle first{{RgbColor{255, 127, 64}}, {RgbColor{5, 41, 90}}, true, true, true};
  const CellStyle second{{RgbColor{3, 127, 9}}, {RgbColor{84, 193, 211}}, false, true, true};

  require(ascii.has_value(), "native frame text");

  if (!recovery) {
    for (int y = 0; y < FRAME_VIEWPORT.Y; ++y) {
      for (int x = 0; x < FRAME_VIEWPORT.X; ++x) {
        require(frame->put({x, y}, *ascii, 0, x % 2 ? first : second).has_value(), "native pressure cell");
      }
    }
  } else {
    const auto mark = GraphemeText::from("R");
    const auto omega = GraphemeText::from("\xce\xa9");
    const auto wide = GraphemeText::from("\xe4\xb8\xad");

    require(mark && omega && wide, "native recovery text");
    require(frame->put({0, 0}, *mark, 0, first).has_value() && frame->put({1, 0}, *omega, 0).has_value() &&
              frame->put({2, 0}, *wide, 0).has_value() &&
              frame->put({FRAME_VIEWPORT.X - 2, FRAME_VIEWPORT.Y - 1}, *wide, 0, second).has_value(),
            "native recovery cells");
  }

  return std::move(*frame);
}

CHAR_INFO nativeCell(HANDLE output, SHORT x, SHORT y) {
  CHAR_INFO cell{};
  SMALL_RECT area{x, y, x, y};

  require(ReadConsoleOutputW(output, &cell, {1, 1}, {0, 0}, &area), "native frame cell observation");
  require(area.Left == x && area.Right == x && area.Top == y && area.Bottom == y, "native cell inside viewport");

  return cell;
}

void checkNativeFrame(HANDLE output, bool recovery, bool alternate = false) {
  CONSOLE_SCREEN_BUFFER_INFO info{};

  require(GetConsoleScreenBufferInfo(output, &info), "native frame cursor observation");
  require(info.dwCursorPosition.X == 0 && info.dwCursorPosition.Y == 0 && info.srWindow.Left == 0 &&
            info.srWindow.Top == 0,
          "completed frame cursor at home");

  if (!recovery) {
    const wchar_t expected = alternate ? L'B' : L'A';

    require(nativeCell(output, 0, 0).Char.UnicodeChar == expected &&
              nativeCell(output, FRAME_VIEWPORT.X - 1, FRAME_VIEWPORT.Y - 1).Char.UnicodeChar == expected,
            "native full frame reaches lower-right without scroll");
  } else {
    const auto lowerLead = nativeCell(output, FRAME_VIEWPORT.X - 2, FRAME_VIEWPORT.Y - 1);
    const auto lowerTail = nativeCell(output, FRAME_VIEWPORT.X - 1, FRAME_VIEWPORT.Y - 1);

    require(nativeCell(output, 0, 0).Char.UnicodeChar == L'R' &&
              nativeCell(output, 1, 0).Char.UnicodeChar == L'\x03a9' &&
              nativeCell(output, 2, 0).Char.UnicodeChar == L'\x4e2d' && lowerLead.Char.UnicodeChar == L'\x4e2d' &&
              (lowerLead.Attributes & COMMON_LVB_LEADING_BYTE) && (lowerTail.Attributes & COMMON_LVB_TRAILING_BYTE),
            "native recovered BMP and complete CJK width at lower-right");
    require(nativeCell(output, FRAME_VIEWPORT.X - 1, 0).Char.UnicodeChar == L' ' &&
              nativeCell(output, FRAME_VIEWPORT.X / 2, FRAME_VIEWPORT.Y / 2).Char.UnicodeChar == L' ',
            "native recovery removes stale pressure cells");
  }
}

DWORD frameCase(HANDLE resultPipe, const wchar_t *kind) {
  NativeConsole native;

  require(!IsWindowVisible(GetConsoleWindow()), "headless native frame console");

  TerminalOptions options;

  options.max_output_bytes = FRAME_OUTPUT_LIMIT;

  auto session = TerminalSession::open(native.handles(), options);

  require(session.has_value() && session->size() == CellSize{FRAME_VIEWPORT.X, FRAME_VIEWPORT.Y},
          "native frame viewport matches buffer");

  TerminalFrameOutput output(*session);
  FrameWriter writer(output, FRAME_WRITER_LIMITS);
  auto desired = makeFrame(false);
  const auto encoded = encodeDiff(nullptr, desired, FRAME_OUTPUT_LIMIT);

  require(encoded && encoded->size() >= FRAME_MINIMUM_PRESSURE_BYTES, "native frame creates bounded pressure batch");

  auto expectedBytes = encoded->size();

  require(writer.submit(std::move(desired)).has_value() && session->pendingBytes() == 0 && writer.pending() &&
            !writer.committedFrame(),
          "frame submission does no native I/O or belief commit");

  std::stop_source preStopped;

  preStopped.request_stop();

  const auto stoppedBeforeIo = writer.flush(std::chrono::steady_clock::now() + 1s, preStopped.get_token());

  require(stoppedBeforeIo.status == TerminalStatus::STOPPED && !stoppedBeforeIo.error &&
            stoppedBeforeIo.completed == 0 && stoppedBeforeIo.pending == expectedBytes && writer.pending() &&
            !writer.poisoned() && !writer.committedFrame(),
          "frame stop before I/O preserves retryable batch and unknown belief");

  const auto expiredBeforeIo = writer.flush(std::chrono::steady_clock::now() - 1ms);

  require(expiredBeforeIo.status == TerminalStatus::DEADLINE && !expiredBeforeIo.error &&
            expiredBeforeIo.pending == expectedBytes && !writer.poisoned(),
          "frame deadline before I/O preserves retryable batch");

  auto blockedFrame = makeFrame(true);
  const auto rejectedPending = writer.submit(std::move(blockedFrame));

  require(!rejectedPending && rejectedPending.error() == FrameError::PENDING &&
            blockedFrame.size() == CellSize{FRAME_VIEWPORT.X, FRAME_VIEWPORT.Y},
          "native pending frame cannot be replaced");

  const bool resume = wcscmp(kind, L"frame-resume") == 0;
  const bool stop = wcscmp(kind, L"frame-stop") == 0;
  std::stop_source source;
  std::atomic<ULONGLONG> activeFlushStarted{0};
  std::atomic<bool> observerReady{false};
  std::atomic<bool> resumePressureObserved{false};
  std::exception_ptr observerError;
  std::jthread pressureObserver;

  if (resume || stop) {
    // Start the observer before ConPTY pressure can delay new thread startup.
    pressureObserver = std::jthread([&](std::stop_token observerStop) {
      observerReady.store(true);

      try {
        while (!observerStop.stop_requested()) {
          const auto flushStarted = activeFlushStarted.load();

          if (flushStarted != 0 && GetTickCount64() - flushStarted >= MINIMUM_FRAME_WAIT_MS &&
              activeFlushStarted.load() == flushStarted) {
            if (resume) {
              resumePressureObserved.store(true);
              report(resultPipe, FRAME_PRESSURE_OBSERVED, 0, BLOCKED,
                     static_cast<DWORD>(GetTickCount64() - flushStarted));
            } else {
              source.request_stop();
            }

            return;
          }

          Sleep(PRESSURE_OBSERVE_POLL_MS);
        }
      } catch (...) {
        observerError = std::current_exception();
      }
    });

    while (!observerReady.load()) {
      Sleep(PRESSURE_OBSERVE_POLL_MS);
    }
  }

  const ULONGLONG start = GetTickCount64();

  report(resultPipe, PRESSURE_READY, 0, HIDDEN_WINDOW, 0, expectedBytes);

  // ConPTY can accept one complete viewport before its renderer blocks on the
  // host pipe. Bound the warmup series and require actual pending uncertainty,
  // rather than treating a large encoded byte count as proof of backpressure.
  const auto writeBudget = resume ? RESUMED_OUTPUT_TIMEOUT : stop ? 4s : 500ms;
  TerminalWrite written;
  DWORD elapsed = 0;
  DWORD flushWaitMs = 0;
  bool alternate = false;
  std::size_t completedFrames = 0;

  for (std::size_t attempt = 0; attempt < MAXIMUM_PRESSURE_FRAMES; ++attempt) {
    if (attempt == 1) {
      // Preparation must not consume a later flush's cancellation budget.
      Sleep(SLOW_FRAME_PREPARATION_MS);
    }

    const ULONGLONG flushStarted = GetTickCount64();

    if (resume || stop) {
      activeFlushStarted.store(flushStarted);
    }

    // Each flush owns its wait budget; frame preparation is outside it.
    const auto writeDeadline = std::chrono::steady_clock::now() + writeBudget;

    written = writer.flush(writeDeadline, source.get_token());
    activeFlushStarted.store(0);
    flushWaitMs = static_cast<DWORD>(GetTickCount64() - flushStarted);
    elapsed = static_cast<DWORD>(GetTickCount64() - start);

    char completion[128]{};

    std::snprintf(completion, sizeof(completion),
                  "status=%d pending=%llu poisoned=%d committed=%d attempt=%llu flush_ms=%lu",
                  static_cast<int>(written.status), static_cast<unsigned long long>(written.pending), writer.poisoned(),
                  writer.committedFrame() != nullptr, static_cast<unsigned long long>(attempt + 1), flushWaitMs);
    report(resultPipe, WRITER_COMPLETE, written.error ? static_cast<DWORD>(written.error->native_code) : 0, 0, elapsed,
           written.completed, completion);

    if (written.status != TerminalStatus::PROGRESS) {
      break;
    }

    ++completedFrames;
    require(written.completed == expectedBytes && written.pending == 0 && writer.committedFrame(),
            "native warmup frame commits complete bytes only");

    if (resume && resumePressureObserved.load() && flushWaitMs >= MINIMUM_FRAME_WAIT_MS) {
      break;
    }

    require(attempt + 1 < MAXIMUM_PRESSURE_FRAMES, "native fixture must establish bounded pressure");
    alternate = !alternate;

    auto next = makeFrame(false, alternate);
    const auto nextEncoded = encodeDiff(writer.committedFrame(), next, FRAME_OUTPUT_LIMIT);

    require(nextEncoded && nextEncoded->size() >= FRAME_MINIMUM_PRESSURE_BYTES, "native alternating pressure delta");
    expectedBytes = nextEncoded->size();
    require(writer.submit(std::move(next)).has_value(), "native pressure series keeps only one pending frame");
  }

  if (pressureObserver.joinable()) {
    pressureObserver.request_stop();
    pressureObserver.join();
  }

  if (observerError) {
    std::rethrow_exception(observerError);
  }

  if (resume) {
    require(resumePressureObserved.load() && written.status == TerminalStatus::PROGRESS && !written.error &&
              written.completed == expectedBytes && written.pending == 0 && flushWaitMs >= MINIMUM_FRAME_WAIT_MS &&
              !writer.pending() && !writer.poisoned() && writer.committedFrame(),
            "native pressure resume commits only completed frame");
    checkNativeFrame(native.output.value, false, alternate);
    report(resultPipe, FRAME_COMMITTED, 0, BLOCKED, elapsed, written.completed);
    require(writer.submit(makeFrame(false, alternate)).has_value() && !writer.pending() && session->pendingBytes() == 0,
            "unchanged native frame creates no terminal batch");
    require(writer.flush(std::chrono::steady_clock::now() + 1s).completed == 0,
            "unchanged native frame writes no bytes");
  } else {
    require(written.status == TerminalStatus::FAILED && written.error &&
              written.error->kind == TerminalErrorKind::OUTPUT_UNCERTAIN && written.pending > 0 &&
              flushWaitMs < MAXIMUM_FRAME_CANCELLATION_MS && writer.poisoned() && !writer.pending() &&
              !writer.committedFrame() && completedFrames > 0,
            "native pressure cancellation poisons frame belief before host resume");

    const auto rejectedPoisoned = writer.submit(std::move(blockedFrame));

    require(!rejectedPoisoned && rejectedPoisoned.error() == FrameError::POISONED,
            "native uncertain writer rejects later frames");

    const auto pendingNative = session->pendingBytes();
    const auto refusedRetry = writer.flush(std::chrono::steady_clock::now() + 100ms);

    require(refusedRetry.status == TerminalStatus::FAILED && refusedRetry.error &&
              refusedRetry.error->kind == TerminalErrorKind::CLOSED && session->pendingBytes() == pendingNative,
            "poisoned writer rejects retry without forwarding to uncertain session");
    report(resultPipe, FRAME_POISONED, 0, BLOCKED, elapsed, written.completed);
  }

  require(session->close().has_value(), "frame transport close joins old writer before recovery");
  native.checkRestored();
  report(resultPipe, FRAME_JOINED, 0, BLOCKED | RESTORED, static_cast<DWORD>(GetTickCount64() - start));

  if (!resume) {
    auto freshSession = TerminalSession::open(native.handles(), options);

    require(freshSession.has_value(), "frame recovery opens fresh session after join");

    TerminalFrameOutput freshOutput(*freshSession);
    FrameWriter recovered(freshOutput, FRAME_WRITER_LIMITS);

    require(recovered.submit(makeFrame(true)).has_value() && !recovered.committedFrame(),
            "new writer begins full CAN recovery without inherited belief");

    const auto completed = recovered.flush(std::chrono::steady_clock::now() + 4s);

    require(completed.status == TerminalStatus::PROGRESS && !completed.error && completed.completed > 0 &&
              completed.pending == 0 && recovered.committedFrame() && !recovered.pending(),
            "fresh native full repaint completes and commits");
    checkNativeFrame(native.output.value, true);
    require(freshSession->close().has_value(), "recovered frame closes normally");
    native.checkRestored();
    report(resultPipe, FRAME_RECOVERED, 0, RESTORED, static_cast<DWORD>(GetTickCount64() - start), completed.completed);
  }

  report(resultPipe, PRESSURE_DONE, 0, BLOCKED | RESTORED, static_cast<DWORD>(GetTickCount64() - start),
         written.completed);

  return 0;
}

int nativeSmoke(const wchar_t *reportPath) {
  Handle outputReport(
    CreateFileW(reportPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));

  if (outputReport.value == INVALID_HANDLE_VALUE) {
    return 2;
  }

  int status = 0;
  std::string detail;

  try {
    NativeConsole native;
    auto session = TerminalSession::open(native.handles());

    require(session.has_value(), "native smoke open");
    require(session->size().has_value(), "native smoke viewport");

    const std::string sample = "Portals native session: \xd0\x96 \xf0\x9f\x98\x80\r\n";

    require(session->submit(sample).has_value(), "native smoke submit");

    const auto written = session->flush(std::chrono::steady_clock::now() + 1s);

    require(written.status == TerminalStatus::PROGRESS && written.completed == sample.size(), "native smoke output");
    require(session->close().has_value(), "native smoke close");
    native.checkRestored();
    {
      auto exceptional = TerminalSession::open(native.handles());

      require(exceptional.has_value(), "native smoke exception open");

      try {
        TerminalSession owner = std::move(*exceptional);

        throw std::runtime_error("intentional caught fixture exception");
      } catch (const std::runtime_error &) {
      }
    }

    native.checkRestored();
    detail = "PASS: native public TerminalSession Unicode output, normal close, caught-error unwind; "
             "exact input/output modes and both code pages restored. No input consumed or injected.\n";
  } catch (const std::exception &error) {
    status = 1;
    detail = std::string("FAIL: ") + error.what() + "\n";
  }

  DWORD written = 0;

  if (!WriteFile(outputReport.value, detail.data(), static_cast<DWORD>(detail.size()), &written, nullptr) ||
      written != detail.size()) {
    return 2;
  }

  return status;
}

// Only fixture-owned pipe bytes are polled. The thread never blocks in ReadFile.
void drain(HANDLE output, std::atomic<bool> &enabled, std::atomic<bool> &stop, std::atomic<std::uint64_t> &total) {
  char buffer[16384];

  while (!stop.load()) {
    DWORD available = 0;

    if (enabled.load()) {
      if (!PeekNamedPipe(output, nullptr, 0, nullptr, &available, nullptr)) {
        break;
      }

      if (available != 0) {
        DWORD count = 0;
        const DWORD requested = available < sizeof(buffer) ? available : static_cast<DWORD>(sizeof(buffer));

        if (!ReadFile(output, buffer, requested, &count, nullptr)) {
          break;
        }

        total.fetch_add(count);

        continue;
      }
    }

    Sleep(1);
  }
}

struct Session {
  Handle inputRead, inputWrite, outputRead, outputWrite, resultRead, resultWrite;
  Handle process, processThread, job;
  HPCON console = nullptr;
  std::atomic<bool> enabled{true};
  std::atomic<bool> stop{false};
  std::atomic<std::uint64_t> drained{0};
  std::thread reader;

  ~Session() {
    enabled.store(true);

    if (process.value && WaitForSingleObject(process.value, 0) == WAIT_TIMEOUT) {
      TerminateProcess(process.value, WATCHDOG_EXIT);

      const DWORD waited = WaitForSingleObject(process.value, 2000);
      DWORD code = STILL_ACTIVE;

      GetExitCodeProcess(process.value, &code);
      std::printf("cleanup child_pid=%lu waited=%lu child_exit=%lu\n", GetProcessId(process.value), waited, code);
      std::fflush(stdout);
    }

    inputWrite.reset();

    if (console) {
      ClosePseudoConsole(console);
    }

    stop.store(true);

    if (reader.joinable()) {
      reader.join();
    }
  }
};

struct ConsoleScreen {
  COORD viewport{};
  COORD cursor{};
  WORD attributes{};
  std::vector<CHAR_INFO> cells;
};

ConsoleScreen snapshotScreen(HANDLE output) {
  CONSOLE_SCREEN_BUFFER_INFO info{};

  require(GetConsoleScreenBufferInfo(output, &info), "snapshot active caller buffer");

  ConsoleScreen result{{static_cast<SHORT>(info.srWindow.Right - info.srWindow.Left + 1),
                        static_cast<SHORT>(info.srWindow.Bottom - info.srWindow.Top + 1)},
                       info.dwCursorPosition,
                       info.wAttributes,
                       {}};

  result.cells.resize(static_cast<std::size_t>(result.viewport.X) * result.viewport.Y);

  SMALL_RECT rectangle = info.srWindow;

  require(ReadConsoleOutputW(output, result.cells.data(), result.viewport, {0, 0}, &rectangle),
          "snapshot active caller cells");

  return result;
}

void waitViewport(HANDLE output, COORD size) {
  const auto deadline = GetTickCount64() + 2000;

  do {
    CONSOLE_SCREEN_BUFFER_INFO info{};

    require(GetConsoleScreenBufferInfo(output, &info), "resized viewport query");

    if (info.srWindow.Right - info.srWindow.Left + 1 == size.X &&
        info.srWindow.Bottom - info.srWindow.Top + 1 == size.Y) {
      return;
    }

    Sleep(1);
  } while (GetTickCount64() < deadline);

  require(false, "host ResizePseudoConsole must reach the child");
}

int screenResizeCase(HANDLE resultPipe, const wchar_t *kind) {
  NativeConsole native;
  constexpr COORD initial{80, 25};
  constexpr COORD position{77, 23};
  constexpr WORD attributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN;
  const COORD target = wcscmp(kind, L"screen-resize-smaller") == 0 ? COORD{40, 12} : COORD{100, 35};
  std::vector<CHAR_INFO> original(initial.X * initial.Y);

  for (std::size_t index = 0; index < original.size(); ++index) {
    original[index].Char.UnicodeChar = static_cast<wchar_t>(L'A' + index % 26);
    original[index].Attributes = attributes;
  }

  auto seed = [&] {
    SMALL_RECT rectangle{0, 0, initial.X - 1, initial.Y - 1};

    require(WriteConsoleOutputW(native.output.value, original.data(), initial, {0, 0}, &rectangle),
            "seed resize caller screen");
    require(SetConsoleCursorPosition(native.output.value, position), "seed cursor outside smaller viewport");
    require(SetConsoleTextAttribute(native.output.value, attributes), "seed resize caller rendition");
  };
  auto directWrite = [&](std::string_view bytes) {
    DWORD amount = 0;

    require(WriteFile(native.output.value, bytes.data(), static_cast<DWORD>(bytes.size()), &amount, nullptr) &&
              amount == bytes.size(),
            "control VT1049 write");
  };

  constexpr std::string_view FRAME = "\x1b[2J\x1b[1;1H\x1b[31;42mAlerts\r\nRun\r\nQuit\x1b[1;1H";

  // Establish the expected reflow/crop/cursor result from the actual host's plain VT1049 lifecycle.
  seed();
  require(SetConsoleMode(native.output.value,
                         native.outputMode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING),
          "enable control VT output");
  directWrite("\x1b[?1049h");
  directWrite(FRAME);
  report(resultPipe, SCREEN_RESIZE_READY);
  waitViewport(native.output.value, target);
  directWrite("\x1b[?1049l");

  const auto expected = snapshotScreen(native.output.value);

  require(SetConsoleMode(native.output.value, native.outputMode), "restore control output mode");
  report(resultPipe, SCREEN_RESET_READY);
  waitViewport(native.output.value, initial);
  seed();

  auto terminal = openNativeApplicationTerminal(native.handles(), {}, 2s, false);

  require(terminal.has_value(), "managed resized screen opens");
  require((*terminal)->submit(std::string(FRAME)).has_value(), "managed resize frame submits");

  const auto frame = (*terminal)->flush(std::chrono::steady_clock::now() + 2s, {});

  require(!frame.error && frame.pending == 0, "managed resize frame flushes");
  report(resultPipe, SCREEN_RESIZE_READY);
  waitViewport(native.output.value, target);
  require((*terminal)->close().has_value(), "managed resize screen closes without stale cursor error");
  native.checkRestored();
  report(resultPipe, SCREEN_MODES_RESTORED, 0, RESTORED, 0, 0, "OS modes/codepages restored");

  const auto actual = snapshotScreen(native.output.value);

  require(actual.viewport.X == target.X && actual.viewport.Y == target.Y, "exit preserves user's new viewport");
  require(actual.viewport.X == expected.viewport.X && actual.viewport.Y == expected.viewport.Y &&
            actual.cursor.X == expected.cursor.X && actual.cursor.Y == expected.cursor.Y &&
            actual.attributes == expected.attributes && actual.cells.size() == expected.cells.size() &&
            std::equal(actual.cells.begin(), actual.cells.end(), expected.cells.begin(),
                       [](const CHAR_INFO &a, const CHAR_INFO &b) {
                         return a.Char.UnicodeChar == b.Char.UnicodeChar && a.Attributes == b.Attributes;
                       }),
          "managed resize exit matches host primary-buffer restoration without TUI residue");
  require(actual.cursor.X >= 0 && actual.cursor.X < target.X && actual.cursor.Y >= 0 && actual.cursor.Y < target.Y,
          "restored cursor remains inside current viewport");
  report(resultPipe, PRESSURE_DONE, 0, RESTORED | SCREEN_RESTORED, 0, actual.cells.size(),
         "resized primary matches host; new geometry preserved");

  return 0;
}

int screenRestoreCase(HANDLE resultPipe, const wchar_t *kind) {
  NativeConsole native;

  require(SetStdHandle(STD_INPUT_HANDLE, native.input.value) && SetStdHandle(STD_OUTPUT_HANDLE, native.output.value),
          "bind application standard handles to its own hidden ConPTY");

  CONSOLE_SCREEN_BUFFER_INFO before{};

  require(GetConsoleScreenBufferInfo(native.output.value, &before), "screen snapshot");

  constexpr COORD extent{80, 25};
  constexpr COORD position{7, 9};
  constexpr WORD attributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN;
  std::vector<CHAR_INFO> original(extent.X * extent.Y);

  for (std::size_t index = 0; index < original.size(); ++index) {
    original[index].Char.UnicodeChar = static_cast<wchar_t>(L'A' + index % 26);
    original[index].Attributes = attributes;
  }

  SMALL_RECT rectangle{0, 0, extent.X - 1, extent.Y - 1};

  require(WriteConsoleOutputW(native.output.value, original.data(), extent, {0, 0}, &rectangle), "seed caller screen");
  require(SetConsoleCursorPosition(native.output.value, position), "seed caller cursor");
  require(SetConsoleTextAttribute(native.output.value, attributes), "seed caller rendition");

  CONSOLE_CURSOR_INFO cursorBefore{};

  require(GetConsoleCursorInfo(native.output.value, &cursorBefore), "caller cursor visibility");

  const bool mainScreen = wcscmp(kind, L"screen-main") == 0;
  const bool slowDrain = wcscmp(kind, L"screen-uncertain-slow-drain") == 0;
  const bool delayedHost = wcscmp(kind, L"screen-uncertain-delayed-host") == 0;
  const bool uncertain = wcscmp(kind, L"screen-uncertain") == 0 || slowDrain || delayedHost;
  const bool application = wcscmp(kind, L"screen-application") == 0 || wcscmp(kind, L"screen-failure") == 0;

  if (application) {
    struct Observation {
      std::atomic<bool> flushed{};
      bool fail{};
    } observation;

    observation.fail = wcscmp(kind, L"screen-failure") == 0;

    ApplicationOptions options;

    options.nativeMouseReporting = false;
    options.observer = {[](void *context, ApplicationPhase phase, std::chrono::nanoseconds) {
                          auto &observed = *static_cast<Observation *>(context);

                          if (phase == ApplicationPhase::FLUSH) {
                            observed.flushed.store(true);

                            if (observed.fail) {
                              throw std::runtime_error("failure after gallery frame flush");
                            }
                          }
                        },
                        &observation};

    auto created = Application::create(options);

    require(created.has_value(), "gallery application creates");

    auto &app = **created;
    gallery::Form form(app);
    std::jthread stopper([&](std::stop_token stop) {
      while (!stop.stop_requested() && !observation.flushed.load()) {
        Sleep(1);
      }

      if (observation.flushed.load()) {
        app.requestStop();
      }
    });

    const auto ran = form.run();

    require(observation.flushed.load(), "actual gallery frame reached console");
    require(observation.fail
              ? !ran && ran.error().application && ran.error().application->phase == ApplicationPhase::FLUSH
              : ran.has_value(),
            "native Application run preserves normal/error outcome");
  } else {
    TerminalOptions terminalOptions;

    terminalOptions.max_output_bytes = PRESSURE_BYTES + 2;

    auto terminal =
      openNativeApplicationTerminal(native.handles(), terminalOptions, SCREEN_MODE_TIMEOUT, false,
                                    mainScreen ? NativeScreenMode::MAIN_SCREEN : NativeScreenMode::ALTERNATE_SCREEN);

    require(terminal.has_value(), "native application opens");
    require((*terminal)->submit("\x1b[2J\x1b[1;1H\x1b[31;42mAlerts\r\nRun\r\nQuit\x1b[1;1H\x1b[?25l").has_value(),
            "paint gallery-like frame");

    const auto written = (*terminal)->flush(std::chrono::steady_clock::now() + 2s, {});

    require(!written.error && written.pending == 0, "gallery frame flush");

    if (uncertain) {
      report(resultPipe, PRESSURE_READY);
      require((*terminal)->submit(std::string(PRESSURE_BYTES, '!') + "\x1b[").has_value(), "uncertain gallery frame");

      const auto flushStarted = GetTickCount64();
      const auto cancelled = (*terminal)->flush(std::chrono::steady_clock::now() + 100ms, {});
      const auto flushElapsed = static_cast<DWORD>(GetTickCount64() - flushStarted);

      require(cancelled.error && cancelled.error->kind == TerminalErrorKind::OUTPUT_UNCERTAIN,
              "issued Windows frame cancellation is uncertain");
      require(flushElapsed < MAXIMUM_FRAME_CANCELLATION_MS,
              "uncertain screen flush keeps its public deadline while host is paused");
      report(resultPipe, CANCEL_RESULT, 0, BLOCKED, flushElapsed, cancelled.pending,
             "public flush returned before host drain");
    }

    const auto closeStarted = GetTickCount64();
    const auto closed = (*terminal)->close();
    const auto closeElapsed = static_cast<DWORD>(GetTickCount64() - closeStarted);

    require(uncertain ? !closed && closed.error().kind == TerminalErrorKind::OUTPUT_UNCERTAIN : closed.has_value(),
            "screen close preserves uncertain-write failure");

    if (uncertain) {
      report(resultPipe, CANCEL_RESULT, static_cast<DWORD>(closed.error().restoration_code), 0, closeElapsed,
             static_cast<std::uint64_t>(closed.error().native_code), "close native/restoration error codes");
    }

    if (slowDrain || delayedHost) {
      require(closeElapsed > std::chrono::duration_cast<std::chrono::milliseconds>(SCREEN_MODE_TIMEOUT).count(),
              "host synchronization exceeds the ordinary cleanup budget");
      report(resultPipe, WRITER_COMPLETE, 0, BLOCKED, closeElapsed, 0, "write and host settled after delayed drain");
    }

    require((*terminal)->close().has_value(), "application screen closes twice");
  }
  native.checkRestored();
  report(resultPipe, SCREEN_MODES_RESTORED, 0, RESTORED, 0, 0, "OS modes/codepages restored");

  std::vector<CHAR_INFO> restored(original.size());

  rectangle = {0, 0, extent.X - 1, extent.Y - 1};
  require(ReadConsoleOutputW(native.output.value, restored.data(), extent, {0, 0}, &rectangle),
          "read caller screen after close");
  const bool sameCells =
    std::equal(original.begin(), original.end(), restored.begin(), [](const CHAR_INFO &a, const CHAR_INFO &b) {
      return a.Char.UnicodeChar == b.Char.UnicodeChar && a.Attributes == b.Attributes;
    });

  if (!mainScreen && !sameCells) {
    const auto mismatch =
      std::mismatch(original.begin(), original.end(), restored.begin(), [](const CHAR_INFO &a, const CHAR_INFO &b) {
        return a.Char.UnicodeChar == b.Char.UnicodeChar && a.Attributes == b.Attributes;
      });
    char detail[128];

    std::snprintf(detail, sizeof(detail), "first cell expected U+%04x attr=%u, actual U+%04x attr=%u",
                  static_cast<unsigned>(mismatch.first->Char.UnicodeChar), mismatch.first->Attributes,
                  static_cast<unsigned>(mismatch.second->Char.UnicodeChar), mismatch.second->Attributes);
    report(resultPipe, CHILD_FAILURE, 0, 0, 0, static_cast<std::uint64_t>(mismatch.first - original.begin()), detail);
  }

  require(mainScreen ? !sameCells : sameCells,
          "application close must restore caller screen instead of leaving gallery cells");

  if (mainScreen) {
    report(resultPipe, PRESSURE_DONE, 0, RESTORED, 0, original.size(), "MAIN explicitly retains painted caller screen");

    return 0;
  }

  CONSOLE_SCREEN_BUFFER_INFO after{};

  require(GetConsoleScreenBufferInfo(native.output.value, &after), "caller cursor after close");
  require(after.dwCursorPosition.X == position.X && after.dwCursorPosition.Y == position.Y,
          "application close must restore caller cursor");
  require(after.wAttributes == attributes, "application close must restore caller rendition");

  CONSOLE_CURSOR_INFO cursorAfter{};

  require(GetConsoleCursorInfo(native.output.value, &cursorAfter) && cursorAfter.bVisible == cursorBefore.bVisible &&
            cursorAfter.dwSize == cursorBefore.dwSize,
          "application close must restore caller cursor visibility");
  report(resultPipe, PRESSURE_DONE, 0, RESTORED | SCREEN_RESTORED, 0, original.size(),
         "caller screen/cursor/rendition restored");

  return 0;
}

void launch(Session &session, const wchar_t *kind) {
  HANDLE read = nullptr;
  HANDLE write = nullptr;

  require(CreatePipe(&read, &write, nullptr, 4096), "input pipe");
  session.inputRead.reset(read);
  session.inputWrite.reset(write);
  require(CreatePipe(&read, &write, nullptr, 4096), "output pipe");
  session.outputRead.reset(read);
  session.outputWrite.reset(write);

  SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};

  require(CreatePipe(&read, &write, &security, 4096), "result pipe");
  session.resultRead.reset(read);
  session.resultWrite.reset(write);
  require(SetHandleInformation(read, HANDLE_FLAG_INHERIT, 0), "result inheritance");

  const HRESULT created = CreatePseudoConsole(isFrameCase(kind) ? FRAME_VIEWPORT : COORD{80, 25},
                                              session.inputRead.value, session.outputWrite.value, 0, &session.console);

  require(SUCCEEDED(created), "CreatePseudoConsole");
  session.enabled.store(wcscmp(kind, L"input") == 0 ||
                        (wcsncmp(kind, L"screen-", 7) == 0 && wcsncmp(kind, L"screen-uncertain", 16) != 0));
  session.reader = std::thread(drain, session.outputRead.value, std::ref(session.enabled), std::ref(session.stop),
                               std::ref(session.drained));
  session.job.reset(CreateJobObjectW(nullptr, nullptr));
  require(session.job.value != nullptr, "CreateJobObjectW");

  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit{};

  limit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  require(SetInformationJobObject(session.job.value, JobObjectExtendedLimitInformation, &limit, sizeof(limit)),
          "job kill-on-close");

  SIZE_T bytes = 0;

  InitializeProcThreadAttributeList(nullptr, 2, 0, &bytes);

  std::vector<unsigned char> storage(bytes);
  auto attributes = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(storage.data());

  require(InitializeProcThreadAttributeList(attributes, 2, 0, &bytes), "attributes");

  struct AttributeCleanup {
    PPROC_THREAD_ATTRIBUTE_LIST value;

    ~AttributeCleanup() {
      DeleteProcThreadAttributeList(value);
    }
  } cleanup{attributes};

  require(UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, session.console,
                                    sizeof(session.console), nullptr, nullptr),
          "pseudoconsole attribute");
  require(UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &session.resultWrite.value,
                                    sizeof(HANDLE), nullptr, nullptr),
          "result handle attribute");

  wchar_t path[32768];

  require(GetModuleFileNameW(nullptr, path, 32768) != 0, "self executable path");

  std::wstring command = L"\"" + std::wstring(path) + L"\" --child " + kind + L" " +
                         std::to_wstring(reinterpret_cast<std::uintptr_t>(session.resultWrite.value));
  STARTUPINFOEXW startup{};

  startup.StartupInfo.cb = sizeof(startup);
  startup.lpAttributeList = attributes;

  PROCESS_INFORMATION process{};

  require(CreateProcessW(path, command.data(), nullptr, nullptr, TRUE, EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED,
                         nullptr, nullptr, &startup.StartupInfo, &process),
          "CreateProcessW child");
  session.process.reset(process.hProcess);
  session.processThread.reset(process.hThread);
  require(AssignProcessToJobObject(session.job.value, session.process.value), "AssignProcessToJobObject");
  require(ResumeThread(session.processThread.value) != static_cast<DWORD>(-1), "ResumeThread");
  std::printf("launched case=%ls host_pid=%lu child_pid=%lu\n", kind, GetCurrentProcessId(), process.dwProcessId);
  std::fflush(stdout);
  session.inputRead.reset();
  session.outputWrite.reset();
  session.resultWrite.reset();
}

bool hostCase(const wchar_t *kind) {
  Session session;

  launch(session, kind);

  const bool frameResume = wcscmp(kind, L"frame-resume") == 0;
  const bool frameCancellation = wcscmp(kind, L"frame-cancel") == 0 || wcscmp(kind, L"frame-stop") == 0;
  const ULONGLONG deadline = GetTickCount64() + (wcscmp(kind, L"hang") == 0 ? 500 : CHILD_DEADLINE_MS);
  ULONGLONG pressureStarted = 0;
  ULONGLONG idleResizeStarted = 0;
  bool finalSeen = false;
  bool pressureBlocked = false;
  bool framePressureSeen = false;
  DWORD childExit = STILL_ACTIVE;

  while (GetTickCount64() < deadline) {
    DWORD available = 0;

    while (PeekNamedPipe(session.resultRead.value, nullptr, 0, nullptr, &available, nullptr) &&
           available >= sizeof(Message)) {
      Message message{};
      DWORD count = 0;

      require(ReadFile(session.resultRead.value, &message, sizeof(message), &count, nullptr) &&
                count == sizeof(message),
              "host result read");
      std::printf("case=%ls stage=%lu error=%lu flags=%lu elapsed_ms=%lu amount=%llu detail=%s\n", kind, message.stage,
                  message.error, message.flags, message.elapsedMs, static_cast<unsigned long long>(message.amount),
                  message.detail);
      std::fflush(stdout);

      if (message.stage == SCREEN_RESIZE_READY) {
        const COORD size = wcscmp(kind, L"screen-resize-smaller") == 0 ? COORD{40, 12} : COORD{100, 35};

        require(SUCCEEDED(ResizePseudoConsole(session.console, size)), "host screen lifecycle resize");
      }

      if (message.stage == SCREEN_RESET_READY) {
        require(SUCCEEDED(ResizePseudoConsole(session.console, {80, 25})),
                "reset control viewport before managed resize");
      }

      if (message.stage == RESIZE_READY) {
        require(SUCCEEDED(ResizePseudoConsole(session.console, {95, 31})), "resize without text input");
      }

      if (message.stage == IDLE_RESIZE_READY) {
        idleResizeStarted = GetTickCount64();
      }

      if (message.stage == INPUT_READY) {
        constexpr std::string_view UNICODE_AND_CTRL_C =
          "\xd0\x96"
          "e\xcc\x81\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x92\xbb\xe7\x95\x8c\x03";
        DWORD written = 0;

        require(WriteFile(session.inputWrite.value, UNICODE_AND_CTRL_C.data(),
                          static_cast<DWORD>(UNICODE_AND_CTRL_C.size()), &written, nullptr) &&
                  written == UNICODE_AND_CTRL_C.size(),
                "host VT input write");
        require(SUCCEEDED(ResizePseudoConsole(session.console, {91, 37})), "ResizePseudoConsole");
      }

      if (message.stage == PRESSURE_READY && !isFrameCase(kind)) {
        pressureStarted = GetTickCount64();
      }

      if (message.stage == FRAME_POISONED) {
        require(frameCancellation && (message.flags & BLOCKED) != 0 && !session.enabled.load(),
                "host observes cancelled frame before resuming");
        session.enabled.store(true);
      }

      if (message.stage == FRAME_PRESSURE_OBSERVED) {
        require(frameResume && message.elapsedMs >= MINIMUM_FRAME_WAIT_MS && (message.flags & BLOCKED) != 0 &&
                  !session.enabled.load(),
                "host observes incomplete frame before resuming");
        framePressureSeen = true;
        session.enabled.store(true);
      }

      if (message.stage == INPUT_DONE || message.stage == PRESSURE_DONE) {
        finalSeen = true;
        pressureBlocked = (message.flags & BLOCKED) != 0;
      }
    }

    if (idleResizeStarted != 0 && GetTickCount64() - idleResizeStarted >= 100) {
      require(SUCCEEDED(ResizePseudoConsole(session.console, {97, 33})), "delayed idle host resize");
      idleResizeStarted = 0;
    }

    if (pressureStarted != 0) {
      DWORD pause = MAXIMUM_DRAIN_PAUSE_MS;

      if (wcscmp(kind, L"screen-uncertain-slow-drain") == 0 || wcscmp(kind, L"screen-uncertain-delayed-host") == 0) {
        pause = SLOW_DRAIN_PAUSE_MS;
      } else if (wcscmp(kind, L"resume") == 0) {
        pause = 700;
      }

      if (GetTickCount64() - pressureStarted >= pause) {
        session.enabled.store(true);
      }
    }

    if (WaitForSingleObject(session.process.value, 0) == WAIT_OBJECT_0) {
      require(GetExitCodeProcess(session.process.value, &childExit), "child exit");

      if (finalSeen || childExit != 0) {
        break;
      }
    }

    Sleep(5);
  }

  session.enabled.store(true);
  std::printf("case=%ls child_exit=%lu final_seen=%d pressure_blocked=%d drained=%llu\n", kind, childExit, finalSeen,
              pressureBlocked, static_cast<unsigned long long>(session.drained.load()));

  return childExit == 0 && finalSeen && (!frameResume || framePressureSeen);
}
} // namespace

int wmain(int argc, wchar_t **argv) {
  if (argc == 3 && wcscmp(argv[1], L"--native-smoke") == 0) {
    return nativeSmoke(argv[2]);
  }

  if (argc == 4 && wcscmp(argv[1], L"--child") == 0) {
    const HANDLE result = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(_wcstoui64(argv[3], nullptr, 10)));

    try {
      std::optional<UncancellableDriver> uncancellable;

      if (wcscmp(argv[2], L"screen-uncertain-slow-drain") == 0) {
        uncancellable.emplace();
      }

      if (wcscmp(argv[2], L"hang") == 0) {
        Sleep(30000);

        return 0;
      }

      if (wcsncmp(argv[2], L"screen-resize-", 14) == 0) {
        return screenResizeCase(result, argv[2]);
      }

      if (wcsncmp(argv[2], L"screen-", 7) == 0) {
        return screenRestoreCase(result, argv[2]);
      }

      if (isFrameCase(argv[2])) {
        return frameCase(result, argv[2]);
      }

      return wcscmp(argv[2], L"input") == 0 ? inputCase(result) : pressureCase(result, argv[2]);
    } catch (const std::exception &error) {
      report(result, CHILD_FAILURE, GetLastError(), 0, 0, 0, error.what());

      return 1;
    }
  }

  try {
    if (argc == 2 && wcscmp(argv[1], L"--screen-uncertain-slow-drain") == 0) {
      return hostCase(L"screen-uncertain-slow-drain") ? 0 : 1;
    }

    if (argc == 2 && wcscmp(argv[1], L"--screen-uncertain-delayed-host") == 0) {
      return hostCase(L"screen-uncertain-delayed-host") ? 0 : 1;
    }

    if (argc == 2 && wcscmp(argv[1], L"--screen-restore") == 0) {
      const bool restored = hostCase(L"screen-restore");
      const bool main = hostCase(L"screen-main");
      const bool application = hostCase(L"screen-application");
      const bool failure = hostCase(L"screen-failure");
      const bool uncertain = hostCase(L"screen-uncertain");
      const bool slowUncertain = hostCase(L"screen-uncertain-slow-drain");
      const bool delayedHost = hostCase(L"screen-uncertain-delayed-host");

      const bool smaller = hostCase(L"screen-resize-smaller");
      const bool larger = hostCase(L"screen-resize-larger");

      return restored && main && application && failure && uncertain && slowUncertain && delayedHost && smaller &&
                 larger
               ? 0
               : 1;
    }

    if (argc == 2 && wcscmp(argv[1], L"--child-watchdog") == 0) {
      return hostCase(L"hang") ? 0 : WATCHDOG_EXIT;
    }

    if (argc == 2 && wcscmp(argv[1], L"--hold-host") == 0) {
      Session session;

      launch(session, L"hang");
      Sleep(30000);

      return 0;
    }

    const bool screen = hostCase(L"screen-restore") && hostCase(L"screen-main") && hostCase(L"screen-application") &&
                        hostCase(L"screen-failure") && hostCase(L"screen-uncertain") &&
                        hostCase(L"screen-uncertain-slow-drain") && hostCase(L"screen-uncertain-delayed-host") &&
                        hostCase(L"screen-resize-smaller") && hostCase(L"screen-resize-larger");
    const bool input = hostCase(L"input");
    const bool cancel = hostCase(L"cancel");
    const bool resume = hostCase(L"resume");
    const bool stopped = hostCase(L"stop");
    const bool frameResume = hostCase(L"frame-resume");
    const bool frameCancel = hostCase(L"frame-cancel");
    const bool frameStop = hostCase(L"frame-stop");

    std::printf("summary input=%d cancel=%d resume=%d stopped=%d frame_resume=%d frame_cancel=%d frame_stop=%d\n",
                input, cancel, resume, stopped, frameResume, frameCancel, frameStop);

    return screen && input && cancel && resume && stopped && frameResume && frameCancel && frameStop ? 0 : 1;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "host failure: %s\n", error.what());

    return 2;
  }
}
