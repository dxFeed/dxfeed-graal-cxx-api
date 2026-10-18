// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/input.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace portals {
namespace {
constexpr unsigned char ESCAPE_BYTE = 0x1b;
constexpr std::string_view PASTE_END = "\x1b[201~";

constexpr unsigned char CONTROL_BYTE_END = 0x20;
constexpr unsigned char DELETE_BYTE = 0x7f;
constexpr unsigned char ASCII_END = 0x80;
constexpr unsigned char CSI_FINAL_BEGIN = 0x40;
constexpr unsigned char CSI_FINAL_END = 0x7e;
constexpr unsigned char CSI_PARAMETER_BEGIN = 0x30;
constexpr unsigned char CONTROL_CHARACTER_OFFSET = 0x40;
constexpr unsigned char BELL_BYTE = 7;

// ESC plus introducer precede the body; the final byte completes the sequence.
constexpr std::size_t SEQUENCE_PREFIX_BYTES = 2;
constexpr std::size_t MIN_SEQUENCE_BYTES = SEQUENCE_PREFIX_BYTES + 1;
constexpr std::size_t MAX_PARAMETERS = decltype(InputProbeCandidate::values){}.size();
constexpr std::size_t MAX_PARAMETER_DIGITS = std::numeric_limits<std::uint32_t>::digits10 + 1;
constexpr std::uint32_t DECIMAL_BASE = 10;
constexpr std::uint32_t PASTE_BEGIN_PARAMETER = 200;

// xterm modifiers encode shift, alt and control in the low three bits.
constexpr std::uint32_t SHIFT_MASK = 1;
constexpr std::uint32_t ALT_MASK = 2;
constexpr std::uint32_t CONTROL_MASK = 4;
constexpr std::uint32_t MODIFIER_MASK = SHIFT_MASK | ALT_MASK | CONTROL_MASK;
constexpr std::uint32_t MAX_KEY_MODIFIER = MODIFIER_MASK + 1;

constexpr std::size_t MOUSE_PARAMETER_COUNT = 3;
constexpr std::uint32_t MOUSE_BUTTON_MASK = 3;
constexpr std::uint32_t MOUSE_NO_BUTTON = 3;
constexpr unsigned MOUSE_MODIFIER_SHIFT = 2;
constexpr std::uint32_t MOUSE_MOTION_MASK = 32;
constexpr std::uint32_t MOUSE_WHEEL_MASK = 64;
constexpr std::uint32_t MAX_MOUSE_CODE = 127;

constexpr unsigned char UTF8_LEAD_BEGIN = 0xc2;
constexpr unsigned char UTF8_LEAD_END = 0xf4;
constexpr unsigned char UTF8_THREE_BYTE_BEGIN = 0xe0;
constexpr unsigned char UTF8_FOUR_BYTE_BEGIN = 0xf0;
constexpr unsigned char UTF8_TWO_BYTE_PAYLOAD_MASK = 0x1f;
constexpr unsigned char UTF8_THREE_BYTE_PAYLOAD_MASK = 0x0f;
constexpr unsigned char UTF8_FOUR_BYTE_PAYLOAD_MASK = 0x07;
constexpr unsigned char UTF8_CONTINUATION_CLASS_MASK = 0xc0;
constexpr unsigned char UTF8_CONTINUATION_CLASS = 0x80;
constexpr unsigned char UTF8_CONTINUATION_PAYLOAD_MASK = 0x3f;
constexpr unsigned UTF8_CONTINUATION_BITS = 6;
constexpr char32_t UTF8_TWO_BYTE_MIN_SCALAR = 0x80;
constexpr char32_t UTF8_THREE_BYTE_MIN_SCALAR = 0x800;
constexpr char32_t UTF8_FOUR_BYTE_MIN_SCALAR = 0x10000;
constexpr char32_t MAX_UNICODE_SCALAR = 0x10ffff;
constexpr char32_t SURROGATE_BEGIN = 0xd800;
constexpr char32_t SURROGATE_END = 0xdfff;

InputTime after(InputTime now, InputTime interval) noexcept {
  return now > std::numeric_limits<InputTime>::max() - interval ? std::numeric_limits<InputTime>::max()
                                                                : now + interval;
}

InputModifiers modifiers(std::uint32_t bits) noexcept {
  return {(bits & SHIFT_MASK) != 0, (bits & ALT_MASK) != 0, (bits & CONTROL_MASK) != 0, false};
}

std::optional<InputKey> letterKey(char final) noexcept {
  switch (final) {
  case 'A':
    return InputKey::UP;
  case 'B':
    return InputKey::DOWN;
  case 'C':
    return InputKey::RIGHT;
  case 'D':
    return InputKey::LEFT;
  case 'H':
    return InputKey::HOME;
  case 'F':
    return InputKey::END;
  case 'P':
    return InputKey::F1;
  case 'Q':
    return InputKey::F2;
  case 'R':
    return InputKey::F3;
  case 'S':
    return InputKey::F4;
  default:
    return {};
  }
}

std::optional<InputKey> tildeKey(std::uint32_t number) noexcept {
  // xterm's CSI Ps ~ table includes historical Home/End aliases and gaps between function keys.
  switch (number) {
  case 1:
  case 7:
    return InputKey::HOME;
  case 2:
    return InputKey::INSERT;
  case 3:
    return InputKey::DELETE_KEY;
  case 4:
  case 8:
    return InputKey::END;
  case 5:
    return InputKey::PAGE_UP;
  case 6:
    return InputKey::PAGE_DOWN;
  case 11:
    return InputKey::F1;
  case 12:
    return InputKey::F2;
  case 13:
    return InputKey::F3;
  case 14:
    return InputKey::F4;
  case 15:
    return InputKey::F5;
  case 17:
    return InputKey::F6;
  case 18:
    return InputKey::F7;
  case 19:
    return InputKey::F8;
  case 20:
    return InputKey::F9;
  case 21:
    return InputKey::F10;
  case 23:
    return InputKey::F11;
  case 24:
    return InputKey::F12;
  default:
    return {};
  }
}
} // namespace

std::expected<InputDecoder, InputError> InputDecoder::create(InputOptions options) {
  constexpr auto SEQUENCE_CAPACITY = decltype(sequence_){}.size();

  if (options.max_sequence_bytes < MIN_SEQUENCE_BYTES || options.max_sequence_bytes > SEQUENCE_CAPACITY ||
      options.max_parameters == 0 || options.max_parameters > MAX_PARAMETERS || options.max_digits == 0 ||
      options.max_digits > MAX_PARAMETER_DIGITS || options.max_paste_bytes > std::string{}.max_size() ||
      options.work_bytes == 0 || options.escape_timeout == 0 || options.sequence_timeout == 0 ||
      options.paste_timeout == 0) {
    return std::unexpected(InputError::INVALID_OPTIONS);
  }

  return InputDecoder{options};
}

void InputDecoder::reset() noexcept {
  state_ = State::GROUND;
  last_time_.reset();
  deadline_ = sequence_started_ = offset_ = start_ = 0;
  length_ = marker_ = 0;
  intermediate_ = string_osc_ = string_escape_ = alt_ = ending_ = finished_ = false;
  remaining_ = 0;
  scalar_ = minimum_scalar_ = 0;
  std::string{}.swap(paste_);
  window_ = {};
  pending_.reset();
}

std::optional<InputTime> InputDecoder::deadline() const noexcept {
  if (state_ == State::GROUND || state_ == State::FAILED || finished_) {
    return {};
  }

  return deadline_;
}

std::expected<void, InputError> InputDecoder::expectReplies(InputProbeWindow window) noexcept {
  if (state_ != State::GROUND || pending_ || ending_ || window.deadline <= window.opens_at) {
    return std::unexpected(InputError::INVALID_WINDOW);
  }

  window_ = window;

  return {};
}

InputIssue InputDecoder::issue(InputIssueKind kind) const noexcept {
  return {kind, start_, offset_};
}

void InputDecoder::begin(State state, InputTime now, InputTime duration) noexcept {
  state_ = state;
  sequence_started_ = now;
  deadline_ = after(now, duration);
}

void InputDecoder::clearSequence() noexcept {
  state_ = State::GROUND;
  length_ = 0;
  intermediate_ = false;
  remaining_ = 0;
  alt_ = false;
}

std::optional<InputEvent> InputDecoder::expire(InputTime now) {
  if (!deadline() || now < deadline_) {
    return {};
  }

  if (state_ == State::ESCAPE) {
    clearSequence();

    return KeyEvent{InputKey::ESCAPE};
  }

  if (state_ == State::UTF8) {
    const auto was_alt = alt_;
    auto error = issue(InputIssueKind::TRUNCATED);

    error.begin += was_alt ? 1 : 0;
    clearSequence();

    if (was_alt) {
      pending_ = error;

      return KeyEvent{InputKey::ESCAPE};
    }

    return error;
  }

  // Without a terminator, accepting a suffix as keys would cross the data boundary.
  state_ = State::FAILED;
  std::string{}.swap(paste_);

  return issue(InputIssueKind::TIMEOUT);
}

std::optional<InputEvent> InputDecoder::ground(unsigned char value, bool alt, InputTime now) {
  if (!alt) {
    start_ = offset_ - 1;
  }

  if (value == ESCAPE_BYTE) {
    begin(State::ESCAPE, now, options_.escape_timeout);

    return {};
  }

  const InputModifiers observed{false, alt, false, false};

  if (value < CONTROL_BYTE_END || value == DELETE_BYTE) {
    InputKey key = InputKey::CHARACTER;

    switch (value) {
    case '\t':
      key = InputKey::TAB;

      break;
    case '\r':
      key = InputKey::ENTER;

      break;
    case '\b':
    case DELETE_BYTE:
      key = InputKey::BACKSPACE;

      break;
    default:
      break;
    }

    auto mods = observed;

    mods.control = key == InputKey::CHARACTER;

    return KeyEvent{key, key == InputKey::CHARACTER ? static_cast<char32_t>(value + CONTROL_CHARACTER_OFFSET) : U'\0',
                    mods};
  }

  if (value < ASCII_END) {
    return TextInputEvent{static_cast<char32_t>(value), observed};
  }

  if (value >= UTF8_LEAD_BEGIN && value <= UTF8_LEAD_END) {
    remaining_ = value < UTF8_THREE_BYTE_BEGIN ? 1 : value < UTF8_FOUR_BYTE_BEGIN ? 2 : 3;
    scalar_ = value & (remaining_ == 1   ? UTF8_TWO_BYTE_PAYLOAD_MASK
                       : remaining_ == 2 ? UTF8_THREE_BYTE_PAYLOAD_MASK
                                         : UTF8_FOUR_BYTE_PAYLOAD_MASK);
    minimum_scalar_ = remaining_ == 1   ? UTF8_TWO_BYTE_MIN_SCALAR
                      : remaining_ == 2 ? UTF8_THREE_BYTE_MIN_SCALAR
                                        : UTF8_FOUR_BYTE_MIN_SCALAR;
    alt_ = alt;

    if (alt) {
      state_ = State::UTF8;
    } else {
      begin(State::UTF8, now, options_.sequence_timeout);
    }

    return {};
  }

  return issue(InputIssueKind::INVALID_UTF8);
}

std::optional<InputEvent> InputDecoder::control(unsigned char value, InputTime now) {
  if (value < CONTROL_BYTE_END || value > CSI_FINAL_END) {
    clearSequence();

    return issue(InputIssueKind::INVALID_SEQUENCE);
  }

  const auto final = value >= CSI_FINAL_BEGIN;

  if (state_ == State::DISCARD_CSI) {
    if (final) {
      clearSequence();
    }

    return {};
  }

  if (length_ == options_.max_sequence_bytes) {
    state_ = final ? State::GROUND : State::DISCARD_CSI;

    return issue(InputIssueKind::SEQUENCE_LIMIT);
  }

  sequence_[length_++] = static_cast<char>(value);

  if (final) {
    auto event = sequence(now);

    if (state_ != State::PASTE) {
      clearSequence();
    }

    return event;
  }

  if (value < CSI_PARAMETER_BEGIN) {
    intermediate_ = true;
  } else if (intermediate_) {
    state_ = State::DISCARD_CSI;

    return issue(InputIssueKind::INVALID_SEQUENCE);
  }

  return {};
}

// Parse the bounded numeric grammar before interpreting the final byte. A syntactic
// reply is only a candidate inside the caller-owned window, never terminal authentication.
std::optional<InputEvent> InputDecoder::sequence(InputTime now) {
  const bool ss3 = state_ == State::SS3;
  const char final = sequence_[length_ - 1];
  auto body = std::string_view{sequence_.data() + SEQUENCE_PREFIX_BYTES, length_ - MIN_SEQUENCE_BYTES};
  char prefix{};

  if (!body.empty() && (body.front() == '<' || body.front() == '?')) {
    prefix = body.front();
    body.remove_prefix(1);
  }

  if (intermediate_) {
    return issue(InputIssueKind::UNKNOWN_SEQUENCE);
  }

  std::array<std::uint32_t, MAX_PARAMETERS> values{};
  std::size_t count = 0;

  while (!body.empty()) {
    if (count == options_.max_parameters) {
      return issue(InputIssueKind::PARAMETER_LIMIT);
    }

    std::uint32_t number = 0;
    std::size_t digits = 0;

    while (!body.empty() && body.front() >= '0' && body.front() <= '9') {
      const auto digit = static_cast<std::uint32_t>(body.front() - '0');

      if (++digits > options_.max_digits) {
        return issue(InputIssueKind::PARAMETER_LIMIT);
      }

      if (number > (std::numeric_limits<std::uint32_t>::max() - digit) / DECIMAL_BASE) {
        return issue(InputIssueKind::NUMERIC_OVERFLOW);
      }

      number = number * DECIMAL_BASE + digit;
      body.remove_prefix(1);
    }

    if (digits == 0) {
      return issue(InputIssueKind::INVALID_SEQUENCE);
    }

    values[count++] = number;

    if (!body.empty()) {
      if (body.front() != ';' || body.size() == 1) {
        return issue(InputIssueKind::INVALID_SEQUENCE);
      }

      body.remove_prefix(1);
    }
  }

  const bool in_window = sequence_started_ >= window_.opens_at && now < window_.deadline;

  if (!ss3 && prefix == '?' && final == 'c' && count > 0 && in_window && window_.device_attributes) {
    window_.device_attributes = false;

    return InputProbeCandidate{InputProbeKind::PRIMARY_DEVICE_ATTRIBUTES, window_.generation, values, count};
  }

  if (!ss3 && prefix == 0 && final == 'R' && count == 2 && values[0] > 0 && values[1] > 0 && in_window &&
      window_.cursor_position) {
    window_.cursor_position = false;

    return InputProbeCandidate{InputProbeKind::CURSOR_POSITION, window_.generation, values, count};
  }

  if (!ss3 && prefix == '<' && (final == 'M' || final == 'm') && options_.sgr_mouse) {
    constexpr std::uint32_t MAX_ONE_BASED = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) + 1;

    if (count != MOUSE_PARAMETER_COUNT || values[0] > MAX_MOUSE_CODE || values[1] == 0 || values[2] == 0 ||
        values[1] > MAX_ONE_BASED || values[2] > MAX_ONE_BASED) {
      return issue(InputIssueKind::INVALID_SEQUENCE);
    }

    const auto code = values[0];
    const auto button = code & MOUSE_BUTTON_MASK;

    const bool motion = (code & MOUSE_MOTION_MASK) != 0;
    const bool wheel = (code & MOUSE_WHEEL_MASK) != 0;

    if ((wheel && (motion || final == 'm')) || (!wheel && button == MOUSE_NO_BUTTON && (!motion || final == 'm')) ||
        (motion && final == 'm')) {
      return issue(InputIssueKind::INVALID_SEQUENCE);
    }

    const auto identity = wheel ? static_cast<MouseButton>(static_cast<unsigned>(MouseButton::WHEEL_UP) + button)
                                : static_cast<MouseButton>(button);

    const auto action = wheel          ? MouseAction::SCROLL
                        : motion       ? MouseAction::MOVE
                        : final == 'm' ? MouseAction::RELEASE
                                       : MouseAction::PRESS;

    return MouseInputEvent{static_cast<std::int32_t>(values[1] - 1), static_cast<std::int32_t>(values[2] - 1), identity,
                           action, modifiers((code >> MOUSE_MODIFIER_SHIFT) & MODIFIER_MASK)};
  }

  if (prefix != 0) {
    return issue(InputIssueKind::UNKNOWN_SEQUENCE);
  }

  if (!ss3 && final == '~' && count == 1 && values[0] == PASTE_BEGIN_PARAMETER && options_.bracketed_paste) {
    paste_.clear();
    marker_ = 0;
    begin(State::PASTE, now, options_.paste_timeout);

    return {};
  }

  if (!ss3 && final == 'Z' && count == 0) {
    return KeyEvent{InputKey::TAB, 0, {true, false, false, false}};
  }

  std::optional<InputKey> key;
  std::uint32_t modifier = 1;

  if (!ss3 && final == '~' && (count == 1 || count == 2)) {
    key = tildeKey(values[0]);
    modifier = count == 2 ? values[1] : 1;
  } else if (count == 0 || (count == 1 && values[0] == 1) || (count == 2 && values[0] == 1)) {
    key = letterKey(final);
    modifier = count == 2 ? values[1] : 1;
  }

  if (key && modifier >= 1 && modifier <= MAX_KEY_MODIFIER) {
    return KeyEvent{*key, 0, modifiers(modifier - 1)};
  }

  return issue(InputIssueKind::UNKNOWN_SEQUENCE);
}

std::optional<InputEvent> InputDecoder::paste(unsigned char value) {
  // Only the exact six-byte end marker has meaning inside a paste.
  if (value == static_cast<unsigned char>(PASTE_END[marker_])) {
    if (++marker_ == PASTE_END.size()) {
      marker_ = 0;

      const auto discarded = state_ == State::DISCARD_PASTE;

      clearSequence();

      if (!discarded) {
        std::string payload;

        payload.swap(paste_);

        return PasteInputEvent{std::move(payload)};
      }
    }

    return {};
  }

  // A failed terminator prefix is literal paste data. Keep a new leading ESC as the
  // next candidate instead of interpreting any payload byte as an ordinary key.
  const auto held = marker_;

  marker_ = value == ESCAPE_BYTE ? 1 : 0;

  const auto extra = held + (marker_ == 0 ? 1 : 0);

  if (state_ == State::DISCARD_PASTE) {
    return {};
  }

  if (extra > options_.max_paste_bytes - paste_.size()) {
    std::string{}.swap(paste_);
    state_ = State::DISCARD_PASTE;

    return issue(InputIssueKind::PASTE_LIMIT);
  }

  paste_.append(PASTE_END.substr(0, held));

  if (marker_ == 0) {
    paste_ += static_cast<char>(value);
  }

  return {};
}

// consume=false emits an event for the preceding prefix while leaving this byte for
// the next pull. The caller rolls back its offset, preserving chunk-independent decoding.
std::optional<InputEvent> InputDecoder::byte(unsigned char value, bool &consume, InputTime now) {
  switch (state_) {
  case State::GROUND:
    return ground(value, false, now);
  case State::ESCAPE:
    if (value == ESCAPE_BYTE) {
      consume = false;
      clearSequence();

      return KeyEvent{InputKey::ESCAPE};
    }

    if (value == '[' || value == 'O') {
      state_ = value == '[' ? State::CSI : State::SS3;
      deadline_ = after(sequence_started_, options_.sequence_timeout);
      sequence_[0] = static_cast<char>(ESCAPE_BYTE);
      sequence_[1] = static_cast<char>(value);
      length_ = SEQUENCE_PREFIX_BYTES;

      return {};
    }

    if (value == ']' || value == 'P' || value == 'X' || value == '^' || value == '_') {
      state_ = State::STRING;
      deadline_ = after(sequence_started_, options_.sequence_timeout);
      string_osc_ = value == ']';
      string_escape_ = false;

      return issue(InputIssueKind::UNKNOWN_SEQUENCE);
    }

    state_ = State::GROUND;

    return ground(value, true, now);
  case State::UTF8:
    if ((value & UTF8_CONTINUATION_CLASS_MASK) != UTF8_CONTINUATION_CLASS) {
      consume = false;

      auto error = issue(InputIssueKind::INVALID_UTF8);

      --error.end;
      clearSequence();

      return error;
    }

    scalar_ = (scalar_ << UTF8_CONTINUATION_BITS) | (value & UTF8_CONTINUATION_PAYLOAD_MASK);

    if (--remaining_ == 0) {
      const auto scalar = scalar_;
      const auto alt = alt_;
      const auto valid = scalar >= minimum_scalar_ && scalar <= MAX_UNICODE_SCALAR &&
                         !(scalar >= SURROGATE_BEGIN && scalar <= SURROGATE_END);

      clearSequence();

      if (!valid) {
        return issue(InputIssueKind::INVALID_UTF8);
      }

      return TextInputEvent{scalar, {false, alt, false, false}};
    }

    return {};
  case State::CSI:
  case State::SS3:
  case State::DISCARD_CSI:
    return control(value, now);
  case State::STRING:
    if ((string_escape_ && value == '\\') || (string_osc_ && value == BELL_BYTE)) {
      clearSequence();
    }

    string_escape_ = value == ESCAPE_BYTE;

    return {};
  case State::PASTE:
  case State::DISCARD_PASTE:
    return paste(value);
  case State::FAILED:
    return {};
  }

  return {};
}

std::expected<InputStep, InputError> InputDecoder::pull(std::string_view bytes, InputTime now, bool ending) {
  if (last_time_ && now < *last_time_) {
    return std::unexpected(InputError::BACKWARD_TIME);
  }

  if (!ending && ending_) {
    return std::unexpected(InputError::CLOSED);
  }

  if (state_ == State::FAILED) {
    return std::unexpected(InputError::FAILED);
  }

  // Preflight the maximum prefix this call could consume, before any mutation.
  if (std::min(bytes.size(), options_.work_bytes) > std::numeric_limits<std::uint64_t>::max() - offset_) {
    return std::unexpected(InputError::OFFSET_OVERFLOW);
  }

  last_time_ = now;
  ending_ = ending_ || ending;

  InputStep result;

  // Deliver already-determined events and expire old prefixes before accepting bytes
  // with this timestamp. Waiting or chunking must not turn a timed-out ESC into Alt.
  if (pending_) {
    result.event = std::move(pending_);
    pending_.reset();
  } else {
    result.event = expire(now);
  }

  if (result.event) {
    result.status = state_ == State::FAILED ? InputStatus::FAILED : InputStatus::EVENT;

    return result;
  }

  if (ending) {
    if (!finished_ && state_ != State::GROUND) {
      if (state_ == State::UTF8 && alt_) {
        auto error = issue(InputIssueKind::TRUNCATED);

        ++error.begin;
        pending_ = error;
        clearSequence();
        result.event = KeyEvent{InputKey::ESCAPE};
        result.status = InputStatus::EVENT;

        return result;
      } else if (state_ == State::ESCAPE) {
        result.event = KeyEvent{InputKey::ESCAPE};
      } else {
        result.event = issue(InputIssueKind::TRUNCATED);
      }

      clearSequence();
      std::string{}.swap(paste_);
    }

    finished_ = true;
    result.status = InputStatus::FINISHED;

    return result;
  }

  try {
    while (result.consumed < bytes.size() && result.consumed < options_.work_bytes) {
      bool consume = true;
      ++offset_;
      result.event = byte(static_cast<unsigned char>(bytes[result.consumed]), consume, now);

      if (consume) {
        ++result.consumed;
      } else {
        --offset_;
      }

      if (!result.event) {
        result.event = expire(now);
      }

      if (result.event) {
        result.status = state_ == State::FAILED ? InputStatus::FAILED : InputStatus::EVENT;

        return result;
      }
    }
  } catch (...) {
    state_ = State::FAILED;
    std::string{}.swap(paste_);
    throw;
  }

  result.status = result.consumed < bytes.size() ? InputStatus::WORK_LIMIT : InputStatus::NEED_INPUT;

  return result;
}

std::expected<InputStep, InputError> InputDecoder::next(std::string_view bytes, InputTime now) {
  return pull(bytes, now, false);
}

std::expected<InputStep, InputError> InputDecoder::finish(InputTime now) {
  return pull({}, now, true);
}

} // namespace portals
