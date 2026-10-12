// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "virtual_scene.hpp"

using namespace virtualConsumer;

int main() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  Dashboard dashboard(*app);
  std::size_t settledPins{};

  terminal.checkpoint = [&](std::size_t next, TerminalRead &read) {
    if (next == 0) {
      read.resized = RESIZED_SIZE;
    } else if (next == 1) {
      settledPins = dashboard.pins;
    } else {
      check(dashboard.pins == settledPins, "Idle scene prepared another frame");
    }
  };

  const auto stats = take(app->run(dashboard, terminal));

  check(stats.frames == 2 && dashboard.pins == stats.frames, "Expected one pin per initial/resize frame");
  check(dashboard.tableRequests.empty() && dashboard.chartRequests.empty(), "Resident provider requested work");
  reportCapture(terminal);

  return EXIT_SUCCESS;
}