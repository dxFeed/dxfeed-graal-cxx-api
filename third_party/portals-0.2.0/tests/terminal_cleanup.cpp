// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/application.hpp>

#include "support/native_cleanup.hpp"
#include <charconv>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace {
class ErrorScene final : public portals::ApplicationScene {
public:
  std::expected<void, portals::TreeError> prepare(portals::Application &, portals::CellSize) override {
    throw std::runtime_error("native handled prepare failure");
  }
};

int errorProbe() {
  auto application = portals::Application::create();

  if (!application) {
    return 3;
  }

  ErrorScene scene;
  const auto result = (*application)->run(scene);

  if (result) {
    return 3;
  }

  const auto *cause = std::get_if<portals::ApplicationFailure>(&result.error().cause);

  if (result.error().phase != portals::ApplicationPhase::PREPARE || result.error().restoration || !cause ||
      *cause != portals::ApplicationFailure::CALLBACK_FAILED) {
    return 3;
  }

  return 1;
}

int number(std::string_view value) {
  int result{};
  const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);

  if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || result < 0) {
    throw std::runtime_error("invalid nonnegative numeric option");
  }

  return result;
}
} // namespace

int main(int argc, char **argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--error-probe") {
    return errorProbe();
  }

  try {
    native::Options options;
    std::string logPath;

    for (int i = 1; i < argc; ++i) {
      const std::string_view argument = argv[i];

      if (argument == "--") {
        for (++i; i < argc; ++i) {
          options.child.emplace_back(argv[i]);
        }

        break;
      }

      if (++i == argc) {
        return 2;
      }

      const std::string_view value = argv[i];

      if (argument == "--log") {
        logPath = value;
      } else if (argument == "--title") {
        options.title = value;
      } else if (argument == "--deadline-ms") {
        options.deadlineMs = number(value);
      } else if (argument == "--expected-exit") {
        options.expectedExit = number(value);
      } else if (argument == "--quit-ms") {
        options.quitMs = number(value);
      } else if (argument == "--drive" && (value == "none" || value == "dashboard")) {
        options.drive = value == "dashboard";
      } else {
        return 2;
      }
    }

    if (logPath.empty() || options.child.empty() || options.deadlineMs <= 0 ||
        (options.drive && (options.deadlineMs <= options.quitMs || options.quitMs < 4000))) {
      return 2;
    }

    std::ofstream log(logPath, std::ios::trunc);

    if (!log) {
      return 2;
    }

    const bool passed = native::supervise(options, log);
    log << (passed ? "PASS" : "FAIL") << ": native application supervisor\n";
    log.flush();
    // WT's default profile keeps failed commands open. The launcher must inspect
    // the verdict in the log; exit zero only makes this fixture's tab self-close.
    return 0;
  } catch (...) {
    return 2;
  }
}
