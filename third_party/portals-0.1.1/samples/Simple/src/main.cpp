// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <mutex>
#include <portals.hpp>
#include <print>
#include <thread>

using namespace std::literals;

namespace ot = org::ttldtor;
namespace otp = org::ttldtor::portals;

int main() {
  std::println("{}", ot::VirtualTerminal::enable());
  std::println("{}", ot::VirtualTerminal::enable());

  for (std::uint16_t i = 0; i < 256; i++) {
    std::print("{}{} ", otp::setColor(i), i);
  }

  std::println("{}", otp::resetFormat());

  constexpr auto width = 50;

  std::print("{}", otp::hideCursor());

  std::mutex ioMutex{};

  std::atomic<bool> stop{};
  std::thread t{[width, &stop, &ioMutex] {
    for (int i = 0; i < 1000000; i++) {
      {
        std::lock_guard lock(ioMutex);

        for (auto j = 0; j < width; j++) {
          constexpr auto maxId = 255;
          constexpr auto minId = 233;

          if (stop) {
            return;
          }

          const auto id = static_cast<std::uint8_t>(
            std::floor(minId + (maxId - minId) * (std::sin((i + j) / (width / 2.0) * 3.14) + 1.0) / 2.0));

          std::print("{}#", otp::setColor(id));
        }

        std::print("{}{}", otp::moveToLineBegin(), otp::resetFormat());
      }

      std::this_thread::sleep_for(50ms);
    }
  }};

  std::thread blink{[&stop, &ioMutex] {
    for (int i = 0; i < 1000000; i++) {
      if (stop) {
        return;
      }

      {
        std::lock_guard lock(ioMutex);

        if (i % 2 == 0) {
          std::print("{}{}{}{}", otp::saveCursorPos(),
                     otp::writeTo(13, 5, std::format("{}{:^5s}", otp::setColor(otp::Color::BLUE), "RED")),
                     otp::writeTo(14, 10, std::format("{}{:5s}", otp::resetFormat(), "")), otp::restoreCursorPos());
        } else {
          std::print("{}{}{}{}", otp::saveCursorPos(),
                     otp::writeTo(13, 5, std::format("{}{:5s}", otp::resetFormat(), "")),
                     otp::writeTo(14, 10, std::format("{}{:^5s}", otp::setColor(otp::Color::RED), "BLUE")),
                     otp::restoreCursorPos());
        }
      }

      std::this_thread::sleep_for(1s);
    }
  }};

  std::cin.ignore();
  std::print("{}", otp::resetFormat());
  stop = true;
  std::print("{}", otp::showCursor());

  t.join();
  blink.join();
}