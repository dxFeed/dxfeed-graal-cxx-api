// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/controls.hpp>

namespace portals {
/** @brief Observable editing or service outcome; rejection preserves the draft. */
enum class LineEditStatus {
  EDITED,            ///< A local draft or selection changed.
  COMMITTED,         ///< Enter completed Property::set; inspect its exact receipt.
  CANCELLED,         ///< Escape adopted the latest model snapshot.
  COPIED,            ///< Selected bytes were accepted by the clipboard service.
  CONFLICT,          ///< The model revision changed since this draft began.
  INVALID_TEXT,      ///< Invalid UTF-8 or a line separator was rejected in full.
  LIMIT_EXCEEDED,    ///< Input, cluster, grapheme or cell limit rejected the whole edit.
  VALIDATION_FAILED, ///< The commit validator rejected the complete draft.
  CLIPBOARD_FAILED,  ///< The injected service was absent or rejected the operation.
  FAILED             ///< A tree, allocation or callback failure; inspect error.
};

/** @brief Exact result delivered after an input operation on the UI thread. */
struct LineEditResult {
  LineEditStatus status{LineEditStatus::EDITED};       ///< Meaning of this operation.
  std::optional<PropertyUpdate<std::string>> commit{}; ///< Exact successful set receipt, including notification loss.
  std::optional<TreeError> error{}; ///< Underlying tree, allocation, callback or clipboard service failure.
};

/** @brief Injected UI clipboard boundary; called only from input handling, never from a frame.
 * The binding shares ownership. Implementations must return promptly and may reenter UI code.
 * Bytes remain data; no terminal escapes are interpreted. Exceptions become FAILED results.
 */
class Clipboard {
public:
  /** @brief Release the service on the UI owner after admitted calls finish. */
  virtual ~Clipboard() = default;
  /** @brief Obtain an owning bounded payload.
   * @param maxBytes Maximum accepted payload bytes; the caller also checks this bound.
   * @return Owning bytes, or a service-specific TreeError; no partial success is inferred.
   */
  virtual std::expected<std::string, TreeError> read(std::size_t maxBytes) = 0;
  /** @brief Copy selected bytes into the service.
   * @param bytes UTF-8 selection borrowed only until this call returns.
   * @return Success after accepting the whole selection, or a service-specific TreeError.
   */
  virtual std::expected<void, TreeError> write(std::string_view bytes) = 0;
};

/** @brief Owning bounds and appearances for a one-line editor. */
struct LineEditOptions {
  TextLimits limits{4096, 4096, 4096, 8192};                       ///< Whole draft limits, never silent truncation.
  WidthPolicy widthPolicy{};                                       ///< Immutable cell-width policy for this editor.
  ControlStyle style{};                                            ///< Normal, focused and disabled appearance.
  CellStyle selection{RgbColor{0, 0, 0}, RgbColor{180, 200, 255}}; ///< Whole selected grapheme appearance.
  CellStyle cursor{{}, {}, true, false, true}; ///< Software caret cell; whole wide glyph when it fits.
};

/** @brief UI-owned Unicode draft; mutate only through its scoped binding between frames.
 * Cursor/anchor are UTF-8 byte boundaries, while scrolling and painting use terminal cells.
 * Paint borrows owning text through the enclosing tree.frame and performs no service/model calls.
 */
class LineEdit : public SizedWidget {
public:
  /** @brief Construct an empty editor; binding adopts its initial model snapshot.
   * @param options Owning immutable limits, width policy and styles.
   * @param policy Owning layout size policy validated by layout.
   * @throws std::bad_alloc If initial text storage cannot be allocated.
   * @throws std::bad_expected_access<TextError> If the width policy is invalid.
   */
  explicit LineEdit(LineEditOptions options = {}, SizePolicy policy = {});
  /** @brief Measure the full draft plus one caret cell.
   * @param available Offered extent; does not constrain intrinsic measurement.
   * @return Intrinsic cell width and height one, or INVALID_GEOMETRY on overflow.
   */
  std::expected<CellSize, TreeError> measure(CellSize available) override;
  /** @brief Store local bounds and scroll by whole graphemes to expose the caret.
   * @param local Parent-relative allocated rectangle; only its extent is retained.
   * @return Success or INVALID_GEOMETRY for unrepresentable bounds.
   */
  std::expected<void, TreeError> arrange(CellRect local) override;
  /** @brief Submit bounded local commands; software caret and selection never split a grapheme.
   * @param context Synchronous sink; retained text borrows expire at enclosing tree.frame exit.
   * @return Success or the sink's error; sink exceptions propagate to WidgetTree::paint.
   */
  std::expected<void, TreeError> paint(PaintContext &context) override;
  /** @brief Borrow validated draft only inside tree.read.
   * @return Reference invalidated by the next edit, synchronization or destruction.
   */
  const GraphemeText &text() const;
  /** @brief Inspect the active selection endpoint.
   * @return UTF-8 byte boundary in text(), including its end.
   */
  std::size_t cursor() const;
  /** @brief Inspect the fixed selection endpoint.
   * @return UTF-8 byte boundary; equals cursor() for an empty selection.
   */
  std::size_t anchor() const;
  /** @brief Inspect whether a local edit awaits Enter or Escape.
   * @return True after local replacement until successful commit or cancellation.
   */
  bool dirty() const;

  /** @return Owning manual control, selection and caret appearances beneath theme declarations. */
  ComputedStyle styleDefaults() const override;

private:
  friend class LineEditBinding;
  void revealCursor();
  std::int32_t cellAt(std::size_t byte) const;
  LineEditOptions options_;
  GraphemeText text_;
  CellRect bounds_;
  std::size_t cursor_{};
  std::size_t anchor_{};
  std::int32_t scroll_{};
  bool dirty_{};
  bool focused_{};
  bool enabled_{true};
};

/** @brief Scoped model/action ownership; destroy and move on UI outside frames.
 * Tree, router and scheduler outlive the binding. Call sync before each frame. Drafts survive
 * external revisions; Enter detects conflict, Escape adopts latest. Focus loss never commits.
 * Admitted callbacks retain state; removal suppresses future callbacks and observer delivery.
 */
class LineEditBinding {
public:
  /** @brief Disconnect subscription/action; admitted calls retain their owners. */
  ~LineEditBinding();
  /** @brief Transfer ownership on UI.
   * @param other Binding emptied by this transfer.
   */
  LineEditBinding(LineEditBinding &&other) noexcept;
  /** @brief Install replacement before retiring old captures, which may reenter UI.
   * @param other Binding emptied by transfer; self-move preserves all state.
   * @return This binding; reentrant cleanup may have destroyed its containing owner.
   */
  LineEditBinding &operator=(LineEditBinding &&other) noexcept;
  LineEditBinding(const LineEditBinding &) = delete;
  LineEditBinding &operator=(const LineEditBinding &) = delete;
  /** @brief Synchronize latest clean model and focus presentation between frames.
   * @param focused Snapshot from EventRouter::focused; empty means no focus.
   * @return Success, INVALID_ARGUMENT for an empty binding, WOULD_BLOCK on frame/reentry,
   * NO_MEMORY, INVALID_ARGUMENT for invalid model text, or delegated tree/model error.
   * Rejected/old notifications never override a dirty draft. Invalid models preserve presentation.
   */
  std::expected<void, TreeError> sync(Node<Widget> focused);

private:
  struct State;
  friend std::expected<LineEditBinding, TreeError>
  bindLineEdit(WidgetTree &, EventRouter &, Node<LineEdit>, std::shared_ptr<Property<std::string>>,
               std::function<void(LineEditResult)>, std::shared_ptr<Clipboard>, std::function<bool(std::string_view)>);
  LineEditBinding(std::shared_ptr<State> state, ActionBinding action, Subscription subscription);
  std::shared_ptr<State> state_;
  ActionBinding action_;
  Subscription subscription_;
};

/** @brief Bind a one-line editor and adopt its initial model on UI outside frames.
 * @param tree Borrowed UI owner, outliving binding and model.
 * @param router Borrowed router for this tree, outliving binding.
 * @param node Observing editor handle; one action registration is allowed per node.
 * @param model Required shared model owner. External values must satisfy editor text limits.
 * @param result Required owning callback, invoked last for consumed editing operations; may remove
 * node/ancestors or destroy the binding. Successful commits stay committed if this callback throws.
 * @param clipboard Optional shared UI service for Ctrl+C/X/V; absence reports CLIPBOARD_FAILED.
 * @param validate Optional commit predicate borrowing the entire candidate only during its call.
 * False preserves draft with VALIDATION_FAILED; exceptions report FAILED/CALLBACK_FAILED.
 * @return Owning binding or delegated tree/router/property error; invalid initial text is INVALID_ARGUMENT.
 * Internal allocations report NO_MEMORY; by-value argument construction may throw before entry.
 * Enter commits only if the starting revision still matches, including after validator reentry.
 * Escape adopts latest. Shift+arrows/Home/End select; Ctrl+A selects all. Cut changes draft only after
 * a successful service write. Paste rejects invalid UTF-8, CR/LF/U+2028/U+2029 and whole over-limit edits.
 * Other controls remain logical data displayed as safe renderer placeholders.
 */
std::expected<LineEditBinding, TreeError> bindLineEdit(WidgetTree &tree, EventRouter &router, Node<LineEdit> node,
                                                       std::shared_ptr<Property<std::string>> model,
                                                       std::function<void(LineEditResult)> result,
                                                       std::shared_ptr<Clipboard> clipboard = {},
                                                       std::function<bool(std::string_view)> validate = {});
} // namespace portals
