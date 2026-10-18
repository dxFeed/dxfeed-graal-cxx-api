// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include "../../examples/gallery/common.hpp"
#include "../../examples/gallery/options.hpp"
#include <format>
#include <portals/model.hpp>
#include <portals/ui.hpp>

namespace meter_before {
using namespace portals;
using example::take;
inline constexpr int MIN_LEVEL = 0;
inline constexpr int MAX_LEVEL = 100;
inline constexpr int LEVEL_STEP = 5;
inline constexpr int BAR_TOP = 2;

// The immutable provider stays independent of both Widget and managed composition.
class LevelSnapshot final : public ModelSnapshot<int> {
public:
  LevelSnapshot(std::uint64_t revision, int value) : ModelSnapshot(revision, 1), row_{{1, 1}, value} {
  }

  ModelRows<int> read(RowRange range) const override {
    return {RangeState::READY, std::span(&row_, 1).subspan(static_cast<std::size_t>(range.first), range.count), {}};
  }

private:
  ModelRow<int> row_;
};

// Paint consumes retained presentation only; model reads and formatting belong to prepare.
class LevelMeter final : public Widget {
public:
  /** @param value Domain value in the inclusive MIN_LEVEL..MAX_LEVEL percent range.
   * @return Whether text/value changed, or a text construction error mapped to TreeError.
   */
  std::expected<bool, TreeError> prepare(int value) {
    if (initialized_ && value_ == value) {
      return false;
    }

    auto label = GraphemeText::from(std::format("Level {}% | Left/Right adjust | F2 theme | F10 quit", value));

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
  explicit MeterScene(Application &app, std::size_t theme = 0)
      : app_(app), theme_(theme), scene_(take(ManagedScene::create(app))) {
    meter_ =
      take(ui::mount(*scene_, ui::custom<LevelMeter>()
                                .style({.type = "Meter"})
                                .onPrepare([&](LevelMeter &meter, Node<Widget>) -> std::expected<void, TreeError> {
                                  auto pin = model_->pin();

                                  if (!pin) {
                                    return std::unexpected(pin.error());
                                  }

                                  auto value = pin->read({0, 1});

                                  if (!value) {
                                    return std::unexpected(value.error());
                                  }

                                  auto changed = meter.prepare(value->rows()[0].value);

                                  if (!changed) {
                                    return std::unexpected(changed.error());
                                  }

                                  return *changed ? app_.tree().invalidate(meter_, Invalidation::PAINT)
                                                  : std::expected<void, TreeError>{};
                                })));

    model_ = take(Model<int>::create(app.tree(), meter_, std::make_shared<const LevelSnapshot>(revision_, level_)));
    take(scene_->theme(example::THEME_PRESETS.at(theme_).make()));
    take(scene_->onInput([&](const InputEvent &event) -> std::expected<bool, ManagedError> {
      if (example::quitKey(event)) {
        app_.requestStop();

        return true;
      }

      const auto *key = std::get_if<KeyEvent>(&event);

      if (!key || key->action == KeyAction::RELEASE) {
        return false;
      }

      if (key->key == InputKey::F2) {
        const auto next = (theme_ + 1) % example::THEME_PRESETS.size();
        auto changed = scene_->theme(example::THEME_PRESETS[next].make());

        if (!changed) {
          return std::unexpected(changed.error());
        }

        theme_ = next;

        return true;
      }

      if (key->key != InputKey::LEFT && key->key != InputKey::RIGHT) {
        return false;
      }

      const auto next =
        std::clamp(level_ + (key->key == InputKey::RIGHT ? LEVEL_STEP : -LEVEL_STEP), MIN_LEVEL, MAX_LEVEL);
      auto published = model_->publisher().publish(std::make_shared<const LevelSnapshot>(revision_ + 1, next));

      if (!published) {
        return std::unexpected(ManagedError{.phase = ManagedPhase::INPUT,
                                            .node = meter_.id(),
                                            .cause = ApplicationError{ApplicationPhase::INPUT, published.error(), {}},
                                            .application = {},
                                            .cleanup = {}});
      }

      level_ = next;
      ++revision_;

      return true;
    }));
  }

  std::expected<ApplicationStats, ManagedError> run() {
    return scene_->run();
  }

  std::expected<ApplicationStats, ManagedError> run(ApplicationTerminal &terminal) {
    return scene_->run(terminal);
  }

private:
  Application &app_;
  int level_{50};
  std::uint64_t revision_{1};
  std::size_t theme_{};
  Node<LevelMeter> meter_;
  std::unique_ptr<Model<int>> model_;
  // Destroy captured adapters before their borrowed model and application references.
  std::unique_ptr<ManagedScene> scene_;
};
} // namespace meter_before
