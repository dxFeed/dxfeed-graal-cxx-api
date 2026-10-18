// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/action.hpp>
#include <portals/layout.hpp>
#include <portals/property.hpp>

namespace portals {
/** @brief Owning paint values; no theme engine or behavior is encoded here. */
struct ControlStyle {
  CellStyle normal{};                                                  ///< Ordinary appearance.
  CellStyle focused{{}, {}, true, false, true};                        ///< Keyboard or mouse focus appearance.
  CellStyle disabled{RgbColor{128, 128, 128}, {}, false, true, false}; ///< Ineligible subtree appearance.
  /** @brief Compare every appearance with the unnamed other style argument.
   * @return True when normal, focused and disabled styles all compare equal.
   */
  bool operator==(const ControlStyle &) const = default;
};

/** @brief UI-owned one-line control with owning Unicode text and local paint coordinates.
 * Mutate through the free setters between frames. The frame owner translates local commands
 * and intersects their clips with NodeGeometry, then renders before leaving tree.frame.
 * Focus/enabled flags are presentation snapshots only; EventRouter owns input eligibility.
 */
class Control : public SizedWidget {
public:
  /** @brief Report intrinsic text and decoration width, with height one.
   * @param available Layout's offered extent; intentionally does not limit intrinsic measurement.
   * @return Owning cell size, or INVALID_GEOMETRY if text plus decoration width is unrepresentable.
   * @note Called on UI through WidgetTree::measure; no model reads, I/O or invalidation.
   */
  std::expected<CellSize, TreeError> measure(CellSize available) override;
  /** @brief Retain a zero-origin rectangle with the allocated local extent.
   * @param local Parent-relative allocation; its origin is handled by the frame owner's surface adapter.
   * @return Success after storing the extent, or INVALID_GEOMETRY if the zero-origin rectangle is invalid.
   * @note Called on UI through WidgetTree::arrange; no domain mutation or I/O.
   */
  std::expected<void, TreeError> arrange(CellRect local) override;
  /** @brief Submit local fill/text commands from the synchronized presentation snapshot.
   * @param context Borrowed synchronous sink; command storage is valid during submit only. Text belongs
   * to this control and may be borrowed by a retained command until the enclosing tree.frame returns.
   * @return Success, intrinsic-measurement INVALID_GEOMETRY, or the sink's unchanged TreeError.
   * @note The frame owner translates local geometry, intersects the ancestor clip and renders before
   * leaving tree.frame. No context/text reference escapes that frame; paint performs no model reads.
   * @throws std::bad_alloc If the sink cannot allocate. Other sink exceptions also propagate;
   * WidgetTree::paint translates escaped exceptions to CALLBACK_FAILED.
   */
  std::expected<void, TreeError> paint(PaintContext &context) override;
  /** @brief Inspect the control's owning Unicode text on UI.
   * @return A const reference borrowed only within the enclosing tree.read callback; never retain it.
   */
  const GraphemeText &text() const;
  /** @brief Inspect the last synchronized focus presentation on UI.
   * @return True when the last sync marked this eligible control focused; not routing authority.
   */
  bool focused() const;
  /** @brief Inspect the last synchronized subtree eligibility presentation on UI.
   * @return True when the control and its ancestors were eligible at the last sync.
   */
  bool enabled() const;

  /** @return Owning manual normal/focused/disabled appearances beneath theme declarations. */
  ComputedStyle styleDefaults() const override;

protected:
  /** @brief Own text/options and create fixed ASCII decorations under the text's validated width policy.
   * @param text Validated owning text, transferred into the control with its complete WidthPolicy.
   * @param prefix Borrowed leading ASCII decoration, copied during construction; "[ ] " enables
   * an alternate same-width "[x] " decoration for checkbox presentation.
   * @param suffix Borrowed trailing ASCII decoration, copied during construction.
   * @param style Owning normal/focused/disabled paint values.
   * @param policy Owning layout size policy, subsequently validated by the layout owner.
   * @throws std::bad_alloc If text transfer or decoration storage cannot allocate.
   * @throws std::bad_expected_access<TextError> If a supplied decoration cannot be validated within text limits.
   * @note Construct on UI through tree.emplace, which translates allocation failure to NO_MEMORY
   * and other constructor exceptions to CALLBACK_FAILED. Argument construction precedes that boundary.
   */
  Control(GraphemeText text, std::string_view prefix, std::string_view suffix, ControlStyle style, SizePolicy policy);
  /** @brief Select a same-width prefix between frames; used only by the checkbox binding.
   * @param checked True selects the checked decoration; false selects the original prefix.
   * @note The caller owns paint invalidation; this helper only changes presentation.
   */
  void showChecked(bool checked);
  /** @brief Inspect the single checked presentation flag on UI.
   * @return The last value supplied to showChecked, initially false.
   */
  bool shownChecked() const;

private:
  friend std::expected<void, TreeError> setControlText(WidgetTree &, Node<Control>, GraphemeText);
  friend std::expected<void, TreeError> setControlStyle(WidgetTree &, Node<Control>, ControlStyle);
  friend std::expected<void, TreeError> syncControl(WidgetTree &, Node<Control>, Node<Widget>);
  GraphemeText text_;
  GraphemeText prefix_;
  GraphemeText checked_prefix_;
  GraphemeText suffix_;
  ControlStyle style_;
  CellRect bounds_;
  bool checked_{};
  bool focused_{};
  bool enabled_{true};
};

/** @brief Noninteractive Unicode label; text remains data, including control characters. */
class Label : public Control {
public:
  /** @brief Construct an owning undecorated label on UI, normally through tree.emplace.
   * @param text Validated owning text and WidthPolicy, transferred into the label.
   * @param style Owning paint values; disabled ancestors are reflected by syncControl.
   * @param policy Owning size policy, checked later by layout.
   * @throws std::bad_alloc If text transfer or decoration storage cannot allocate; tree.emplace
   * reports internal allocation failure as NO_MEMORY, excluding construction of its arguments.
   */
  explicit Label(GraphemeText text, ControlStyle style = {}, SizePolicy policy = {});
};

/** @brief One-line button; registration is an explicit caller-owned ActionBinding. */
class Button : public Control {
public:
  /** @brief Construct an owning button with ASCII brackets on UI, normally through tree.emplace.
   * @param text Validated owning caption and WidthPolicy, transferred into the button.
   * @param style Owning normal/focused/disabled paint values.
   * @param policy Owning size policy, checked later by layout.
   * @throws std::bad_alloc If text transfer or decoration storage cannot allocate; tree.emplace
   * reports internal allocation failure as NO_MEMORY, excluding construction of its arguments.
   */
  explicit Button(GraphemeText text, ControlStyle style = {}, SizePolicy policy = {});
};

/** @brief Checkbox presentation; Property<bool> is its authoritative value when bound. */
class CheckBox : public Control {
public:
  /** @brief Construct an owning initially unchecked view on UI, normally through tree.emplace.
   * @param text Validated owning caption and WidthPolicy, transferred into the checkbox.
   * @param style Owning normal/focused/disabled paint values.
   * @param policy Owning size policy, checked later by layout.
   * @throws std::bad_alloc If text transfer or decoration storage cannot allocate; tree.emplace
   * reports internal allocation failure as NO_MEMORY, excluding construction of its arguments.
   */
  explicit CheckBox(GraphemeText text, ControlStyle style = {}, SizePolicy policy = {});
  /** @brief Inspect the last synchronized model value on UI inside tree.read.
   * @return The checked presentation value, initially false; the binding's Property remains authoritative.
   */
  bool checked() const;

private:
  friend class CheckBoxBinding;
  void display(bool value);
};

/** @brief Replace owning text between frames; equal bytes/policy are a no-op, changes invalidate layout.
 * @param tree Borrowed UI owner containing node; used only during this call.
 * @param node Observing handle to the control whose text is replaced.
 * @param text Validated owning replacement, transferred only after decorations are prepared and invalidation succeeds.
 * @return Success, internal NO_MEMORY, or the tree's WRONG_THREAD, WRONG_SESSION, STALE_NODE, closed,
 * WOULD_BLOCK (active frame), CALLBACK_FAILED or invalidation error. Failure retains the previous text.
 * @throws std::bad_alloc If construction of the by-value argument fails before this function begins;
 * allocation inside the mutation boundary is returned as NO_MEMORY instead.
 */
std::expected<void, TreeError> setControlText(WidgetTree &tree, Node<Control> node, GraphemeText text);
/** @brief Replace owning paint values on UI between frames; equal is a no-op, changes invalidate paint.
 * @param tree Borrowed UI owner containing node; used only during this call.
 * @param node Observing handle to the control whose appearances are replaced.
 * @param style Owning replacement paint values; does not affect behavior or layout metrics.
 * @return Success or the tree's WRONG_THREAD, WRONG_SESSION, STALE_NODE, closed, WOULD_BLOCK (active frame),
 * CALLBACK_FAILED or invalidation error. Rejection leaves the stored style unchanged.
 */
std::expected<void, TreeError> setControlStyle(WidgetTree &tree, Node<Control> node, ControlStyle style);
/** @brief Copy focus and ancestor eligibility on UI between frames; changes invalidate paint.
 * @param tree Borrowed UI owner containing node; also supplies ancestor eligibility.
 * @param node Observing control handle receiving the presentation snapshot.
 * @param focused Observing snapshot obtained once from EventRouter::focused before synchronizing the controls;
 * an empty handle means no focused control. This identity is compared, not dereferenced or made authoritative.
 * @return Success or the tree's WRONG_THREAD, WRONG_SESSION, STALE_NODE, closed, WOULD_BLOCK (active frame),
 * CALLBACK_FAILED or invalidation error. Equal presentation flags do not invalidate.
 * @note No focus recovery, registration or routing is performed; never call router.focused from paint.
 */
std::expected<void, TreeError> syncControl(WidgetTree &tree, Node<Control> node, Node<Widget> focused);
/** @brief Bind one activation callback on UI outside a frame; the caller owns the returned disconnect token.
 * @param router Borrowed router for node's tree; its tree and Scheduler must outlive the router.
 * The token may outlive the router, following ActionBinding's lifetime contract.
 * @param node Observing button handle in router's tree; input eligibility remains the router's responsibility.
 * @param action Required owning void callback, transferred into the registration. Captures must remain valid
 * for an admitted call, including after token destruction; router reclamation releases them on UI.
 * @return An owning ActionBinding, or INVALID_ARGUMENT for an empty callback/duplicate registration;
 * WRONG_THREAD, WRONG_SESSION, STALE_NODE, closed, WOULD_BLOCK (frame/nested dispatch), NO_MEMORY,
 * NODE_LIMIT or CALLBACK_FAILED from the router registration boundary.
 * @throws std::bad_alloc If construction of the by-value callback argument cannot allocate before API entry.
 * Other exceptions from user capture construction before API entry also propagate.
 *
 * Unmodified activate (including Enter), text Space and left mouse press activate once.
 * Releases, motion, paste and other input are ignored. Callback can remove the node/ancestor
 * or destroy its token; existing router/tree borrows retain the admitted widget and captures.
 * Invocation exceptions become CALLBACK_FAILED through the router and do not undo completed user effects.
 */
std::expected<ActionBinding, TreeError> bindButton(EventRouter &router, Node<Button> node,
                                                   std::function<void()> action);
/** @brief Exact Property set outcome; errors mean no commit, successful receipts can reject notifications. */
using CheckBoxResult = std::expected<PropertyUpdate<bool>, TreeError>;

/** @brief Own one checkbox's action, subscription and shared UI model owner.
 * Tree, router and scheduler must outlive this token; destroy/move on UI between frames,
 * including from its callback. Model changes never invoke the business callback.
 * Call sync before each frame after external changes/draining events: notifications can
 * be rejected, and equal Property::set does not retry them. Latest committed value wins.
 */
class CheckBoxBinding {
public:
  /** @brief Disconnect subscription/action and release model ownership on UI.
   * Admitted callbacks retain their captures; router collection may defer the final model release.
   * Wrong-thread subscription destruction terminates rather than throwing.
   */
  ~CheckBoxBinding();
  /** @brief Transfer all ownership from the unnamed source binding argument on UI.
   * The source becomes empty; its sync returns INVALID_ARGUMENT and destruction is harmless.
   * No callbacks run during this nonthrowing transfer.
   */
  CheckBoxBinding(CheckBoxBinding &&) noexcept;
  /** @brief Replace ownership from the unnamed source binding argument on UI; self-move is a no-op.
   * The source becomes empty. All replacement fields are installed before retiring old resources;
   * deferred capture reclamation can reenter UI code. Neither binding owns its tree/router.
   * @return This binding by reference; callers must not use the result if reentrant cleanup destroyed its owner.
   */
  CheckBoxBinding &operator=(CheckBoxBinding &&) noexcept;
  CheckBoxBinding(const CheckBoxBinding &) = delete;
  CheckBoxBinding &operator=(const CheckBoxBinding &) = delete;
  /** @brief Read the latest committed model snapshot on UI between frames and synchronize presentation.
   * @return Success with paint invalidation only on a value change; INVALID_ARGUMENT for a moved-from token,
   * WRONG_THREAD from the model/tree, or the tree's WRONG_SESSION, STALE_NODE, closed, WOULD_BLOCK (active frame),
   * CALLBACK_FAILED or invalidation error. Rejected notifications do not prevent resynchronization.
   * @note No business callback runs; old queued notifications cannot overwrite a newer committed value.
   */
  std::expected<void, TreeError> sync();

private:
  struct State;
  friend std::expected<CheckBoxBinding, TreeError> bindCheckbox(WidgetTree &, EventRouter &, Node<CheckBox>,
                                                                std::shared_ptr<Property<bool>>,
                                                                std::function<void(CheckBoxResult)>);
  CheckBoxBinding(std::shared_ptr<State> state, ActionBinding action, Subscription subscription);
  std::shared_ptr<State> state_;
  ActionBinding action_;
  Subscription subscription_;
};

/** @brief Bind and initially synchronize a checkbox on UI outside a frame; the caller owns the returned token.
 * @param tree Borrowed UI tree containing node and the model's invalidation target; outlives the binding/model.
 * @param router Borrowed router for the same tree; tree, router and Scheduler outlive the binding.
 * @param node Observing checkbox handle receiving presentation updates and routed activations.
 * @param model Required shared owner of Property<bool>; retained by the binding's action and subscription.
 * @param changed Required owning callback accepting CheckBoxResult by value. It receives the exact result
 * of this gesture's set: unexpected means no commit, while a successful receipt can contain rejected or
 * cancelled notifications. Receipt snapshots own their value and may be retained; callbacks may remove
 * node/ancestors or destroy the binding. Observer deliveries never call changed.
 * @return An owning CheckBoxBinding, or INVALID_ARGUMENT for a null model/callback or duplicate action;
 * WRONG_THREAD, WRONG_SESSION, STALE_NODE, closed, WOULD_BLOCK (frame/nested dispatch), NO_MEMORY,
 * NODE_LIMIT, CALLBACK_FAILED or another delegated Property/WidgetTree/EventRouter registration error.
 * @throws std::bad_alloc If construction of the by-value callback argument cannot allocate before API entry.
 * Other exceptions from user capture construction before API entry also propagate.
 *
 * Gestures match Button. Toggle reads the latest model, commits once, updates presentation
 * immediately even on rejected notifications, then transfers the exact receipt to changed.
 * Invocation exceptions follow EventRouter's CALLBACK_FAILED contract; a committed model is never rolled back.
 * Observer deliveries read the latest snapshot, so an old queued event cannot undo a toggle.
 */
std::expected<CheckBoxBinding, TreeError> bindCheckbox(WidgetTree &tree, EventRouter &router, Node<CheckBox> node,
                                                       std::shared_ptr<Property<bool>> model,
                                                       std::function<void(CheckBoxResult)> changed);
} // namespace portals
