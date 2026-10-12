// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../gallery/managed_report.hpp"
#include "../replay/terminal.hpp"
#include "meter.hpp"

int main(int argc, char **argv) {
  try {
    const auto parsed = example::parseSceneOptions(argc, argv);

    if (!parsed || parsed->help) {
      if (!parsed) {
        std::cerr << parsed.error() << '\n';
      }

      example::printSceneUsage(std::cout, "portals_custom_widget");

      return parsed ? 0 : 2;
    }

    portals::ApplicationOptions options;

    options.writer.colorMode = parsed->ansi16 ? portals::ColorMode::ANSI16 : portals::ColorMode::TRUE_COLOR;

    example::ReplayTerminal terminal(
      {{50, "\x1b[C\x1b[C", {}}, {100, "", portals::CellSize{50, 8}}, {150, "\x1bOQ", {}}, {200, "\x1b[21~", {}}}, 250);
    auto app = example::take(
      portals::Application::create(options, parsed->check ? terminal.clock() : portals::SchedulerClock{}));
    meter_example::MeterScene scene(*app, parsed->theme);

    return example::report(parsed->check ? scene.run(terminal) : scene.run());
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';

    return 1;
  }
}
