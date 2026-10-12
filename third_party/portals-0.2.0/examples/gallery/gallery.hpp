// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <portals/controls.hpp>

#include <optional>
#include <stdexcept>

/** @brief Finite control-gallery fixture shared by the example and semantic replay tests. */
namespace gallery {
using namespace portals;

inline constexpr CellSize FIXTURE_SIZE{30, 9}; ///< Default semantic replay viewport in terminal cells.

/** @brief Unwrap an owning expected result or stop this finite example with an exception.
 * @tparam T Success type accepted by std::expected: void, or a movable value returned by ownership transfer.
 * @tparam E Error type accepted by std::expected; the helper does not inspect or format its value.
 * @param result Owning expected passed by value; argument copying/moving occurs before helper entry.
 * @return The success value moved out of result, or no value when T is void.
 * @throws std::runtime_error If result is unexpected; its original error detail is not included.
 * @throws std::bad_alloc If allocating the diagnostic or moving T requires unavailable storage.
 * Other exceptions from T's move construction or argument construction also propagate.
 */
template <class T, class E> T take(std::expected<T, E> result) {
  if (!result) {
    throw std::runtime_error("gallery operation failed");
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

/** @brief Create owning gallery text with the default width policy.
 * @param value Borrowed UTF-8 input; the returned text owns its decoded content.
 * @return Validated text using the default WidthPolicy.
 * @throws std::runtime_error If text validation fails.
 * @throws std::bad_alloc If text or diagnostic storage cannot be allocated.
 */
inline GraphemeText text(std::string_view value) {
  return take(GraphemeText::from(value));
}

/** @brief Translate widget-local paint into the layout surface and ancestor clip.
 * Commands borrow widget text; rendering must consume them inside the same tree frame.
 */
class SurfaceSink : public PaintContext {
public:
  /** @param geometry Owning transform/clip captured by the current layout pass.
   * @param commands Borrowed output vector, kept alive through submit and frame rendering.
   */
  SurfaceSink(NodeGeometry geometry, std::vector<RenderCommand> &commands) : geometry_(geometry), commands_(commands) {
  }

  /** @param commands Borrowed local commands; copied metadata still borrows widget text.
   * @return Success or INVALID_GEOMETRY for overflowing transforms; allocation exceptions propagate.
   * Submission appends in order; on failure the caller must discard the incomplete frame.
   */
  std::expected<void, TreeError> submit(std::span<const RenderCommand> commands) override {
    for (const auto &command : commands) {
      auto status = std::visit(
        [&](const auto &value) -> std::expected<void, TreeError> {
          // Apply the node transform, then intersect with the layout-owned ancestor clip.
          auto clip = value.clip.translated(geometry_.surface.origin());

          if (!clip) {
            return std::unexpected(TreeError::INVALID_GEOMETRY);
          }

          if constexpr (std::same_as<std::decay_t<decltype(value)>, TextCommand>) {
            auto origin = translated(value.origin, geometry_.surface.origin());

            if (!origin) {
              return std::unexpected(TreeError::INVALID_GEOMETRY);
            }

            commands_.emplace_back(
              TextCommand{*origin, value.text, clip->intersection(geometry_.clip), value.style, value.z});
          } else {
            auto area = value.area.translated(geometry_.surface.origin());

            if (!area) {
              return std::unexpected(TreeError::INVALID_GEOMETRY);
            }

            commands_.emplace_back(FillCommand{*area, clip->intersection(geometry_.clip), value.style, value.z});
          }

          return {};
        },
        command);

      if (!status) {
        return status;
      }
    }

    return {};
  }

private:
  NodeGeometry geometry_;                ///< Owning surface transform and ancestor clip for the current frame.
  std::vector<RenderCommand> &commands_; ///< Borrowed frame output, consumed before widgets can change.
};

/** @brief Finite low-level fixture for semantic screenshots and control regression tests.
 * The interactive program uses Form instead. This fixture intentionally retains explicit
 * tree/router ownership, binding tokens and presentation sync to exercise the low-level API.
 * All operations and destruction are UI-affine; field order keeps dependencies alive.
 */
struct Scene {
  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create());        ///< Dispatcher outliving tree and callbacks.
  std::unique_ptr<WidgetTree> tree = take(WidgetTree::create(*scheduler)); ///< Owning retained control tree.
  Node<Widget> root = take(tree->root()); ///< Observing tree root and property invalidation target.
  std::unique_ptr<EventRouter> router = take(EventRouter::create(*tree)); ///< Input/focus owner borrowing tree.

  Node<Column> column = take(tree->emplace<Column>(root));                          ///< Vertical fixture layout.
  Node<Label> title = take(tree->emplace<Label>(column, text("Portals controls"))); ///< Static title.
  // UTF-8 escapes test a fullwidth ideograph and an e + combining acute grapheme.
  Node<Label> label = take(tree->emplace<Label>(column, text("Unicode: \xe7\x95\x8c e\xcc\x81"))); ///< Width fixture.
  Node<Button> button = take(tree->emplace<Button>(column, text("Run")));          ///< Enabled business action target.
  Node<CheckBox> checkbox = take(tree->emplace<CheckBox>(column, text("Alerts"))); ///< Shared boolean editor.

  Node<Column> disabledGroup = take(tree->emplace<Column>(column)); ///< Disabled ancestor for input exclusion.
  Node<Label> disabledLabel = take(tree->emplace<Label>(disabledGroup, text("Disabled label"))); ///< Disabled text.
  Node<Button> disabledButton =
    take(tree->emplace<Button>(disabledGroup, text("Disabled button"))); ///< Disabled action.
  Node<CheckBox> disabledCheckbox =
    take(tree->emplace<CheckBox>(disabledGroup, text("Disabled check")));              ///< Disabled editor.
  Node<Label> status = take(tree->emplace<Label>(column, text("Initial focus: Run"))); ///< Stage diagnostic caption.

  std::vector<Node<Control>> controls{title,         label,          button,           checkbox,
                                      disabledLabel, disabledButton, disabledCheckbox, status}; ///< Paint/sync order.

  /// Shared checkbox state; both registrations retain shared ownership of this UI-affine property.
  std::shared_ptr<Property<bool>> model = take(Property<bool>::create(*tree, root, false, Invalidation::PAINT));
  int calls{};         ///< Enabled button business callback count.
  int changes{};       ///< Checkbox business callback count, excluding presentation sync.
  int disabledCalls{}; ///< Disabled subtree callback count, expected to stay zero.

  ActionBinding buttonBinding = take(bindButton(*router, button, [&] {
    ++calls;
  })); ///< Owns the enabled button action registration.
  ActionBinding disabledBinding = take(bindButton(*router, disabledButton, [&] {
    ++disabledCalls;
  })); ///< Owns a registration suppressed by the disabled ancestor.
  CheckBoxBinding checkBinding = take(bindCheckbox(*tree, *router, checkbox, model, [&](CheckBoxResult result) {
    take(std::move(result));

    ++changes;
  })); ///< Synchronizes the enabled checkbox and retains its scoped registration.
  CheckBoxBinding disabledCheckBinding =
    take(bindCheckbox(*tree, *router, disabledCheckbox, model, [&](CheckBoxResult result) {
      take(std::move(result));

      ++disabledCalls;
    })); ///< Retains the disabled checkbox registration for exclusion tests.

  InputDecoder decoder = take(InputDecoder::create()); ///< Stateful VT byte decoder used only by this fixture.

  /** @brief Disable the nested group after nodes and registrations exist.
   * Setup errors throw through the example-only take helper.
   */
  Scene() {
    take(tree->setEnabled(disabledGroup, false));
  }

  /** @param bytes Borrowed VT input, consumed synchronously through the retained decoder.
   * @return The final dispatch result; default when no complete event was decoded.
   * Decoder/router failures throw through take; no input bytes are retained by this view.
   */
  DispatchResult input(std::string_view bytes) {
    DispatchResult result;

    do {
      auto decoded = take(decoder.next(bytes, 0));

      bytes.remove_prefix(decoded.consumed);

      if (decoded.event) {
        result = take(router->dispatch(*decoded.event));
      }

      if (bytes.empty() && !decoded.event) {
        break;
      }
    } while (true);

    return result;
  }

  /** @param size Positive output size in terminal cells; defaults to FIXTURE_SIZE.
   * @return Owning rendered cell buffer, independent of the command borrows used inside the frame.
   * Synchronizes binding/focus presentation before entering the protected frame.
   * Layout, paint and rendering consume borrowed commands inside that frame.
   * API failures throw through take; callbacks and temporary commands do not escape the frame.
   */
  CellBuffer render(CellSize size = FIXTURE_SIZE) {
    // Apply model and focus presentation before entering the immutable render frame.
    take(checkBinding.sync());
    take(disabledCheckBinding.sync());

    const auto focused = take(router->focused());

    for (const auto control : controls) {
      take(syncControl(*tree, control, focused));
    }

    const auto viewport = take(CellRect::from({}, size));
    std::optional<CellBuffer> buffer;

    take(tree->frame([&] {
      take(layoutTree(*tree, root, viewport, viewport));

      std::vector<RenderCommand> commands;

      for (const auto control : controls) {
        const auto snapshot = take(tree->inspect(control));

        if (!snapshot.geometry.valid) {
          continue;
        }

        SurfaceSink sink(snapshot.geometry, commands);

        take(tree->paint(control, sink));
      }

      // Finish consuming every borrowed command while the frame still retains the widgets.
      buffer.emplace(take(renderFullFrame(size, commands)));
    }));

    return std::move(*buffer);
  }

  /** @param name Cumulative stage: initial, keyboard, mouse or disabled.
   * @return No value; mutates the fixture to the requested semantic screenshot state.
   * Unknown direct names follow the disabled branch; CLI validates allowed stage spellings.
   * The intermediate render establishes geometry for the subsequent mouse hit test.
   */
  void stage(std::string_view name) {
    if (name == "initial") {
      return;
    }

    input("\r\t ");
    take(setControlText(*tree, status, text("Keyboard: Run + Alerts")));

    if (name == "keyboard") {
      return;
    }

    auto keyboardFrame = render();

    input("\x1b[<0;2;3M\x1b[<0;2;3m");
    take(setControlText(*tree, status, text("Mouse: Run pressed once")));

    if (name == "mouse") {
      return;
    }

    take(tree->setEnabled(button, false));
    take(setControlText(*tree, status, text("Run disabled; focus: Alerts")));
  }
};
} // namespace gallery
