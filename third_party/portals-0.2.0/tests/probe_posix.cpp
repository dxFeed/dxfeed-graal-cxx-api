// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace {
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
constexpr std::string_view QUERIES = "\x1b[0c\x1b[6n";

class ChildGuard {
public:
  explicit ChildGuard(pid_t process) : process_(process) {
  }

  ~ChildGuard() {
    if (process_ > 0) {
      // The PID belongs to this fixture. Never signal other processes.
      kill(process_, SIGKILL);
      waitpid(process_, nullptr, 0);
    }
  }

  void reaped() {
    process_ = -1;
  }

private:
  pid_t process_;
};

class Descriptor {
public:
  explicit Descriptor(int value) : value_(value) {
  }

  ~Descriptor() {
    if (value_ >= 0) {
      close(value_);
    }
  }

  int get() const {
    return value_;
  }

private:
  int value_;
};

bool sameModes(const termios &left, const termios &right) {
  if (left.c_iflag != right.c_iflag || left.c_oflag != right.c_oflag || left.c_cflag != right.c_cflag ||
      left.c_lflag != right.c_lflag || cfgetispeed(&left) != cfgetispeed(&right) ||
      cfgetospeed(&left) != cfgetospeed(&right)) {
    return false;
  }

  for (std::size_t index = 0; index < NCCS; ++index) {
    if (left.c_cc[index] != right.c_cc[index]) {
      return false;
    }
  }

  return true;
}

bool scenario(const char *executable, std::string_view name, std::string_view response, std::string_view expected,
              bool signal_child = false) {
  Descriptor master(posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK));

  if (master.get() < 0 || grantpt(master.get()) != 0 || unlockpt(master.get()) != 0) {
    std::cerr << "FAIL: PTY allocation\n";

    return false;
  }

  const char *slave_name = ptsname(master.get());
  Descriptor slave(slave_name == nullptr ? -1 : open(slave_name, O_RDWR | O_NOCTTY));
  termios before{};

  if (slave.get() < 0 || tcgetattr(slave.get(), &before) != 0) {
    std::cerr << "FAIL: PTY termios snapshot\n";

    return false;
  }

  const int flags_before = fcntl(slave.get(), F_GETFL);
  const auto start = Clock::now();
  const pid_t process = fork();

  if (process == 0) {
    if (dup2(slave.get(), STDIN_FILENO) < 0 || dup2(slave.get(), STDOUT_FILENO) < 0 ||
        dup2(slave.get(), STDERR_FILENO) < 0) {
      _exit(120);
    }

    close(master.get());
    close(slave.get());

    const char *limit = name == "byte limit" ? "8" : "128";

    execl(executable, executable, "--timeout-ms", "80", "--max-bytes", limit, static_cast<char *>(nullptr));
    _exit(121);
  }

  if (process < 0) {
    std::cerr << "FAIL: fork\n";

    return false;
  }

  ChildGuard child(process);
  std::string output;
  bool responded = false;
  bool exited = false;
  int status = 0;

  while (Clock::now() - start < 3s) {
    pollfd descriptor{master.get(), POLLIN, 0};
    const int ready = poll(&descriptor, 1, 10);

    if (ready > 0 && (descriptor.revents & POLLIN) != 0) {
      char buffer[1024];
      const auto count = read(master.get(), buffer, sizeof(buffer));

      if (count > 0) {
        output.append(buffer, static_cast<std::size_t>(count));
      }
    }

    if (!responded && output.starts_with(QUERIES)) {
      responded = true;

      if (signal_child) {
        kill(process, SIGTERM);
      } else if (!response.empty() &&
                 write(master.get(), response.data(), response.size()) != static_cast<ssize_t>(response.size())) {
        std::cerr << "FAIL: PTY response write\n";

        return false;
      }
    }

    if (!exited) {
      const auto result = waitpid(process, &status, WNOHANG);

      if (result == process) {
        exited = true;
        child.reaped();
      }
    }

    if (exited && ready == 0) {
      break;
    }
  }

  termios after{};
  const bool restored =
    tcgetattr(slave.get(), &after) == 0 && sameModes(before, after) && fcntl(slave.get(), F_GETFL) == flags_before;
  const int expected_exit = signal_child ? 1 : 0;
  const bool pass = exited && WIFEXITED(status) && WEXITSTATUS(status) == expected_exit && responded && restored &&
                    output.find(expected) != std::string::npos;

  if (!pass) {
    std::cerr << "FAIL: " << name << " exited=" << exited << " restored=" << restored << " status=" << status << '\n';
  } else {
    std::cout << "PASS: synthetic PTY " << name << " and mode/descriptor restoration\n";
  }

  return pass;
}
} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    return 2;
  }

  bool passed = scenario(argv[1], "matching replies", "typed\x1b[?62;1;6c\x1b[12;34R", "input.unmatched=\"typed\"");

  passed &= scenario(argv[1], "deadline", "", "query.stop=\"deadline\"");
  passed &= scenario(argv[1], "byte limit", "abcdefghij", "input.bytes_consumed=\"8\"");
  passed &= scenario(argv[1], "signal", "", "query.stop=\"I/O error\"", true);

  return passed ? 0 : 1;
}
