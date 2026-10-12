// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include "../../examples/gallery/common.hpp"
#include <portals/line_edit.hpp>
#include <portals/style.hpp>

namespace gallery_before {
// Lifetime is visible: Application owns UI services; this scene owns their bindings and models.
class Form : public portals::ApplicationScene {
public:
  explicit Form(portals::Application &app) : app_(app), styles_(app.tree()) {
    using namespace portals;
    using example::take;
    using example::text;
    auto &tree = app.tree();
    const auto column = take(tree.emplace<Column>(app.root()));
    title_ = take(tree.emplace<Label>(column, text("Portals gallery | F2 theme | F10 quit")));
    label_ = take(tree.emplace<Label>(column, text("Name (Enter commits; Tab changes focus):")));
    editor_ = take(tree.emplace<LineEdit>(column));
    check_ = take(tree.emplace<CheckBox>(column, text("Alerts")));
    button_ = take(tree.emplace<Button>(column, text("Run (no model change)")));
    quit_ = take(tree.emplace<Button>(column, text("Quit")));
    name_ = take(Property<std::string>::create(tree, column, "Portals", Invalidation::PAINT));
    enabled_ = take(Property<bool>::create(tree, column, false, Invalidation::PAINT));
    editBinding_ = take(bindLineEdit(tree, app.router(), editor_, name_, [](LineEditResult) {
    }));
    checkBinding_ = take(bindCheckbox(tree, app.router(), check_, enabled_, [](CheckBoxResult result) {
      take(std::move(result));
    }));
    buttonBinding_ = take(bindButton(app.router(), button_, [&] {
      ++calls;
    }));
    quitBinding_ = take(bindButton(app.router(), quit_, [&] {
      app_.requestStop();
    }));
    take(app.router().setFocus(editor_));

    for (auto node : {Node<Widget>(title_), Node<Widget>(label_), Node<Widget>(editor_), Node<Widget>(check_),
                      Node<Widget>(button_), Node<Widget>(quit_)}) {
      take(styles_.bind(node, {.type = "Control"}));
    }

    take(styles_.replace(modernTheme()));
  }

  std::expected<void, portals::TreeError> prepare(portals::Application &app, portals::CellSize) override {
    const auto focused = example::take(app.router().focused());
    example::take(editBinding_->sync(focused));
    example::take(checkBinding_->sync());

    for (auto node : {portals::Node<portals::Control>(title_), portals::Node<portals::Control>(label_),
                      portals::Node<portals::Control>(check_), portals::Node<portals::Control>(button_),
                      portals::Node<portals::Control>(quit_)}) {
      example::take(portals::syncControl(app.tree(), node, focused));
    }

    return {};
  }

  std::expected<bool, portals::TreeError> input(portals::Application &app, const portals::InputEvent &event) override {
    if (example::quitKey(event)) {
      app.requestStop();

      return true;
    }

    if (const auto *key = std::get_if<portals::KeyEvent>(&event); key && key->key == portals::InputKey::F2) {
      turbo_ = !turbo_;
      example::take(styles_.replace(turbo_ ? portals::turboTheme() : portals::modernTheme()));

      return true;
    }

    return false;
  }

  int calls{};

private:
  portals::Application &app_;
  portals::StyleEngine styles_;
  portals::Node<portals::Label> title_, label_;
  portals::Node<portals::LineEdit> editor_;
  portals::Node<portals::CheckBox> check_;
  portals::Node<portals::Button> button_, quit_;
  std::shared_ptr<portals::Property<std::string>> name_;
  std::shared_ptr<portals::Property<bool>> enabled_;
  std::optional<portals::LineEditBinding> editBinding_;
  std::optional<portals::CheckBoxBinding> checkBinding_;
  portals::ActionBinding buttonBinding_, quitBinding_;
  bool turbo_{};
};
} // namespace gallery_before
