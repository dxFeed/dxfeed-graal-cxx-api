// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include "posix_io.hpp"
#include <portals/application.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <utility>

#ifdef PORTALS_TEST_WRAP_TERMIOS
int screen_write_failure = 0;
// Linker --wrap entry points must retain their externally required names.
extern "C" ssize_t __real_write(int, const void *, std::size_t); // NOLINT(readability-identifier-naming)

extern "C" ssize_t __wrap_write(int fd, const void *bytes, std::size_t count) { // NOLINT(readability-identifier-naming)
  if (screen_write_failure == 1 &&
      std::string_view(static_cast<const char *>(bytes), count).starts_with("\x1b[?1049h")) {
    screen_write_failure = 2;

    return __real_write(fd, bytes, count - 1);
  }

  if (screen_write_failure == 3 && std::string_view(static_cast<const char *>(bytes), count)
                                     .ends_with("\x1b"
                                                "7")) {
    screen_write_failure = 4;

    return __real_write(fd, bytes, count - 1);
  }

  if (screen_write_failure == 2 || screen_write_failure == 4) {
    screen_write_failure = 0;
    errno = EIO;

    return -1;
  }

  return __real_write(fd, bytes, count);
}

int termios_calls = 0;
int fail_termios_call = 0;
int second_fail_termios_call = 0;
// Linker --wrap entry points must retain their externally required names.
extern "C" int __real_tcsetattr(int, int, const termios *); // NOLINT(readability-identifier-naming)

extern "C" int __wrap_tcsetattr(int fd, int action, const termios *modes) { // NOLINT(readability-identifier-naming)
  ++termios_calls;

  if (termios_calls == fail_termios_call || termios_calls == second_fail_termios_call) {
    errno = EIO;

    return -1;
  }

  return __real_tcsetattr(fd, action, modes);
}
#endif

namespace {
using namespace portals;
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;
int failures = 0;

void check(bool condition, const char *message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

class Descriptor {
public:
  explicit Descriptor(int value = -1) : value_(value) {
  }

  ~Descriptor() {
    reset();
  }

  Descriptor(const Descriptor &) = delete;
  Descriptor &operator=(const Descriptor &) = delete;

  int get() const {
    return value_;
  }

  void reset() {
    if (value_ >= 0) {
      ::close(value_);
      value_ = -1;
    }
  }

private:
  int value_;
};

class Pty {
public:
  Pty() : master(posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK)), slave(openSlave()) {
    if (slave.get() < 0 || tcgetattr(slave.get(), &before) != 0) {
      throw std::runtime_error("PTY allocation/snapshot");
    }

    flags = fcntl(slave.get(), F_GETFL);

    winsize initial{25, 80, 0, 0};

    if (ioctl(slave.get(), TIOCSWINSZ, &initial) != 0) {
      throw std::runtime_error("PTY initial size");
    }
  }

  NativeTerminalHandles handles() const {
    return {slave.get(), slave.get()};
  }

  bool restored() const {
    termios after{};

    if (tcgetattr(slave.get(), &after) != 0) {
      return false;
    }

    return before.c_iflag == after.c_iflag && before.c_oflag == after.c_oflag && before.c_cflag == after.c_cflag &&
           before.c_lflag == after.c_lflag && cfgetispeed(&before) == cfgetispeed(&after) &&
           cfgetospeed(&before) == cfgetospeed(&after) &&
           std::equal(std::begin(before.c_cc), std::end(before.c_cc), std::begin(after.c_cc)) &&
           flags == fcntl(slave.get(), F_GETFL);
  }

  Descriptor master;
  Descriptor slave;
  termios before{};
  int flags{};

private:
  int openSlave() {
    if (master.get() < 0 || grantpt(master.get()) != 0 || unlockpt(master.get()) != 0) {
      return -1;
    }

    const char *name = ptsname(master.get());

    return name ? ::open(name, O_RDWR | O_NOCTTY) : -1;
  }
};

std::string drainOutput(Pty &pty) {
  std::string bytes;
  char buffer[512];

  for (;;) {
    const auto count = ::read(pty.master.get(), buffer, sizeof(buffer));

    if (count <= 0) {
      return bytes;
    }

    bytes.append(buffer, static_cast<std::size_t>(count));
  }
}

constexpr std::string_view ENCODING_REPLIES = "\x1b[?1005;2$y\x1b[?1006;1$y\x1b[?1015;2$y\x1b[?1016;2$y";

std::expected<std::unique_ptr<ApplicationTerminal>, TerminalError>
openMouseTestTerminal(Pty &pty, TerminalOptions options = {}, std::chrono::milliseconds timeout = 100ms,
                      std::string replies = std::string(ENCODING_REPLIES)) {
  std::jthread responder([&] {
    std::string received;
    const auto deadline = Clock::now() + timeout;

    while (Clock::now() < deadline) {
      received += drainOutput(pty);

      if (received.find("\x1b[?1016$p") != std::string::npos) {
        // Deliberately split replies across native reads; preserve ordinary input.
        for (const char byte : replies) {
          if (::write(pty.master.get(), &byte, 1) != 1) {
            break;
          }

          std::this_thread::yield();
        }

        return;
      }

      std::this_thread::yield();
    }
  });

  return openNativeApplicationTerminal(pty.handles(), options, timeout, true, NativeScreenMode::MAIN_SCREEN);
}

void managedMouse() {
  Pty pty;
  auto opened = openMouseTestTerminal(pty);

  check(opened.has_value(), "managed terminal opens");

  if (!opened) {
    return;
  }

  const auto startup = drainOutput(pty);

  check(startup.find("\x1b[?1000;1006h") != std::string::npos, "native transport enables SGR button tracking");
  check(startup.find("s") < startup.find("h"), "mouse modes saved before mutation");
  check((*opened)->submit("pending-frame").has_value(), "managed transport accepts pending frame");
  check((*opened)->close().has_value(), "managed transport closes");

  const auto shutdown = drainOutput(pty);

  check(shutdown.starts_with("pending-frame"), "managed close drains retryable output before restoration");
  check(shutdown.ends_with("r"), "managed close restores saved private modes");
  check(pty.restored(), "managed close restores native OS modes");
  check((*opened)->close().has_value() && drainOutput(pty).empty(), "managed close is idempotent");

  auto invalid = openNativeApplicationTerminal(pty.handles(), {}, 0ms);

  check(!invalid && invalid.error().kind == TerminalErrorKind::INVALID_OPTIONS && pty.restored(),
        "invalid mouse timeout leaves terminal unchanged");

  auto excessive = openNativeApplicationTerminal(pty.handles(), {}, std::chrono::milliseconds::max());

  check(!excessive && excessive.error().kind == TerminalErrorKind::INVALID_OPTIONS && pty.restored(),
        "unrepresentable mouse timeout is rejected before native mutation");

  TerminalOptions small;

  small.max_output_bytes = 1;

  auto rejected = openMouseTestTerminal(pty, small);

  check(!rejected && rejected.error().kind == TerminalErrorKind::OUTPUT_LIMIT && pty.restored(),
        "startup output rejection rolls back OS state");
  check(drainOutput(pty).empty(), "rejected startup emits no incomplete mode sequence");
  Pty restoreLimitPty;
  TerminalOptions limitedCleanup;

  // Queries fit in 40 bytes, but restoring the original modes needs a larger batch.
  limitedCleanup.max_output_bytes = 40;

  auto restoreRejected = openMouseTestTerminal(restoreLimitPty, limitedCleanup);

  check(!restoreRejected && restoreRejected.error().kind == TerminalErrorKind::OUTPUT_LIMIT &&
          restoreLimitPty.restored(),
        "cleanup output limit is rejected before enabling mouse reporting");
  check(drainOutput(restoreLimitPty).empty(), "cleanup preflight emits neither mode save nor mutation");

  {
    auto scoped = openMouseTestTerminal(pty);

    check(scoped.has_value(), "managed scope opens");
    (void)drainOutput(pty);
  }

  check(drainOutput(pty).ends_with("r") && pty.restored(), "managed destructor restores without explicit close");
}

void managedMouseQueries() {
  Pty pty;
  TerminalOptions options;

  options.max_input_bytes = 32;

  const std::string mixed = "before\x1b[?1005;2$ybetween\x1b[?1006;1$y\x1b[?777;2$y\x1b[?1015;2$y\x1b[?1016;2$yafter";
  auto opened = openMouseTestTerminal(pty, options, 100ms, mixed);

  check(opened.has_value(), "small read cap permits mixed chunked replies and input");

  if (opened) {
    std::string retained;

    for (int attempt = 0; attempt < 3 && retained.size() < 29; ++attempt) {
      retained += (*opened)->read(Clock::now() + 50ms, {}).bytes;
    }

    check(retained == "beforebetween\x1b[?777;2$yafter",
          "startup preserves non-query input and unknown replies in order");
    check((*opened)->close().has_value() && pty.restored(), "mixed startup fixture restores");
  }

  (void)drainOutput(pty);

  Pty overflowPty;
  auto overflow =
    openMouseTestTerminal(overflowPty, options, 100ms, std::string(33, 'x') + std::string(ENCODING_REPLIES));

  check(!overflow && overflow.error().kind == TerminalErrorKind::INPUT_LIMIT && overflowPty.restored(),
        "startup retained input limit fails before mode mutation");

  Pty unsupportedPty;
  auto unsupported =
    openMouseTestTerminal(unsupportedPty, {}, 100ms, "\x1b[?1005;2$y\x1b[?1006;0$y\x1b[?1015;0$y\x1b[?1016;0$y");

  check(!unsupported && unsupported.error().kind == TerminalErrorKind::UNSUPPORTED && unsupportedPty.restored(),
        "unrecognized SGR query fails without assuming mouse modes disabled");

  Pty missingPty;
  auto missing = openNativeApplicationTerminal(missingPty.handles(), {}, 15ms);

  check(!missing && missing.error().native_code == ETIMEDOUT && missingPty.restored(),
        "missing query replies have a deadline");

  Pty optedOutPty;
  auto optedOut = openNativeApplicationTerminal(optedOutPty.handles(), {}, 15ms, false, NativeScreenMode::MAIN_SCREEN);

  check(optedOut.has_value() && drainOutput(optedOutPty).empty(), "mouse opt-out requires no query or setup I/O");

  if (optedOut) {
    check((*optedOut)->close().has_value() && drainOutput(optedOutPty).empty() && optedOutPty.restored(),
          "mouse opt-out performs native OS cleanup only");
  }
}

void managedMouseFailures() {
  Pty pty;
  auto opened = openMouseTestTerminal(pty, {}, 100ms);

  check(opened.has_value(), "managed deadline fixture opens");

  if (!opened) {
    return;
  }

  (void)drainOutput(pty);

  std::string pressure(1024 * 1024, 'x');

  check((*opened)->submit(std::move(pressure)).has_value(), "managed fixture retains output pressure");

  const auto partial = (*opened)->flush(Clock::now() + 5ms, {});

  check(partial.status == TerminalStatus::DEADLINE && partial.completed > 0 && partial.pending > 0,
        "managed fixture observes actual partial native writes");

  const auto begin = Clock::now();
  const auto closed = (*opened)->close();

  check(!closed && closed.error().native_code == ETIMEDOUT, "blocked shutdown reports restoration deadline");
  check(Clock::now() - begin < 500ms && pty.restored(), "blocked shutdown stays bounded and still restores OS state");
  check((*opened)->close().has_value(), "failed managed close still releases ownership");
  (void)drainOutput(pty);

  auto failed = openMouseTestTerminal(pty, {}, 100ms);

  check(failed.has_value(), "managed I/O failure fixture opens");

  if (failed) {
    pty.master.reset();

    const auto error = (*failed)->close();

    check(!error && error.error().kind == TerminalErrorKind::SYSTEM, "broken terminal restoration remains an error");
    check(!(*failed)->size() && (*failed)->size().error().kind == TerminalErrorKind::CLOSED,
          "managed I/O failure still closes native session");
  }
}

void managedScreen() {
  Pty pty;
  auto terminal = openNativeApplicationTerminal(pty.handles(), {}, 100ms, false);

  check(terminal.has_value(), "screen transport opens without mouse queries");

  if (!terminal) {
    return;
  }

  const auto startup = drainOutput(pty);

  check(startup.find("\x1b[?1049h") != std::string::npos && startup.find("\x1b[?25s") < startup.find("1049h"),
        "alternate screen is entered after confirmed cursor visibility save");
  check((*terminal)->submit("pending-screen-frame").has_value(), "screen frame accepted before close");
  check((*terminal)->close().has_value() && pty.restored(), "screen close restores original OS state");

  const auto shutdown = drainOutput(pty);

  check(shutdown.starts_with("pending-screen-frame") && shutdown.find("\x1b[?1049l") != std::string::npos &&
          shutdown.ends_with("\x1b[?1049l\x1b[?25r"),
        "close drains frame then restores screen/cursor/rendition/visibility");
  check((*terminal)->close().has_value() && drainOutput(pty).empty(), "screen close is idempotent");

  {
    auto scoped = openNativeApplicationTerminal(pty.handles(), {}, 100ms, false);

    check(scoped.has_value(), "screen destructor fixture opens");
    (void)drainOutput(pty);
  }

  check(drainOutput(pty).find("\x1b[?1049l") != std::string::npos && pty.restored(),
        "screen destructor restores without explicit close");

  auto direct = openNativeApplicationTerminal(pty.handles(), {}, 100ms, false, NativeScreenMode::MAIN_SCREEN);

  check(direct.has_value() && drainOutput(pty).empty(), "MAIN opt-out emits no screen setup");
  check((*direct)->close().has_value() && drainOutput(pty).empty() && pty.restored(),
        "MAIN opt-out emits no screen cleanup");

  TerminalOptions small;

  small.max_output_bytes = 1;

  auto rejected = openNativeApplicationTerminal(pty.handles(), small, 100ms, false);

  check(!rejected && rejected.error().kind == TerminalErrorKind::OUTPUT_LIMIT && pty.restored() &&
          drainOutput(pty).empty(),
        "screen cleanup capacity rejected before native mutation");

#ifdef PORTALS_TEST_WRAP_TERMIOS
  screen_write_failure = 1;

  auto partial = openNativeApplicationTerminal(pty.handles(), {}, 100ms, false);

  check(!partial && partial.error().kind == TerminalErrorKind::SYSTEM && partial.error().native_code == EIO,
        "partial alternate-screen entry remains a startup error");

  const auto rollback = drainOutput(pty);

  check(rollback.find("\x1b[?1049\x18") != std::string::npos && rollback.find("\x1b[?1049l") != std::string::npos &&
          pty.restored(),
        "partial entry aborts the incomplete sequence and attempts screen rollback");
#endif

  auto broken = openNativeApplicationTerminal(pty.handles(), {}, 100ms, false);

  check(broken.has_value(), "broken screen cleanup fixture opens");
  pty.master.reset();

  const auto failed = (*broken)->close();

  check(!failed && !(*broken)->size() && (*broken)->close().has_value(),
        "failed screen restoration is reported and closes the session");
}

void sessions() {
  Pty pty;
  TerminalOptions options;

  options.max_input_bytes = 2;
  options.max_output_bytes = 8;
  options.wait_cadence = 2ms;
  {
    auto opened = TerminalSession::open(pty.handles(), options);

    check(opened.has_value(), "open shared input/output descriptor");

    if (!opened) {
      return;
    }

    auto session = std::move(*opened);

    check(!opened->size() && opened->read(Clock::now()).error->kind == TerminalErrorKind::CLOSED,
          "moved-from session closed");

    termios raw{};

    tcgetattr(pty.slave.get(), &raw);
    check((raw.c_lflag & (ICANON | ECHO | ISIG | IEXTEN)) == 0 && (raw.c_oflag & OPOST) == 0 &&
            (fcntl(pty.slave.get(), F_GETFL) & O_NONBLOCK) != 0,
          "raw and nonblocking modes");
    check(session.size().has_value() && *session.size() == CellSize{80, 25}, "initial cell size");
    check(::write(pty.master.get(), "abc", 3) == 3, "inject raw bytes");

    auto first = session.read(Clock::now() + 1s);
    auto second = session.read(Clock::now() + 1s);

    check(first.bytes == "ab" && second.bytes == "c" && first.events.empty(), "bounded owning raw input");

    winsize resized{31, 91, 0, 0};

    check(ioctl(pty.master.get(), TIOCSWINSZ, &resized) == 0, "resize PTY");

    auto resize = session.read(Clock::now() + 1s);

    check(resize.resized == CellSize{91, 31} && resize.status == TerminalStatus::PROGRESS, "owner resize event");

    const auto start = Clock::now();

    check(session.read(start + 5ms).status == TerminalStatus::DEADLINE, "read deadline");
    check(Clock::now() - start < 500ms, "bounded empty-input wait");

    std::stop_source stopped;

    stopped.request_stop();
    check(session.read(Clock::now() + 1s, stopped.get_token()).status == TerminalStatus::STOPPED, "read cancellation");
    check(!session.submit("123456789") && session.pendingBytes() == 0, "bounded output rejection");
    check(session.submit("hello\n").has_value(), "submit owned output");
    check(!session.submit("x") && session.pendingBytes() == 6, "one pending batch only");

    auto timeout = session.flush(Clock::now());

    check(timeout.status == TerminalStatus::DEADLINE && timeout.completed == 0 && timeout.pending == 6,
          "deadline preserves output");

    auto stop = session.flush(Clock::now() + 1s, stopped.get_token());

    check(stop.status == TerminalStatus::STOPPED && stop.pending == 6, "stop preserves output");

    auto written = session.flush(Clock::now() + 1s);

    check(written.status == TerminalStatus::PROGRESS && written.completed == 6 && written.pending == 0,
          "honest completed-byte count");

    char output[16]{};

    check(::read(pty.master.get(), output, sizeof(output)) == 6 && std::string_view(output, 6) == "hello\n",
          "no output postprocessing");
    check(session.submit("discard").has_value(), "pending output before close");
    check(session.close().has_value() && session.close().has_value(), "explicit close idempotent");
    check(::read(pty.master.get(), output, sizeof(output)) == -1 && errno == EAGAIN,
          "close discards pending output without flushing");
    check(!session.submit("x") && !session.size(), "closed calls rejected");
    check(first.bytes == "ab", "earlier owning input survives close");
  }

  check(pty.restored(), "all original modes/flags restored after shared-fd close");

  try {
    auto session = TerminalSession::open(pty.handles());

    if (!session) {
      throw std::runtime_error("open");
    }

    throw std::runtime_error("caught application error");
  } catch (const std::runtime_error &) {
  }

  check(pty.restored(), "RAII restores after caught exception");

  Pty other;

  {
    auto first = TerminalSession::open(pty.handles());
    auto second = TerminalSession::open(other.handles());

    check(first.has_value() && second.has_value(), "two independent terminals");

    if (first && second) {
      *first = std::move(*second);
      check(pty.restored(), "move assignment restores old owner");
    }
  }

  check(other.restored(), "move assignment owns replacement");

  TerminalOptions invalid;

  invalid.wait_cadence = 0ms;
  check(!TerminalSession::open(pty.handles(), invalid), "invalid cadence rejected before mutation");
  check(pty.restored(), "invalid open leaves modes alone");

#ifdef PORTALS_TEST_WRAP_TERMIOS
  termios_calls = 0;
  fail_termios_call = 2;

  auto failed = TerminalSession::open(pty.handles());

  fail_termios_call = 0;
  check(!failed && failed.error().native_code == EIO && failed.error().restoration_code == 0,
        "partial acquisition exposes original error");
  check(pty.restored() && termios_calls == 4, "partial acquisition restores both independent states");

  termios_calls = 0;
  fail_termios_call = 2;
  second_fail_termios_call = 3;
  failed = TerminalSession::open(pty.handles());
  fail_termios_call = 0;
  second_fail_termios_call = 0;
  check(!failed && failed.error().native_code == EIO && failed.error().restoration_code == EIO,
        "open distinguishes acquisition and rollback errors");
  check(pty.restored() && termios_calls == 4, "rollback failure still attempts the other shared state");

  auto session = TerminalSession::open({pty.slave.get(), other.slave.get()});

  termios_calls = 0;
  fail_termios_call = 1;

  auto closed = session->close();

  fail_termios_call = 0;
  check(!closed && closed.error().native_code == EIO, "explicit close reports restore error");
  check(termios_calls == 2 && other.restored() && fcntl(pty.slave.get(), F_GETFL) == pty.flags,
        "restore failure still restores other terminal and both descriptor flags");
  tcsetattr(pty.slave.get(), TCSANOW, &pty.before);
#endif
}

struct Script {
  TerminalDeadline current{};
  std::string written;
  int writes{};
  int polls{};
  int reads{};
  std::vector<int> timeouts;
} script;

TerminalDeadline scriptedNow() {
  return script.current;
}

int scriptedPoll(pollfd *, nfds_t, int timeout) {
  script.timeouts.push_back(timeout);
  ++script.polls;
  script.current += 3ms;
  errno = EINTR;

  return -1;
}

ssize_t scriptedWrite(int, const void *bytes, std::size_t count) {
  ++script.writes;

  if (script.writes == 1) {
    const auto size = std::min<std::size_t>(2, count);

    script.written.append(static_cast<const char *>(bytes), size);

    return static_cast<ssize_t>(size);
  }

  errno = script.writes == 2 ? EINTR : EAGAIN;

  return -1;
}

ssize_t successWrite(int, const void *bytes, std::size_t count) {
  script.written.append(static_cast<const char *>(bytes), count);

  return static_cast<ssize_t>(count);
}

ssize_t scriptedRead(int, void *bytes, std::size_t count) {
  if (++script.reads < 3) {
    script.current += 1ms;
    errno = EINTR;

    return -1;
  }

  if (count) {
    static_cast<char *>(bytes)[0] = 'x';
  }

  return count ? 1 : 0;
}

void interruptedIo() {
  script = {};

  detail::PosixIo io{scriptedPoll, scriptedRead, scriptedWrite, scriptedNow};
  std::string bytes = "abcdef";
  std::size_t offset = 0;
  auto result = detail::flushPosix(-1, bytes, offset, TerminalDeadline{} + 8ms, {}, 20ms, io);

  check(result.status == TerminalStatus::DEADLINE && result.completed == 2 && result.pending == 4 && offset == 2,
        "partial write/EINTR/EAGAIN preserves exact suffix");
  check(script.timeouts == std::vector<int>({8, 5, 2}), "EINTR recomputes absolute deadline");
  io.write = successWrite;

  auto resumed = detail::flushPosix(-1, bytes, offset, script.current + 1s, {}, 20ms, io);

  check(resumed.completed == 4 && resumed.pending == 0 && script.written == bytes,
        "resumed output contains no duplicated or missing bytes");
  script = {};

  auto input = detail::readPosix(-1, 8, TerminalDeadline{} + 8ms, {}, io);

  check(input.bytes == "x" && script.reads == 3, "read EINTR retries without discarding data");
  script = {};
  input = detail::readPosix(-1, 8, TerminalDeadline{} + 1ms, {}, io);
  check(input.status == TerminalStatus::DEADLINE && input.bytes.empty() && script.reads == 1,
        "read EINTR observes expired absolute deadline");
  check(detail::pollTimeout(TerminalDeadline::max(), 10ms, TerminalDeadline::min()) == 10,
        "extreme absolute deadline has no arithmetic overflow");
}

void terminalEof() {
  Pty pty;
  auto session = TerminalSession::open(pty.handles());

  check(session.has_value(), "open for terminal hangup");

  if (!session) {
    return;
  }

  pty.master.reset();
  check(session->read(Clock::now() + 1s).status == TerminalStatus::END_OF_INPUT,
        "terminal hangup reports EOF even when winsize query fails");

  auto closed = session->close();

  check(!closed && closed.error().kind == TerminalErrorKind::SYSTEM,
        "disconnected terminal explicitly reports unavailable restoration");
}

void pipes() {
  int fds[2];

  if (pipe(fds) != 0) {
    throw std::runtime_error("pipe");
  }

  Descriptor reader(fds[0]);
  Descriptor writer(fds[1]);

  fcntl(reader.get(), F_SETFL, O_NONBLOCK);
  fcntl(writer.get(), F_SETFL, O_NONBLOCK);
  check(!TerminalSession::open({reader.get(), writer.get()}), "public session rejects redirected pipes");

  const auto &io = detail::nativePosixIo();
  std::string bytes(1024 * 1024, 'q');
  std::size_t offset = 0;
  auto partial = detail::flushPosix(writer.get(), bytes, offset, Clock::now() + 5ms, {}, 1ms, io);

  check(partial.status == TerminalStatus::DEADLINE && partial.completed > 0 && partial.pending > 0,
        "real nonblocking pipe short write and backpressure");

  std::string received;
  char buffer[8192];

  for (;;) {
    const auto count = ::read(reader.get(), buffer, sizeof(buffer));

    if (count <= 0) {
      break;
    }

    received.append(buffer, static_cast<std::size_t>(count));
  }

  check(received == bytes.substr(0, offset), "real completed prefix equals pipe bytes");
  check(::write(writer.get(), "tail", 4) == 4, "buffer before hangup");
  writer.reset();

  pollfd ready{reader.get(), POLLIN, 0};

  check(::poll(&ready, 1, 0) == 1 && (ready.revents & (POLLIN | POLLHUP)) == (POLLIN | POLLHUP),
        "fixture exposes simultaneous readable and hangup");

  auto tail = detail::readPosix(reader.get(), 16, Clock::now() + 1s, {}, io);
  auto eof = detail::readPosix(reader.get(), 16, Clock::now() + 1s, {}, io);

  check(tail.bytes == "tail" && eof.status == TerminalStatus::END_OF_INPUT, "drain bytes before EOF");

  int broken[2];

  if (pipe(broken) != 0) {
    throw std::runtime_error("broken pipe");
  }

  Descriptor broken_writer(broken[1]);

  ::close(broken[0]);
  std::signal(SIGPIPE, SIG_DFL);

  sigset_t before{}, after{};

  pthread_sigmask(SIG_SETMASK, nullptr, &before);
  offset = 0;

  auto closed = detail::flushPosix(broken_writer.get(), "x", offset, Clock::now() + 1s, {}, 1ms, io);

  pthread_sigmask(SIG_SETMASK, nullptr, &after);
  check(closed.status == TerminalStatus::FAILED && closed.error->native_code == EPIPE && closed.completed == 0,
        "closed pipe returns EPIPE and child survives default SIGPIPE");
  check(sigismember(&before, SIGPIPE) == sigismember(&after, SIGPIPE), "writer restores thread mask");

  struct sigaction disposition{};

  sigaction(SIGPIPE, nullptr, &disposition);
  check(disposition.sa_handler == SIG_DFL, "writer leaves process SIGPIPE disposition unchanged");

  sigset_t blocked{};

  sigemptyset(&blocked);
  sigaddset(&blocked, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &blocked, &before);
  raise(SIGPIPE);
  check(detail::writeWithoutSigpipe(broken_writer.get(), "x", 1) == -1 && errno == EPIPE,
        "write with already pending SIGPIPE");
  sigpending(&after);
  check(sigismember(&after, SIGPIPE) == 1, "preexisting pending signal remains owned by caller");

  timespec zero{};

  sigtimedwait(&blocked, nullptr, &zero);
  pthread_sigmask(SIG_SETMASK, &before, nullptr);
}

constexpr std::array MOUSE_MODES{9, 1000, 1001, 1002, 1003, 1005, 1006, 1015, 1016};

template <class Terminal> void sendControl(Terminal &terminal, std::string bytes) {
  const auto submitted = terminal.submit(std::move(bytes));

  if (!submitted) {
    throw std::runtime_error("native mode submission");
  }

  const auto written = terminal.flush(Clock::now() + 1s, {});

  if (written.error || written.status != TerminalStatus::PROGRESS || written.pending != 0) {
    throw std::runtime_error("native mode flush");
  }
}

template <class Terminal> std::array<int, MOUSE_MODES.size()> queryMouse(Terminal &terminal) {
  std::string requests;

  for (const int mode : MOUSE_MODES) {
    requests += "\x1b[?" + std::to_string(mode) + "$p";
  }

  sendControl(terminal, std::move(requests));

  const auto deadline = Clock::now() + 1s;
  std::string replies;
  std::array<int, MOUSE_MODES.size()> states;

  states.fill(-1);

  while (Clock::now() < deadline) {
    const auto input = terminal.read(deadline, {});

    replies += input.bytes;

    for (std::size_t index = 0; index < MOUSE_MODES.size(); ++index) {
      const auto prefix = "\x1b[?" + std::to_string(MOUSE_MODES[index]) + ";";
      const auto begin = replies.find(prefix);

      if (begin == std::string::npos) {
        continue;
      }

      const auto end = replies.find("$y", begin + prefix.size());

      if (end != std::string::npos) {
        const auto value = replies.data() + begin + prefix.size();
        const auto parsed = std::from_chars(value, replies.data() + end, states[index]);

        if (parsed.ec != std::errc{} || parsed.ptr != replies.data() + end) {
          throw std::runtime_error("invalid DECRQM response");
        }
      }
    }

    if (std::ranges::all_of(states, [](int state) {
          return state >= 0;
        })) {
      return states;
    }
  }

  throw std::runtime_error("DECRQM response deadline");
}

struct ScreenState {
  std::string screen;
  std::string visibility;
  std::string cursor;
  std::string rendition;
  bool operator==(const ScreenState &) const = default;
};

template <class Terminal> ScreenState queryScreen(Terminal &terminal) {
  sendControl(terminal, "\x1b[?1049$p\x1b[?25$p\x1b[6n\x1bP$qm\x1b\\");

  const auto deadline = Clock::now() + 1s;
  std::string bytes;

  while (Clock::now() < deadline) {
    bytes += terminal.read(deadline, {}).bytes;

    auto field = [&](std::string_view begin, std::string_view end) {
      const auto first = bytes.find(begin);

      if (first == std::string::npos) {
        return std::string{};
      }

      const auto start = first + begin.size();
      const auto last = bytes.find(end, start);

      return last == std::string::npos ? std::string{} : bytes.substr(start, last - start);
    };

    ScreenState state{field("\x1b[?1049;", "$y"), field("\x1b[?25;", "$y"), {}, field("\x1bP1$r", "m\x1b\\")};
    const auto end = bytes.find('R');

    if (end != std::string::npos) {
      const auto start = bytes.rfind("\x1b[", end);

      if (start != std::string::npos) {
        state.cursor = bytes.substr(start + 2, end - start - 2);
      }
    }

    if (!state.screen.empty() && !state.visibility.empty() && !state.cursor.empty() && !state.rendition.empty()) {
      return state;
    }
  }

  throw std::runtime_error("native screen state query deadline");
}

int nativeScreenRestore(const char *path) {
  std::ofstream log(path);
  auto outer = TerminalSession::open({0, 1});

  if (!log || !outer) {
    return 2;
  }

  auto stage = [&](std::string_view name) {
    log << "stage=" << name << '\n';
    log.flush();

    const auto deadline = Clock::now() + 5s;
    const auto ack = std::string(path) + "." + std::string(name) + ".ack";

    while (::access(ack.c_str(), F_OK) != 0) {
      if (Clock::now() >= deadline) {
        throw std::runtime_error("private native screen acknowledgement deadline");
      }

      std::this_thread::sleep_for(10ms);
    }
  };
  auto record = [&](std::string_view name, const ScreenState &state) {
    log << "state=" << name << " screen=" << state.screen << " visibility=" << state.visibility
        << " cursor=" << state.cursor << " rendition=" << state.rendition << '\n';
    log.flush();
  };
  auto seed = [&] {
    // Make an old DECSC cache differ from the actual caller state before opening the managed owner.
    sendControl(*outer, "\x1b[3;5H\x1b[32;45m\x1b"
                        "7\x1b[0m\x1b[2J\x1b[H"
                        "Previous shell output: abcXYZ\r\nprompt> before gallery\x1b[39;118H\x1b[34;42m\x1b[?25l");
  };
  constexpr std::string_view FRAME = "\x1b[0m\x1b[2J\x1b[1;1H\x1b[31;44mAlerts\r\nRun\r\nQuit\x1b[1;1H\x1b[?25h";

  try {
    seed();

    const auto before = queryScreen(*outer);

    record("before", before);
    stage("before");
    check(outer->close().has_value(), "screen probe releases low-level OS owner");

    auto terminal = openNativeApplicationTerminal({0, 1}, {}, 1s, false);

    if (!terminal) {
      throw std::runtime_error("native screen probe opens");
    }

    sendControl(**terminal, std::string(FRAME));

    const auto active = queryScreen(**terminal);

    record("active", active);
    check(active.screen == "1", "actual xterm enters the alternate screen");
    stage("active");
    check((*terminal)->close().has_value(), "actual xterm managed screen closes");
    outer = TerminalSession::open({0, 1});

    const auto after = queryScreen(*outer);

    record("after", after);
    stage("after");
    check(after == before, "actual xterm restores caller cursor/rendition/visibility on the main screen");

#ifdef PORTALS_TEST_WRAP_TERMIOS
    seed();

    const auto partialBefore = queryScreen(*outer);

    record("partial-before", partialBefore);
    stage("partial-before");
    check(outer->close().has_value(), "partial screen probe releases OS owner");
    screen_write_failure = 1;

    auto partial = openNativeApplicationTerminal({0, 1}, {}, 1s, false);

    check(!partial && partial.error().native_code == EIO && partial.error().restoration_code == 0,
          "actual partial screen entry retains EIO and successful cleanup");
    outer = TerminalSession::open({0, 1});

    const auto partialAfter = queryScreen(*outer);

    record("partial-after", partialAfter);
    stage("partial-after");
    check(partialAfter == partialBefore,
          "partial entry must preserve current caller state instead of stale DECSC cache");
    seed();

    const auto captureBefore = queryScreen(*outer);

    record("capture-before", captureBefore);
    stage("capture-before");
    check(outer->close().has_value(), "capture failure probe releases OS owner");
    screen_write_failure = 3;

    auto capture = openNativeApplicationTerminal({0, 1}, {}, 1s, false);

    check(!capture && capture.error().native_code == EIO && capture.error().restoration_code == 0,
          "partial cursor capture retains EIO and successful cleanup");
    outer = TerminalSession::open({0, 1});

    const auto captureAfter = queryScreen(*outer);

    record("capture-after", captureAfter);
    stage("capture-after");
    check(captureAfter == captureBefore, "unconfirmed cursor capture cleans up with CAN without stale restore");
#endif

    for (const auto target : {CellSize{40, 12}, CellSize{140, 44}}) {
      const std::string name = target.width == 40 ? "small" : "large";

      seed();
      sendControl(*outer, "\x1b[?1049h");
      sendControl(*outer, std::string(FRAME));
      stage("resize-" + name + "-control");
      check(outer->size() == target, "actual xterm control reaches resized viewport");
      sendControl(*outer, "\x1b[?1049l\x1b[?25l");

      const auto expected = queryScreen(*outer);

      record(name + "-control", expected);
      stage(name + "-control");
      stage("resize-reset-" + name + "-control");
      check(outer->size() == CellSize{120, 40}, "actual xterm baseline viewport reset between cases");
      seed();
      check(outer->close().has_value(), "resized probe releases low-level OS owner");
      auto resized = openNativeApplicationTerminal({0, 1}, {}, 1s, false);

      if (!resized) {
        throw std::runtime_error("actual resized screen opens");
      }

      sendControl(**resized, std::string(FRAME));
      stage("resize-" + name + "-managed");
      check((*resized)->size() == target, "actual managed xterm reaches new viewport");
      check((*resized)->close().has_value(), "actual resized screen closes");
      outer = TerminalSession::open({0, 1});

      const auto actual = queryScreen(*outer);

      record(name + "-managed", actual);
      stage(name + "-managed");
      check(outer->size() == target && actual == expected,
            "actual resized xterm returns host-restored primary state and retains new viewport");
      stage("resize-reset-" + name + "-managed");
    }
  } catch (const std::exception &error) {
    log << "exception=" << error.what() << '\n';
    ++failures;
  }

  if (outer) {
    check(outer->close().has_value(), "native screen probe closes final OS session");
  }

  log << "result=" << (failures == 0 ? 0 : 1) << " failures=" << failures << '\n';

  return failures == 0 ? 0 : 1;
}

int nativeMouseModes(const char *path) {
  std::ofstream log(path);
  auto outer = TerminalSession::open({0, 1});

  if (!log || !outer) {
    return 2;
  }

  const auto original = queryMouse(*outer);
  int result = 0;

  try {
    sendControl(*outer, "\x1b[?9;1000;1001;1002;1003;1005;1006;1015;1016l\x1b[?1002;1006h");

    const auto before = queryMouse(*outer);

    check(before[3] == 1 && before[6] == 1, "actual xterm pre-enables 1002 and 1006");
    check(outer->close().has_value(), "close baseline OS session before managed owner");

    auto managed = openNativeApplicationTerminal({0, 1});

    if (!managed) {
      throw std::runtime_error("native managed setup");
    }

    const auto active = queryMouse(**managed);

    check(active[1] == 1 && active[3] == 2 && active[6] == 1 && active[8] == 2,
          "actual xterm switches to SGR cell button tracking");
    check((*managed)->close().has_value(), "actual xterm managed close");

    outer = TerminalSession::open({0, 1});

    if (!outer) {
      throw std::runtime_error("native mode verification reopen");
    }

    const auto after = queryMouse(*outer);

    check(before == after, "actual xterm restores all pre-enabled mouse modes");

    for (std::size_t index = 0; index < MOUSE_MODES.size(); ++index) {
      log << "mode=" << MOUSE_MODES[index] << " before=" << before[index] << " active=" << active[index]
          << " after=" << after[index] << '\n';
    }

    result = failures == 0 ? 0 : 1;
  } catch (const std::exception &error) {
    log << "error=" << error.what() << '\n';
    result = 2;
  }

  // This optional real-emulator probe owns the baseline it changed and restores it.
  // Use explicit queried values because managed XTSAVE intentionally replaces its cache.
  if (!outer || !outer->size()) {
    outer = TerminalSession::open({0, 1});
  }

  if (outer) {
    std::string reset = "\x1b[?9;1000;1001;1002;1003;1005;1006;1015;1016l";

    for (std::size_t index = 0; index < MOUSE_MODES.size(); ++index) {
      if (original[index] == 1) {
        reset += "\x1b[?" + std::to_string(MOUSE_MODES[index]) + "h";
      }
    }

    sendControl(*outer, std::move(reset));
    check(queryMouse(*outer) == original, "native probe restores its original baseline");
    check(outer->close().has_value(), "native probe closes original OS state");
  } else {
    result = 2;
  }

  log << "result=" << result << " failures=" << failures << '\n';

  return failures == 0 ? result : 1;
}

int run() {
  sessions();
  managedScreen();
  managedMouse();
  managedMouseQueries();
  managedMouseFailures();
  interruptedIo();
  terminalEof();
  pipes();
  std::cout << "Terminal POSIX failures: " << failures << '\n';

  return failures == 0 ? 0 : 1;
}
} // namespace

int main(int argc, char **argv) {
  if (argc == 3 && std::string_view(argv[1]) == "--native-screen-restore") {
    alarm(40);

    return nativeScreenRestore(argv[2]);
  }

  if (argc == 3 && std::string_view(argv[1]) == "--native-mouse-modes") {
    alarm(8);
    return nativeMouseModes(argv[2]);
  }

  // Only the fixture child owns PTYs, signals and descriptors. Its watchdog also
  // bounds a broken test when run directly without CTest's outer timeout.
  const pid_t child = fork();

  if (child == 0) {
    alarm(10);

    int result = 2;

    try {
      result = run();
    } catch (const std::exception &error) {
      std::cerr << error.what() << '\n';
    }

    std::cout.flush();
    std::cerr.flush();
    _exit(result);
  }

  if (child < 0) {
    return 2;
  }

  int status{};

  while (waitpid(child, &status, 0) < 0) {
    if (errno != EINTR) {
      return 2;
    }
  }

  if (!WIFEXITED(status)) {
    std::cerr << "Fixture child terminated by signal " << WTERMSIG(status) << '\n';

    return 2;
  }

  return WEXITSTATUS(status);
}
