// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include "posix_io.hpp"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <csignal>
#include <pthread.h>
#include <unistd.h>

namespace portals::detail {
namespace {
TerminalDeadline now() {
  return std::chrono::steady_clock::now();
}

TerminalError systemError(int code) {
  return {TerminalErrorKind::SYSTEM, code, 0};
}
} // namespace

const PosixIo &nativePosixIo() noexcept {
  static const PosixIo IO{::poll, ::read, writeWithoutSigpipe, now};

  return IO;
}

int pollTimeout(TerminalDeadline deadline, std::chrono::milliseconds cadence, TerminalDeadline current) noexcept {
  if (deadline <= current) {
    return 0;
  }

  // Avoid time_point subtraction overflow for extreme user-supplied deadlines.
  const auto remaining = std::chrono::duration<long double, std::milli>(deadline.time_since_epoch()).count() -
                         std::chrono::duration<long double, std::milli>(current.time_since_epoch()).count();
  const auto bounded = std::min(remaining, static_cast<long double>(cadence.count()));
  const auto whole = static_cast<int>(std::min(bounded, static_cast<long double>(INT_MAX)));

  return whole + (whole < INT_MAX && static_cast<long double>(whole) < bounded ? 1 : 0);
}

ssize_t writeWithoutSigpipe(int descriptor, const void *bytes, std::size_t count) noexcept {
  sigset_t blocked{};
  sigset_t previous{};
  sigset_t pending{};

  sigemptyset(&blocked);
  sigaddset(&blocked, SIGPIPE);

  const int mask_error = pthread_sigmask(SIG_BLOCK, &blocked, &previous);

  if (mask_error != 0) {
    errno = mask_error;

    return -1;
  }

  if (sigpending(&pending) != 0) {
    const int error = errno;

    pthread_sigmask(SIG_SETMASK, &previous, nullptr);
    errno = error;

    return -1;
  }

  const bool already_pending = sigismember(&pending, SIGPIPE) == 1;
  const auto written = ::write(descriptor, bytes, count);
  const int error = errno;

  if (written < 0 && error == EPIPE && !already_pending) {
    const timespec no_wait{};

    while (sigtimedwait(&blocked, nullptr, &no_wait) < 0 && errno == EINTR) {
    }
  }

  pthread_sigmask(SIG_SETMASK, &previous, nullptr);
  errno = error;

  return written;
}

TerminalRead readPosix(int descriptor, std::size_t limit, TerminalDeadline deadline, std::stop_token stop,
                       const PosixIo &io) {
  TerminalRead result;

  result.bytes.resize(limit);

  for (;;) {
    if (stop.stop_requested() || io.now() >= deadline) {
      result.status = stop.stop_requested() ? TerminalStatus::STOPPED : TerminalStatus::DEADLINE;
      result.bytes.clear();

      return result;
    }

    const auto count = io.read(descriptor, result.bytes.data(), result.bytes.size());

    if (count >= 0) {
      result.bytes.resize(static_cast<std::size_t>(count));
      result.status = count == 0 ? TerminalStatus::END_OF_INPUT : TerminalStatus::PROGRESS;

      return result;
    }

    if (errno == EINTR) {
      continue;
    }

    const int error = errno;

    result.bytes.clear();
    result.status = TerminalStatus::FAILED;
    result.error = systemError(error);

    return result;
  }
}

TerminalWrite flushPosix(int descriptor, const std::string &bytes, std::size_t &offset, TerminalDeadline deadline,
                         std::stop_token stop, std::chrono::milliseconds cadence, const PosixIo &io) {
  TerminalWrite result;
  const auto initial = offset;

  while (offset < bytes.size()) {
    if (stop.stop_requested() || io.now() >= deadline) {
      result.status = stop.stop_requested() ? TerminalStatus::STOPPED : TerminalStatus::DEADLINE;

      break;
    }

    const auto count = io.write(descriptor, bytes.data() + offset, bytes.size() - offset);

    if (count > 0) {
      offset += static_cast<std::size_t>(count);

      continue;
    }

    const int error = count == 0 ? EIO : errno;

    if (error == EINTR) {
      continue;
    }

    if (error != EAGAIN && error != EWOULDBLOCK) {
      result.status = TerminalStatus::FAILED;
      result.error = systemError(error);

      break;
    }

    pollfd descriptor_state{descriptor, POLLOUT, 0};
    const int waited = io.poll(&descriptor_state, 1, pollTimeout(deadline, cadence, io.now()));

    if (waited < 0 && errno != EINTR) {
      result.status = TerminalStatus::FAILED;
      result.error = systemError(errno);

      break;
    }

    if (waited > 0 && (descriptor_state.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      result.status = TerminalStatus::FAILED;
      result.error = systemError((descriptor_state.revents & POLLNVAL) != 0 ? EBADF : EPIPE);

      break;
    }
  }

  result.completed = offset - initial;
  result.pending = bytes.size() - offset;

  return result;
}

} // namespace portals::detail
