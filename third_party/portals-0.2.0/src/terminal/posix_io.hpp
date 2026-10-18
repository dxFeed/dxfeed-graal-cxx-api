// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <portals/terminal.hpp>

#include <poll.h>
#include <sys/types.h>

namespace portals::detail {

// Narrow syscall seam: tests script interruptions, short writes and elapsed time.
struct PosixIo {
  int (*poll)(pollfd *, nfds_t, int);
  ssize_t (*read)(int, void *, std::size_t);
  ssize_t (*write)(int, const void *, std::size_t);
  TerminalDeadline (*now)();
};

const PosixIo &nativePosixIo() noexcept;
int pollTimeout(TerminalDeadline deadline, std::chrono::milliseconds cadence, TerminalDeadline now) noexcept;
ssize_t writeWithoutSigpipe(int descriptor, const void *bytes, std::size_t count) noexcept;
TerminalRead readPosix(int descriptor, std::size_t limit, TerminalDeadline deadline, std::stop_token stop,
                       const PosixIo &io);
TerminalWrite flushPosix(int descriptor, const std::string &bytes, std::size_t &offset, TerminalDeadline deadline,
                         std::stop_token stop, std::chrono::milliseconds cadence, const PosixIo &io);

} // namespace portals::detail
