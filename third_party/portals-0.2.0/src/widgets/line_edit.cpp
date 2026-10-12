// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/line_edit.hpp>

#include <algorithm>
#include <array>
#include <limits>

namespace portals {
namespace {
bool hasLineSeparator(std::string_view bytes) {
  return bytes.find_first_of("\r\n") != std::string_view::npos || bytes.contains("\xe2\x80\xa8") ||
         bytes.contains("\xe2\x80\xa9");
}

LineEditResult failed(TreeError error) {
  return {LineEditStatus::FAILED, {}, error};
}

LineEditStatus textStatus(TextError error) {
  return error.kind == TextErrorKind::INVALID_UTF8 || error.kind == TextErrorKind::INVALID_POLICY
           ? LineEditStatus::INVALID_TEXT
           : LineEditStatus::LIMIT_EXCEEDED;
}

char32_t shortcutCharacter(const KeyEvent *key) {
  if (!key || key->key != InputKey::CHARACTER || key->action == KeyAction::RELEASE ||
      key->modifiers != InputModifiers{.control = true}) {
    return U'\0';
  }

  auto scalar = key->scalar;

  if (scalar >= U'A' && scalar <= U'Z') {
    scalar += U'a' - U'A';
  }

  return scalar == U'a' || scalar == U'c' || scalar == U'x' || scalar == U'v' ? scalar : U'\0';
}

bool acceptsAction(const ActionEvent *command) {
  if (!command || command->modifiers.alt || command->modifiers.control || command->modifiers.super) {
    return false;
  }

  switch (command->action) {
  case Action::MOVE_LEFT:
  case Action::MOVE_RIGHT:
  case Action::MOVE_HOME:
  case Action::MOVE_END:
    return true;
  case Action::ACTIVATE:
  case Action::CANCEL:
  case Action::ERASE_BACKWARD:
  case Action::ERASE_FORWARD:
    return !command->modifiers.shift;
  default:
    return false;
  }
}

struct EncodedScalar {
  std::array<char, 4> bytes{};
  std::size_t size{};

  std::string_view view() const noexcept {
    return {bytes.data(), size};
  }
};

EncodedScalar encode(char32_t scalar) noexcept {
  EncodedScalar result;
  const auto append = [&](char byte) {
    result.bytes[result.size++] = byte;
  };

  if (scalar <= 0x7f) {
    append(static_cast<char>(scalar));
  } else if (scalar <= 0x7ff) {
    append(static_cast<char>(0xc0 | (scalar >> 6)));
    append(static_cast<char>(0x80 | (scalar & 0x3f)));
  } else if (scalar <= 0xffff) {
    append(static_cast<char>(0xe0 | (scalar >> 12)));
    append(static_cast<char>(0x80 | ((scalar >> 6) & 0x3f)));
    append(static_cast<char>(0x80 | (scalar & 0x3f)));
  } else if (scalar <= 0x10ffff) {
    append(static_cast<char>(0xf0 | (scalar >> 18)));
    append(static_cast<char>(0x80 | ((scalar >> 12) & 0x3f)));
    append(static_cast<char>(0x80 | ((scalar >> 6) & 0x3f)));
    append(static_cast<char>(0x80 | (scalar & 0x3f)));
  } else {
    append('\xff');
  }

  return result;
}
} // namespace

LineEdit::LineEdit(LineEditOptions options, SizePolicy policy)
    : SizedWidget(policy), options_(options), text_(GraphemeText::from("", options.widthPolicy).value()) {
}

std::expected<CellSize, TreeError> LineEdit::measure(CellSize) {
  if (text_.measure() == std::numeric_limits<std::int32_t>::max()) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  return CellSize{text_.measure() + 1, 1};
}

std::expected<void, TreeError> LineEdit::arrange(CellRect local) {
  auto bounds = CellRect::from({}, local.size());

  if (!bounds) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  bounds_ = *bounds;
  revealCursor();

  return {};
}

std::int32_t LineEdit::cellAt(std::size_t byte) const {
  std::int32_t cells{};

  for (const auto &cluster : text_.graphemes()) {
    if (cluster.byte_begin >= byte) {
      break;
    }

    cells += cluster.cells;
  }

  return cells;
}

void LineEdit::revealCursor() {
  const auto width = bounds_.size().width;
  const auto caret = cellAt(cursor_);

  if (static_cast<std::int64_t>(text_.measure()) + 1 <= width) {
    scroll_ = 0;

    return;
  }
  auto caretWidth = 1;
  const auto atCursor = std::ranges::find(text_.graphemes(), cursor_, &Grapheme::byte_begin);

  if (atCursor != text_.graphemes().end() && atCursor->cells <= width) {
    caretWidth = atCursor->cells;
  }

  // Scroll origins are always complete grapheme boundaries. A viewport narrower than
  // the current glyph shows a styled blank caret, never half of a wide glyph.
  if (caret < scroll_) {
    scroll_ = caret;
  }

  std::int32_t origin{};

  for (const auto &cluster : text_.graphemes()) {
    if (width <= 0 || static_cast<std::int64_t>(caret) + caretWidth - scroll_ <= width) {
      break;
    }

    origin += cluster.cells;

    if (origin > scroll_) {
      scroll_ = origin;
    }
  }
}

std::expected<void, TreeError> LineEdit::paint(PaintContext &context) {
  const auto resolved = computedStyle() ? *computedStyle() : LineEdit::styleDefaults();
  const auto style = !enabled_ ? resolved.disabled : focused_ ? resolved.focused : resolved.normal;
  const auto selectionBegin = cellAt(std::min(cursor_, anchor_)) - scroll_;
  const auto selectionEnd = cellAt(std::max(cursor_, anchor_)) - scroll_;
  const auto selection = CellRect::from({selectionBegin, 0}, {selectionEnd - selectionBegin, 1}).value();
  const std::array<RenderCommand, 3> base{
    FillCommand{bounds_, bounds_, style}, TextCommand{{-scroll_, 0}, text_, bounds_, style},
    TextCommand{{-scroll_, 0}, text_, selection.intersection(bounds_), enabled_ ? resolved.selected : style}};
  auto submitted = context.submit(base);

  if (!submitted || !focused_ || !enabled_ || bounds_.empty()) {
    return submitted;
  }

  const auto caret = cellAt(cursor_) - scroll_;
  const auto cluster = std::ranges::find(text_.graphemes(), cursor_, &Grapheme::byte_begin);
  const auto width = cluster != text_.graphemes().end() && cluster->cells <= bounds_.size().width ? cluster->cells : 1;
  const auto area = CellRect::from({caret, 0}, {width, 1}).value();
  const std::array<RenderCommand, 2> cursor{
    FillCommand{area, bounds_, resolved.caret},
    TextCommand{{-scroll_, 0}, text_, area.intersection(bounds_), resolved.caret}};

  return context.submit(cursor);
}

ComputedStyle LineEdit::styleDefaults() const {
  ComputedStyle result;

  result.normal = options_.style.normal;
  result.focused = options_.style.focused;
  result.disabled = options_.style.disabled;
  result.selected = options_.selection;
  result.caret = options_.cursor;

  return result;
}

const GraphemeText &LineEdit::text() const {
  return text_;
}

std::size_t LineEdit::cursor() const {
  return cursor_;
}

std::size_t LineEdit::anchor() const {
  return anchor_;
}

bool LineEdit::dirty() const {
  return dirty_;
}

struct LineEditBinding::State {
  WidgetTree &tree;
  Node<LineEdit> node;
  std::shared_ptr<Property<std::string>> model;
  std::shared_ptr<Clipboard> clipboard;
  std::function<bool(std::string_view)> validate;
  std::uint64_t revision{};
  bool initialized{};
  bool busy{};

  std::expected<void, TreeError> alive() {
    return tree.withScope(node, [](const UiScope &) {
    });
  }

  std::expected<void, TreeError> adopt(LineEdit &edit, const PropertyChange<std::string> &snapshot) {
    if (hasLineSeparator(snapshot.value)) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    auto text = GraphemeText::from(snapshot.value, edit.options_.widthPolicy, edit.options_.limits);

    if (!text) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    auto invalidated = tree.invalidate(node, Invalidation::LAYOUT);

    if (!invalidated) {
      return invalidated;
    }

    edit.text_ = std::move(*text);
    edit.cursor_ = edit.text_.bytes().size();
    edit.anchor_ = edit.cursor_;
    edit.dirty_ = false;
    edit.scroll_ = 0;
    edit.revealCursor();
    revision = snapshot.revision;
    initialized = true;

    return {};
  }

  std::expected<void, TreeError> syncModel(LineEdit &edit) {
    auto snapshot = model->snapshot();

    if (!snapshot) {
      return std::unexpected(snapshot.error());
    }

    if (!edit.dirty_ && (!initialized || revision != (*snapshot)->revision)) {
      return adopt(edit, **snapshot);
    }

    return {};
  }

  std::expected<void, TreeError> sync(std::optional<Node<Widget>> focused) {
    auto allowed = alive();

    if (!allowed) {
      return allowed;
    }
    if (busy) {
      return std::unexpected(TreeError::WOULD_BLOCK);
    }

    std::expected<void, TreeError> result;
    auto edited = tree.edit(node, [&](LineEdit &edit) {
      try {
        result = syncModel(edit);

        if (!result || !focused) {
          return;
        }

        auto eligible = tree.inputEligible(node);

        if (!eligible) {
          result = std::unexpected(eligible.error());

          return;
        }

        const bool hasFocus = *eligible && focused->id() == node.id();

        if (edit.focused_ != hasFocus || edit.enabled_ != *eligible) {
          result = tree.invalidate(node, Invalidation::PAINT);

          if (result) {
            edit.focused_ = hasFocus;
            edit.enabled_ = *eligible;
          }
        }
      } catch (const std::bad_alloc &) {
        result = std::unexpected(TreeError::NO_MEMORY);
      }
    });

    return edited ? result : edited;
  }

  LineEditResult replace(LineEdit &edit, std::size_t begin, std::size_t end, std::string_view inserted) {
    const auto old = edit.text_.bytes();
    const auto remaining = old.size() - (end - begin);

    if (remaining > edit.options_.limits.max_input_bytes ||
        inserted.size() > edit.options_.limits.max_input_bytes - remaining) {
      return {LineEditStatus::LIMIT_EXCEEDED};
    }

    if (hasLineSeparator(inserted)) {
      return {LineEditStatus::INVALID_TEXT};
    }

    std::string candidate(old.substr(0, begin));

    candidate.append(inserted);
    candidate.append(old.substr(end));

    auto text = GraphemeText::from(candidate, edit.options_.widthPolicy, edit.options_.limits);

    if (!text) {
      return {textStatus(text.error())};
    }

    // Inserted scalars can merge with either adjacent cluster (combining marks, ZWJ,
    // regional indicators). Re-segment first and choose the next legal byte boundary.
    const auto requested = begin + inserted.size();
    auto cursor = requested;

    for (const auto &cluster : text->graphemes()) {
      if (cluster.byte_begin < requested && requested < cluster.byte_end) {
        cursor = cluster.byte_end;

        break;
      }
    }

    auto invalidated = tree.invalidate(node, Invalidation::LAYOUT);

    if (!invalidated) {
      return failed(invalidated.error());
    }

    edit.text_ = std::move(*text);
    edit.cursor_ = cursor;
    edit.anchor_ = cursor;
    edit.dirty_ = true;
    edit.scroll_ = 0;
    edit.revealCursor();

    return {LineEditStatus::EDITED};
  }

  LineEditResult insert(LineEdit &edit, std::string_view bytes) {
    return replace(edit, std::min(edit.cursor_, edit.anchor_), std::max(edit.cursor_, edit.anchor_), bytes);
  }

  LineEditResult commit(LineEdit &edit) {
    auto snapshot = model->snapshot();

    if (!snapshot) {
      return failed(snapshot.error());
    }

    if (revision != (*snapshot)->revision) {
      return {LineEditStatus::CONFLICT};
    }

    // The predicate may reenter or remove the node. Own the candidate and revalidate
    // both node and model before committing; never borrow widget text across user code.
    std::string candidate(edit.text_.bytes());

    if (validate && !validate(candidate)) {
      return {LineEditStatus::VALIDATION_FAILED};
    }

    auto allowed = alive();

    if (!allowed) {
      return failed(allowed.error());
    }

    snapshot = model->snapshot();

    if (!snapshot) {
      return failed(snapshot.error());
    }

    if (revision != (*snapshot)->revision) {
      return {LineEditStatus::CONFLICT};
    }

    auto updated = model->set(candidate);

    if (!updated) {
      return failed(updated.error());
    }

    // Property pruning may release reentrant user captures and remove the node.
    // Preserve the exact successful receipt even when presentation no longer exists.
    auto applied = tree.edit(node, [&](LineEdit &current) {
      current.dirty_ = false;
      revision = updated->snapshot->revision;
    });
    LineEditResult result{LineEditStatus::COMMITTED, std::move(*updated)};

    if (!applied && applied.error() != TreeError::STALE_NODE && applied.error() != TreeError::CLOSED) {
      result.error = applied.error();
    }

    return result;
  }

  LineEditResult action(LineEdit &edit, ActionEvent event) {
    if (event.action == Action::ACTIVATE) {
      return commit(edit);
    }

    if (event.action == Action::CANCEL) {
      auto snapshot = model->snapshot();
      auto status = snapshot ? adopt(edit, **snapshot) : std::unexpected(snapshot.error());

      return status ? LineEditResult{LineEditStatus::CANCELLED} : failed(status.error());
    }

    auto begin = std::min(edit.cursor_, edit.anchor_);
    auto end = std::max(edit.cursor_, edit.anchor_);

    if (event.action == Action::ERASE_BACKWARD || event.action == Action::ERASE_FORWARD) {
      if (begin == end) {
        if (event.action == Action::ERASE_BACKWARD) {
          begin = edit.text_.previousCursor(begin).value();
        } else {
          end = edit.text_.nextCursor(end).value();
        }
      }

      return replace(edit, begin, end, "");
    }

    auto cursor = edit.cursor_;

    switch (event.action) {
    case Action::MOVE_LEFT:
      cursor = !event.modifiers.shift && begin != end ? begin : edit.text_.previousCursor(cursor).value();

      break;
    case Action::MOVE_RIGHT:
      cursor = !event.modifiers.shift && begin != end ? end : edit.text_.nextCursor(cursor).value();

      break;
    case Action::MOVE_HOME:
      cursor = 0;

      break;
    case Action::MOVE_END:
      cursor = edit.text_.bytes().size();

      break;
    default:
      break;
    }

    auto invalidated = tree.invalidate(node, Invalidation::PAINT);

    if (!invalidated) {
      return failed(invalidated.error());
    }

    edit.cursor_ = cursor;

    if (!event.modifiers.shift) {
      edit.anchor_ = cursor;
    }

    edit.revealCursor();

    return {LineEditStatus::EDITED};
  }

  LineEditResult shortcut(LineEdit &edit, char32_t key) {
    if (key == U'a') {
      auto invalidated = tree.invalidate(node, Invalidation::PAINT);

      if (!invalidated) {
        return failed(invalidated.error());
      }

      edit.anchor_ = 0;
      edit.cursor_ = edit.text_.bytes().size();
      edit.revealCursor();

      return {LineEditStatus::EDITED};
    }

    if (!clipboard) {
      return {LineEditStatus::CLIPBOARD_FAILED};
    }

    if (key == U'v') {
      auto bytes = clipboard->read(edit.options_.limits.max_input_bytes);

      if (!bytes) {
        return {LineEditStatus::CLIPBOARD_FAILED, {}, bytes.error()};
      }

      auto allowed = alive();

      return allowed ? insert(edit, *bytes) : failed(allowed.error());
    }

    const auto begin = std::min(edit.cursor_, edit.anchor_);
    const auto end = std::max(edit.cursor_, edit.anchor_);
    const std::string selected(edit.text_.bytes().substr(begin, end - begin));
    auto written = clipboard->write(selected);

    if (!written) {
      return {LineEditStatus::CLIPBOARD_FAILED, {}, written.error()};
    }

    auto allowed = alive();

    if (!allowed) {
      return failed(allowed.error());
    }

    return key == U'x' ? replace(edit, begin, end, "") : LineEditResult{LineEditStatus::COPIED};
  }

  std::optional<LineEditResult> handle(LineEdit &edit, const RoutedEvent &event) {
    const auto *command = std::get_if<ActionEvent>(&event);
    const auto *text = std::get_if<TextInputEvent>(&event);
    const auto *paste = std::get_if<PasteInputEvent>(&event);
    const auto *key = std::get_if<KeyEvent>(&event);
    const auto shortcutScalar = shortcutCharacter(key);
    const bool shortcutKey = shortcutScalar != U'\0';
    const bool commandKey = acceptsAction(command);

    if (!commandKey && !(text && !text->modifiers.alt && !text->modifiers.control && !text->modifiers.super) &&
        !paste && !shortcutKey) {
      return {};
    }

    if (busy) {
      return failed(TreeError::WOULD_BLOCK);
    }

    struct BusyGuard {
      bool &busy;

      ~BusyGuard() {
        busy = false;
      }
    } guard{busy};

    busy = true;

    try {
      auto synced = syncModel(edit);

      if (!synced) {
        return failed(synced.error());
      }

      if (commandKey) {
        return action(edit, *command);
      }

      if (shortcutKey) {
        return shortcut(edit, shortcutScalar);
      }

      if (paste) {
        return insert(edit, paste->bytes);
      }

      const auto encoded = encode(text->scalar);

      return insert(edit, encoded.view());
    } catch (const std::bad_alloc &) {
      return failed(TreeError::NO_MEMORY);
    } catch (...) {
      return failed(TreeError::CALLBACK_FAILED);
    }
  }
};

LineEditBinding::LineEditBinding(std::shared_ptr<State> state, ActionBinding action, Subscription subscription)
    : state_(std::move(state)), action_(std::move(action)), subscription_(std::move(subscription)) {
}

LineEditBinding::~LineEditBinding() = default;
LineEditBinding::LineEditBinding(LineEditBinding &&) noexcept = default;

LineEditBinding &LineEditBinding::operator=(LineEditBinding &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  LineEditBinding retired(std::move(other));

  using std::swap;

  swap(state_, retired.state_);
  swap(action_, retired.action_);
  swap(subscription_, retired.subscription_);

  return *this;
}

std::expected<void, TreeError> LineEditBinding::sync(Node<Widget> focused) {
  // A retained state also protects against reentrant destruction during synchronization.
  auto state = state_;

  if (!state) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  return state->sync(focused);
}

std::expected<LineEditBinding, TreeError> bindLineEdit(WidgetTree &tree, EventRouter &router, Node<LineEdit> node,
                                                       std::shared_ptr<Property<std::string>> model,
                                                       std::function<void(LineEditResult)> result,
                                                       std::shared_ptr<Clipboard> clipboard,
                                                       std::function<bool(std::string_view)> validate) {
  if (!model || !result) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  try {
    auto state = std::make_shared<LineEditBinding::State>(
      LineEditBinding::State{tree, node, std::move(model), std::move(clipboard), std::move(validate)});
    auto subscription = state->model->observe(node, [state](LineEdit &, auto) {
      // Read latest rather than replaying a potentially obsolete delivered snapshot.
      auto synced = state->sync({});

      if (!synced) {
        throw synced.error();
      }
    });

    if (!subscription) {
      return std::unexpected(subscription.error());
    }

    auto action = router.bind(node, [state, result = std::move(result)](LineEdit &edit, const RoutedEvent &event) {
      auto outcome = state->handle(edit, event);

      if (!outcome) {
        return EventReply::IGNORED;
      }

      // Last access to the widget happened above: this callback may remove its ancestor.
      result(std::move(*outcome));

      return EventReply::HANDLED;
    });

    if (!action) {
      return std::unexpected(action.error());
    }

    auto synced = state->sync({});

    if (!synced) {
      return std::unexpected(synced.error());
    }

    return LineEditBinding(std::move(state), std::move(*action), std::move(*subscription));
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  } catch (...) {
    return std::unexpected(TreeError::CALLBACK_FAILED);
  }
}
} // namespace portals
