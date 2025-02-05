// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <portals.hpp>
#include <thread>
#include <mutex>

using namespace std::literals;

namespace ot = org::ttldtor;
namespace otp = org::ttldtor::portals;

int main() {
  std::cout << ot::VirtualTerminal::enable() << std::endl;
  std::cout << ot::VirtualTerminal::enable() << std::endl;

  for (std::uint16_t i = 0; i < 256; i++) {
    std::cout << std::format("{}{} ", otp::setColor(i), i);
  }

  std::cout << otp::resetFormat() << std::endl;

  auto width = 50;

  std::cout << otp::hideCursor();

  std::mutex ioMutex{};

  std::atomic<bool> stop{};
  std::thread t{[width, &stop, &ioMutex] {
    auto minId = 233;
    auto maxId = 255;

    for (int i = 0; i < 1000000; i++) {
      {
        std::lock_guard<std::mutex> lock(ioMutex);

        for (int j = 0; j < width; j++) {
          if (stop) {
            return;
          }

          auto id = static_cast<std::uint8_t>(
            std::floor(minId + (maxId - minId) * (std::sin(((i + j) / (width / 2.0)) * 3.14) + 1.0) / 2.0));

          std::cout << std::format("{}#", otp::setColor(id));
        }

        std::cout << otp::moveToLineBegin() << otp::resetFormat();
      }

      std::this_thread::sleep_for(50ms);
    }
  }};

  std::thread blink{[&stop, &ioMutex]{
    for (int i = 0; i < 1000000; i++) {
      if (stop) {
        return;
      }

      {
        std::lock_guard<std::mutex> lock(ioMutex);

        if (i % 2 == 0) {
          std::cout << otp::saveCursorPos() + otp::writeTo(13, 5, otp::setColor(otp::Color::BLUE) + " RED ") +
                         otp::writeTo(14, 10, otp::resetFormat() + "     ") + otp::restoreCursorPos();
        } else {
          std::cout << otp::saveCursorPos() + otp::writeTo(13, 5, otp::resetFormat() + "     ") +
                         otp::writeTo(14, 10, otp::setColor(otp::Color::RED) + " BLUE") + otp::restoreCursorPos();
        }
      }

      std::this_thread::sleep_for(1s);
    }
  }};

  std::cin.ignore();
  std::cout << otp::resetFormat();
  stop = true;
  std::cout << otp::showCursor();

  t.join();
  blink.join();
}