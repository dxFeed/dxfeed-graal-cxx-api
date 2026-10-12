// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include "../gallery/common.hpp"
#include "../gallery/options.hpp"
#include <format>
#include <portals/input_match.hpp>
#include <portals/text_format.hpp>
#include <portals/ui.hpp>

namespace meter_example {
using namespace portals;
using example::take;
inline constexpr int MIN_LEVEL = 0;
inline constexpr int MAX_LEVEL = 100;
inline constexpr int INITIAL_LEVEL = 50;
inline constexpr int LEVEL_STEP = 5;
inline constexpr int BAR_TOP = 2;

// Paint consumes retained presentation only; state reads and formatting belong to prepare.
class LevelMeter final : public Widget {
public:
  /** @param value Domain value in the inclusive MIN_LEVEL..MAX_LEVEL percent range.
   * @return Whether text/value changed, or a text construction error mapped to TreeError.
   */
  std::expected<bool, TreeError> prepare(int value) {
    if (initialized_ && value_ == value) {
      return false;
    }

    auto label = formatText("Level {}% | Left/Right adjust | F2 theme | F10 quit", value);

    if (!label) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    label_ = std::move(*label);
    value_ = value;
    initialized_ = true;

    return true;
  }

  std::expected<CellSize, TreeError> measure(CellSize available) override {
    return available;
  }

  std::expected<void, TreeError> arrange(CellRect rect) override {
    size_ = rect.size();

    return {};
  }

  std::expected<void, TreeError> paint(PaintContext &context) override {
    const auto area = take(CellRect::from({}, size_));
    const auto bar =
      take(CellRect::from({0, std::min(BAR_TOP, size_.height)},
                          {static_cast<std::int32_t>(static_cast<std::int64_t>(size_.width) * value_ / MAX_LEVEL),
                           std::max(0, size_.height - BAR_TOP)}));
    const auto styles = computedStyle().value_or(ComputedStyle{});
    const std::array<RenderCommand, 3> commands{FillCommand{area, area, styles.normal},
                                                FillCommand{bar, area, styles.focused},
                                                TextCommand{{}, label_, area, styles.normal}};

    return context.submit(commands);
  }

private:
  int value_{};
  bool initialized_{};
  CellSize size_{};
  GraphemeText label_ = example::text("");
};

class MeterScene {
public:
  /** @param app UI-affine application that must outlive this scene.
   * @param theme Initial index into example::THEME_PRESETS; invalid indices throw.
   */
  explicit MeterScene(Application &app, std::size_t theme = 0)
      : app_(app), theme_(theme), scene_(take(ManagedScene::create(app))) {
    level_ = take(scene_->state(INITIAL_LEVEL));

    meter_ =
      take(ui::mount(*scene_, ui::custom<LevelMeter>()
                                .style({.type = "Meter"})
                                .onPrepare([&](LevelMeter &meter, Node<Widget>) -> std::expected<void, TreeError> {
                                  return level_.snapshot()
                                    .and_then([&](const auto &snapshot) {
                                      return meter.prepare(snapshot->value);
                                    })
                                    .and_then([&](bool changed) -> std::expected<void, TreeError> {
                                      return changed ? app_.tree().invalidate(meter_, Invalidation::PAINT)
                                                     : std::expected<void, TreeError>{};
                                    });
                                })));

    take(scene_->theme(example::THEME_PRESETS.at(theme_).make()));
    take(scene_->onInput([&](const InputEvent &event) -> std::expected<bool, ManagedError> {
      if (example::quitKey(event)) {
        app_.requestStop();

        return true;
      }

      if (keyPressed(event, InputKey::F2)) {
        const auto next = (theme_ + 1) % example::THEME_PRESETS.size();
        auto changed = scene_->theme(example::THEME_PRESETS[next].make());

        if (!changed) {
          return std::unexpected(changed.error());
        }

        theme_ = next;

        return true;
      }

      if (!keyPressed(event, {InputKey::LEFT, InputKey::RIGHT})) {
        return false;
      }

      const auto step = keyPressed(event, InputKey::RIGHT) ? LEVEL_STEP : -LEVEL_STEP;
      return level_.snapshot()
        .and_then([&](const auto &current) {
          return level_.set(std::clamp(current->value + step, MIN_LEVEL, MAX_LEVEL));
        })
        .transform([](const auto &) {
          return true;
        })
        .transform_error([&](TreeError cause) {
          return ManagedError{
            .phase = ManagedPhase::INPUT, .node = meter_.id(), .cause = cause, .application = {}, .cleanup = {}};
        });
    }));
  }

  /** @return Runner statistics or the typed managed error, after native terminal cleanup. */
  std::expected<ApplicationStats, ManagedError> run() {
    return scene_->run();
  }

  /** @param terminal Borrowed terminal opened and closed by the managed runner.
   * @return Runner statistics or the typed managed error, including cleanup failures.
   */
  std::expected<ApplicationStats, ManagedError> run(ApplicationTerminal &terminal) {
    return scene_->run(terminal);
  }

private:
  Application &app_;
  ManagedState<int> level_;
  std::size_t theme_{};
  Node<LevelMeter> meter_;
  // Destroy captured adapters before their state handle and application reference.
  std::unique_ptr<ManagedScene> scene_;
};
} // namespace meter_example
