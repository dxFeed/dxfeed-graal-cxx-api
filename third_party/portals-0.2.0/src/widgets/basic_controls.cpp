// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/controls.hpp>

#include <array>
#include <limits>

namespace portals {
namespace {
bool activates(const RoutedEvent &event) {
  if (const auto *action = std::get_if<ActionEvent>(&event)) {
    return action->action == Action::ACTIVATE && action->modifiers == InputModifiers{};
  }

  if (const auto *text = std::get_if<TextInputEvent>(&event)) {
    return text->scalar == U' ' && text->modifiers == InputModifiers{};
  }

  if (const auto *mouse = std::get_if<MouseInputEvent>(&event)) {
    return mouse->button == MouseButton::LEFT && mouse->action == MouseAction::PRESS &&
           mouse->modifiers == InputModifiers{};
  }

  return false;
}

GraphemeText decoration(std::string_view text, WidthPolicy policy) {
  return GraphemeText::from(text, policy).value();
}
} // namespace

Control::Control(GraphemeText text, std::string_view prefix, std::string_view suffix, ControlStyle style,
                 SizePolicy policy)
    : SizedWidget(policy), text_(std::move(text)), prefix_(decoration(prefix, text_.policy())),
      checked_prefix_(decoration(prefix == "[ ] " ? "[x] " : prefix, text_.policy())),
      suffix_(decoration(suffix, text_.policy())), style_(style) {
}

Label::Label(GraphemeText text, ControlStyle style, SizePolicy policy)
    : Control(std::move(text), "", "", style, policy) {
}

Button::Button(GraphemeText text, ControlStyle style, SizePolicy policy)
    : Control(std::move(text), "[ ", " ]", style, policy) {
}

CheckBox::CheckBox(GraphemeText text, ControlStyle style, SizePolicy policy)
    : Control(std::move(text), "[ ] ", "", style, policy) {
}

std::expected<CellSize, TreeError> Control::measure(CellSize) {
  const auto extra = prefix_.measure() + suffix_.measure();

  if (text_.measure() > std::numeric_limits<std::int32_t>::max() - extra) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  return CellSize{text_.measure() + extra, 1};
}

std::expected<void, TreeError> Control::arrange(CellRect local) {
  auto bounds = CellRect::from({}, local.size());

  if (!bounds) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  bounds_ = *bounds;

  return {};
}

std::expected<void, TreeError> Control::paint(PaintContext &context) {
  const auto width = measure({});

  if (!width) {
    return std::unexpected(width.error());
  }

  // Paint uses only synchronized presentation. The frame owner transforms these local
  // commands and renders their borrowed text before releasing the enclosing tree frame.
  const auto &prefix = checked_ ? checked_prefix_ : prefix_;
  const auto resolved = computedStyle() ? *computedStyle() : Control::styleDefaults();
  const auto style = !enabled_ ? resolved.disabled : focused_ ? resolved.focused : resolved.normal;
  const std::array<RenderCommand, 4> commands{
    FillCommand{bounds_, bounds_, style}, TextCommand{{}, prefix, bounds_, style},
    TextCommand{{prefix.measure(), 0}, text_, bounds_, style},
    TextCommand{{width->width - suffix_.measure(), 0}, suffix_, bounds_, style}};

  return context.submit(commands);
}

ComputedStyle Control::styleDefaults() const {
  ComputedStyle result;

  result.normal = style_.normal;
  result.focused = style_.focused;
  result.disabled = style_.disabled;

  return result;
}

const GraphemeText &Control::text() const {
  return text_;
}

bool Control::focused() const {
  return focused_;
}

bool Control::enabled() const {
  return enabled_;
}

void Control::showChecked(bool checked) {
  checked_ = checked;
}

bool Control::shownChecked() const {
  return checked_;
}

bool CheckBox::checked() const {
  return shownChecked();
}

void CheckBox::display(bool value) {
  showChecked(value);
}

std::expected<void, TreeError> setControlText(WidgetTree &tree, Node<Control> node, GraphemeText text) {
  std::expected<void, TreeError> result;
  auto edited = tree.edit(node, [&](Control &control) {
    if (control.text_.bytes() == text.bytes() && control.text_.policy() == text.policy()) {
      return;
    }

    try {
      // Prepare every owning decoration before invalidating or replacing storage.
      // Allocation failure must preserve both the old caption and the pending frame request.
      auto prefix = decoration(control.prefix_.bytes(), text.policy());
      auto checked = decoration(control.checked_prefix_.bytes(), text.policy());
      auto suffix = decoration(control.suffix_.bytes(), text.policy());

      result = tree.invalidate(node, Invalidation::LAYOUT);

      if (!result) {
        return;
      }

      control.text_ = std::move(text);
      control.prefix_ = std::move(prefix);
      control.checked_prefix_ = std::move(checked);
      control.suffix_ = std::move(suffix);
    } catch (const std::bad_alloc &) {
      result = std::unexpected(TreeError::NO_MEMORY);
    }
  });

  return edited ? result : edited;
}

std::expected<void, TreeError> setControlStyle(WidgetTree &tree, Node<Control> node, ControlStyle style) {
  std::expected<void, TreeError> result;
  auto edited = tree.edit(node, [&](Control &control) {
    if (control.style_ == style) {
      return;
    }

    result = tree.invalidate(node, Invalidation::PAINT);

    if (result) {
      control.style_ = style;
    }
  });

  return edited ? result : edited;
}

std::expected<void, TreeError> syncControl(WidgetTree &tree, Node<Control> node, Node<Widget> focused) {
  auto eligible = tree.inputEligible(node);

  if (!eligible) {
    return std::unexpected(eligible.error());
  }

  const bool has_focus = *eligible && node.id() == focused.id();
  std::expected<void, TreeError> result;
  auto edited = tree.edit(node, [&](Control &control) {
    if (control.focused_ == has_focus && control.enabled_ == *eligible) {
      return;
    }

    result = tree.invalidate(node, Invalidation::PAINT);

    if (result) {
      control.focused_ = has_focus;
      control.enabled_ = *eligible;
    }
  });

  return edited ? result : edited;
}

std::expected<ActionBinding, TreeError> bindButton(EventRouter &router, Node<Button> node,
                                                   std::function<void()> action) {
  if (!action) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  return router.bind(node, [action = std::move(action)](Button &, const RoutedEvent &event) {
    if (!activates(event)) {
      return EventReply::IGNORED;
    }

    action();

    return EventReply::HANDLED;
  });
}

struct CheckBoxBinding::State {
  WidgetTree &tree;
  Node<CheckBox> node;
  std::shared_ptr<Property<bool>> model;

  std::expected<void, TreeError> apply(bool value) {
    std::expected<void, TreeError> result;
    auto edited = tree.edit(node, [&](CheckBox &control) {
      if (control.checked() == value) {
        return;
      }

      result = tree.invalidate(node, Invalidation::PAINT);

      if (result) {
        control.display(value);
      }
    });

    return edited ? result : edited;
  }

  std::expected<void, TreeError> sync() {
    auto snapshot = model->snapshot();

    if (!snapshot) {
      return std::unexpected(snapshot.error());
    }

    return apply((*snapshot)->value);
  }
};

CheckBoxBinding::CheckBoxBinding(std::shared_ptr<State> state, ActionBinding action, Subscription subscription)
    : state_(std::move(state)), action_(std::move(action)), subscription_(std::move(subscription)) {
}

CheckBoxBinding::~CheckBoxBinding() = default;
CheckBoxBinding::CheckBoxBinding(CheckBoxBinding &&) noexcept = default;

CheckBoxBinding &CheckBoxBinding::operator=(CheckBoxBinding &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  // Install the complete replacement before retiring old resources. Reclaimed captures
  // may reenter UI code and destroy the destination token; no partial state may be visible.
  CheckBoxBinding retired(std::move(other));

  using std::swap;

  swap(state_, retired.state_);
  swap(action_, retired.action_);
  swap(subscription_, retired.subscription_);

  return *this;
}

std::expected<void, TreeError> CheckBoxBinding::sync() {
  if (!state_) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  return state_->sync();
}

std::expected<CheckBoxBinding, TreeError> bindCheckbox(WidgetTree &tree, EventRouter &router, Node<CheckBox> node,
                                                       std::shared_ptr<Property<bool>> model,
                                                       std::function<void(CheckBoxResult)> changed) {
  if (!model || !changed) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  try {
    auto state = std::make_shared<CheckBoxBinding::State>(CheckBoxBinding::State{tree, node, std::move(model)});

    // Ignore the queued event value: delayed delivery must not roll back a newer model commit.
    auto subscription = state->model->observe(node, [state](CheckBox &, auto) {
      auto synced = state->sync();

      if (!synced) {
        throw synced.error();
      }
    });

    if (!subscription) {
      return std::unexpected(subscription.error());
    }

    auto action = router.bind(node, [state, changed = std::move(changed)](CheckBox &, const RoutedEvent &event) {
      if (!activates(event)) {
        return EventReply::IGNORED;
      }

      // Read the authoritative value once and perform one toggle. Retain this exact set
      // receipt, including rejected/cancelled notifications, for the business callback.
      auto current = state->model->snapshot();
      CheckBoxResult result = current ? state->model->set(!(*current)->value) : std::unexpected(current.error());

      if (result) {
        auto synced = state->apply(result->snapshot->value);

        // A property observer capture can remove the target while set prunes observers.
        // Removal ends presentation, but must not conceal a successful model commit.
        if (!synced && synced.error() != TreeError::STALE_NODE && synced.error() != TreeError::CLOSED) {
          throw synced.error();
        }
      }

      // A callback exception cannot undo the model commit or replace its receipt.
      changed(std::move(result));

      return EventReply::HANDLED;
    });

    if (!action) {
      return std::unexpected(action.error());
    }

    auto synced = state->sync();

    if (!synced) {
      return std::unexpected(synced.error());
    }

    return CheckBoxBinding(std::move(state), std::move(*action), std::move(*subscription));
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}
} // namespace portals
