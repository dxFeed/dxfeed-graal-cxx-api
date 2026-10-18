// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../examples/dashboard/dashboard.hpp"
#include "../examples/replay/terminal.hpp"
#include "../tools/benchmarks/measurement.hpp"
#include <source_location>

namespace {
void check(bool value, std::source_location at = std::source_location::current()) {
  if (!value) {
    throw std::runtime_error("MVP acceptance line " + std::to_string(at.line()));
  }
}

void windowAndSamples() {
  benchmark::Samples samples(2);
  samples.add(10, false);
  samples.add(20, true);
  samples.add(30, true);
  check(samples.values() == std::vector<std::uint64_t>({20, 30}));
  check(samples.summary().p50 == 20 && samples.summary().p95 == 30);
  samples.add(40, true);
  check(samples.overflow() == 1 && !samples.summary().p95);
  check(!benchmark::Samples(2).summary().p50);
}

void exactFlushAccounting() {
  benchmark::OutputCounters output;
  output.accept(10);
  output.flush({portals::TerminalStatus::DEADLINE, 4, 6, {}});
  check(output.confirmed == 4 && output.completedFrames == 0 && !output.complete());
  output.flush({portals::TerminalStatus::PROGRESS, 6, 0, {}});
  check(output.confirmed == 10 && output.completedFrames == 1 && output.pending == 0 && output.complete());
  output.accept(8);
  output.flush(
    {portals::TerminalStatus::DEADLINE, 2, 6, portals::TerminalError{portals::TerminalErrorKind::OUTPUT_UNCERTAIN}});
  check(output.confirmed == 12 && output.completedFrames == 1 && output.uncertain == 1);
  output.flush({portals::TerminalStatus::PROGRESS, 6, 0, {}});
  check(output.completedFrames == 1 && !output.valid && !output.complete());
}

void realWaitAndLossless() {
  benchmark::HeadlessTerminal terminal;
  const auto begin = std::chrono::steady_clock::now();
  terminal.read(begin + std::chrono::milliseconds(15), {});
  check(std::chrono::steady_clock::now() - begin >= std::chrono::milliseconds(15));
  example::ReplayTerminal replay({}, 1000);
  portals::ApplicationOptions config;
  config.scheduler.data_capacity = config.scheduler.data_budget = 1;
  auto app = example::take(portals::Application::create(config, replay.clock()));
  dashboard::Options workload;
  workload.rate = 10000;
  dashboard::Scene scene(*app, workload);
  example::take(app->run(scene, replay));
  check(benchmark::verifyOhlc(scene.aggregate(), scene.tickCount(), workload.seed));
  check(scene.tickCount() == 10000 && scene.pendingTicks() == 0 && scene.coherent());
}
} // namespace

int main() {
  try {
    windowAndSamples();
    exactFlushAccounting();
    realWaitAndLossless();
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
