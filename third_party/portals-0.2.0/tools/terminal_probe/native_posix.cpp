// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "native.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <termios.h>
#include <unistd.h>

namespace portals::probe {
namespace {
constexpr std::string_view REQUESTS = "\x1b[0c\x1b[6n";
constexpr std::size_t READ_CHUNK_SIZE = 128;
volatile std::sig_atomic_t interrupted = 0;

void interrupt(int signal) {
  interrupted = signal;
}

std::string error(std::string_view operation) {
  const int code = errno;
  const char *description = std::strerror(code);

  return std::format("{}: {}", operation, description);
}

class SignalGuard {
public:
  bool acquire() {
    interrupted = 0;

    struct sigaction handler{};

    handler.sa_handler = interrupt;
    sigemptyset(&handler.sa_mask);

    for (; acquired_ < SIGNALS_COUNT; ++acquired_) {
      if (sigaction(signals_[acquired_], &handler, &original_[acquired_]) != 0) {
        return false;
      }
    }

    return true;
  }

  ~SignalGuard() {
    while (acquired_ > 0) {
      --acquired_;
      sigaction(signals_[acquired_], &original_[acquired_], nullptr);
    }
  }

private:
  static constexpr int SIGNALS_COUNT = 3;
  int signals_[SIGNALS_COUNT] = {SIGINT, SIGTERM, SIGHUP};
  struct sigaction original_[SIGNALS_COUNT]{};
  int acquired_ = 0;
};

class ModeGuard {
public:
  bool acquire() {
    if (tcgetattr(STDIN_FILENO, &original_) != 0) {
      return false;
    }

    input_flags_ = fcntl(STDIN_FILENO, F_GETFL);
    output_flags_ = fcntl(STDOUT_FILENO, F_GETFL);

    if (input_flags_ == -1 || output_flags_ == -1) {
      return false;
    }

    auto mode = original_;

    mode.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ECHONL | ISIG | IEXTEN));
    mode.c_iflag &= static_cast<tcflag_t>(~(ICRNL | INLCR | IGNCR | IXON));
    mode.c_cc[VMIN] = 0;
    mode.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) != 0) {
      return false;
    }

    termios_changed_ = true;

    if (fcntl(STDIN_FILENO, F_SETFL, input_flags_ | O_NONBLOCK) == -1) {
      return false;
    }

    input_changed_ = true;

    if (fcntl(STDOUT_FILENO, F_SETFL, output_flags_ | O_NONBLOCK) == -1) {
      return false;
    }

    output_changed_ = true;

    return true;
  }

  bool restore() {
    bool restored = true;

    if (output_changed_) {
      output_changed_ = fcntl(STDOUT_FILENO, F_SETFL, output_flags_) == -1;
      restored &= !output_changed_;
    }

    if (input_changed_) {
      input_changed_ = fcntl(STDIN_FILENO, F_SETFL, input_flags_) == -1;
      restored &= !input_changed_;
    }

    if (termios_changed_) {
      termios_changed_ = tcsetattr(STDIN_FILENO, TCSANOW, &original_) != 0;
      restored &= !termios_changed_;
    }

    return restored;
  }

  ~ModeGuard() {
    restore();
  }

private:
  termios original_{};
  int input_flags_ = -1;
  int output_flags_ = -1;
  bool termios_changed_ = false;
  bool input_changed_ = false;
  bool output_changed_ = false;
};

int waitReady(int descriptor, short events, QueryCollector::Time deadline) {
  while (interrupted == 0) {
    const auto remaining = deadline - std::chrono::steady_clock::now();

    if (remaining <= QueryCollector::Time::duration::zero()) {
      return 0;
    }

    pollfd request{descriptor, events, 0};
    const auto milliseconds = std::chrono::ceil<std::chrono::milliseconds>(remaining).count();
    const int result = poll(&request, 1, static_cast<int>(milliseconds));

    if (result < 0 && errno == EINTR) {
      continue;
    }

    if (result > 0 && (request.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      errno = EIO;

      return -1;
    }

    return result;
  }

  errno = EINTR;

  return -1;
}

RunResult collect(QueryCollector &collector, QueryCollector::Time deadline, std::size_t byte_limit) {
  std::size_t written = 0;

  while (written < REQUESTS.size()) {
    const int ready = waitReady(STDOUT_FILENO, POLLOUT, deadline);

    if (ready == 0) {
      return {"deadline during query write", "", 0, ""};
    }

    if (ready < 0) {
      return {"I/O error", error("poll output"), 0, ""};
    }

    if (std::chrono::steady_clock::now() >= deadline) {
      return {"deadline during query write", "", 0, ""};
    }

    const auto count = write(STDOUT_FILENO, REQUESTS.data() + written, REQUESTS.size() - written);

    if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    }

    if (count <= 0) {
      return {"I/O error", error("write query"), 0, ""};
    }

    written += static_cast<std::size_t>(count);
  }

  while (collector.active(std::chrono::steady_clock::now())) {
    const int ready = waitReady(STDIN_FILENO, POLLIN, deadline);

    if (ready == 0) {
      break;
    }

    if (ready < 0) {
      return {"I/O error", error("poll input"), 0, ""};
    }

    const auto received_at = std::chrono::steady_clock::now();

    if (!collector.active(received_at)) {
      break;
    }

    char bytes[READ_CHUNK_SIZE];
    const auto capacity = std::min(sizeof(bytes), byte_limit - collector.received());
    const auto count = read(STDIN_FILENO, bytes, capacity);

    if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    }

    if (count < 0) {
      return {"I/O error", error("read input"), 0, ""};
    }

    if (count == 0) {
      return {"input closed", "", 0, ""};
    }

    collector.feed(std::string_view(bytes, static_cast<std::size_t>(count)), received_at);
  }

  return {collector.limited()                                          ? "byte limit"
          : collector.deviceAttributes() && collector.cursorPosition() ? "replies"
                                                                       : "deadline",
          "", 0, ""};
}
} // namespace

NativeFacts inspectNative() {
  NativeFacts facts;
  utsname system{};

  if (uname(&system) == 0) {
    facts.fields.emplace_back("os.name", system.sysname);
    facts.fields.emplace_back("os.version", std::format("{} {}", system.release, system.version));
    facts.fields.emplace_back("os.architecture", system.machine);
  } else {
    facts.fields.emplace_back("os.name", "POSIX (uname unavailable)");
    facts.fields.emplace_back("os.version", "unknown");
  }

  const bool input_tty = isatty(STDIN_FILENO) == 1;
  const bool output_tty = isatty(STDOUT_FILENO) == 1;

  facts.query_eligible = input_tty && output_tty;
  facts.fields.emplace_back("transport.stdin", input_tty ? "POSIX tty" : "redirected/non-tty");
  facts.fields.emplace_back("transport.stdout", output_tty ? "POSIX tty" : "redirected/non-tty");
  facts.fields.emplace_back("transport.underlying", "unknown (tty API does not prove local/PTY/SSH/multiplexer)");

  const char *input_name = input_tty ? ttyname(STDIN_FILENO) : nullptr;

  facts.fields.emplace_back("transport.tty_name", input_name == nullptr ? "unknown" : input_name);

  winsize size{};

  if (output_tty && ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col != 0 && size.ws_row != 0) {
    facts.fields.emplace_back("size.columns", std::to_string(size.ws_col));
    facts.fields.emplace_back("size.rows", std::to_string(size.ws_row));
  } else {
    facts.fields.emplace_back("size.columns", "unknown");
    facts.fields.emplace_back("size.rows", "unknown");
  }

  termios mode{};

  if (input_tty && tcgetattr(STDIN_FILENO, &mode) == 0) {
    facts.fields.emplace_back("mode.stdin.iflag", std::to_string(mode.c_iflag));
    facts.fields.emplace_back("mode.stdin.oflag", std::to_string(mode.c_oflag));
    facts.fields.emplace_back("mode.stdin.cflag", std::to_string(mode.c_cflag));
    facts.fields.emplace_back("mode.stdin.lflag", std::to_string(mode.c_lflag));
    facts.fields.emplace_back("mode.stdin.canonical", (mode.c_lflag & ICANON) != 0 ? "true" : "false");
    facts.fields.emplace_back("mode.stdin.echo", (mode.c_lflag & ECHO) != 0 ? "true" : "false");
  } else {
    facts.fields.emplace_back("mode.stdin", "unknown");
  }

  return facts;
}

RunResult runNativeQueries(QueryCollector &collector, QueryCollector::Time deadline, std::size_t byte_limit) {
  if (isatty(STDIN_FILENO) != 1 || isatty(STDOUT_FILENO) != 1) {
    return {"skipped: redirected handles", "", 0, ""};
  }

  SignalGuard signals;

  if (!signals.acquire()) {
    return {"signal setup error", error("sigaction"), 0, ""};
  }

  ModeGuard modes;
  RunResult result;

  if (!modes.acquire()) {
    result = {"mode setup error", error("temporary terminal modes"), 0, ""};
  } else {
    result = collect(collector, deadline, byte_limit);
  }

  if (!modes.restore()) {
    result.error += "; " + error("restore terminal modes");
    result.stop = "restore error";
  }

  return result;
}
} // namespace portals::probe
