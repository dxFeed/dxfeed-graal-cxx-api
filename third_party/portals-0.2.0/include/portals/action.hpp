// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/input.hpp>
#include <portals/widget.hpp>

namespace portals {
/** @brief Frontend-independent commands; text and paste remain distinct data events. */
enum class Action {
  ACTIVATE,       ///< Activate the target's primary command.
  CANCEL,         ///< Cancel the current interaction.
  MOVE_LEFT,      ///< Move left according to the receiving widget.
  MOVE_RIGHT,     ///< Move right according to the receiving widget.
  MOVE_UP,        ///< Move up according to the receiving widget.
  MOVE_DOWN,      ///< Move down according to the receiving widget.
  MOVE_HOME,      ///< Move to the beginning according to the receiving widget.
  MOVE_END,       ///< Move to the end according to the receiving widget.
  ERASE_BACKWARD, ///< Erase before the current position.
  ERASE_FORWARD   ///< Erase after the current position.
};

/** @brief Owning semantic command, usable without terminal input. */
struct ActionEvent {
  Action action{};            ///< Command identity.
  InputModifiers modifiers{}; ///< Selection and other modifier intent.
  /** @brief Value equality.
   * @param other Command and modifier value to compare.
   * @return True when all compared fields are equal.
   */
  bool operator==(const ActionEvent &other) const = default;
};

/** @brief Synchronous input value; callback borrows it only until return. */
using RoutedEvent = std::variant<ActionEvent, KeyEvent, TextInputEvent, PasteInputEvent, MouseInputEvent>;
/** @brief Explicit bubbling decision; ignored continues toward the original root. */
enum class EventReply {
  IGNORED, ///< Continue bubbling along the original ancestry.
  HANDLED  ///< Stop bubbling; this registration consumed the event.
};

/** @brief Distinguish a miss/ineligible target from an eligible unhandled event. */
enum class DispatchStatus {
  NO_TARGET, ///< No eligible destination was selected.
  UNHANDLED, ///< An eligible destination existed but no handler consumed the event.
  HANDLED    ///< A handler or built-in focus traversal consumed the event.
};

/** @brief Owning routing outcome; empty identities mean no eligible target/handler. */
struct DispatchResult {
  NodeId target{};                                  ///< Initial target, possibly removed during delivery.
  NodeId handler{};                                 ///< Node that consumed the event, possibly removed during delivery.
  DispatchStatus status{DispatchStatus::NO_TARGET}; ///< Delivery outcome; traversal may handle without a handler.
};

/** @brief Bounded router storage; no event queue is created. */
struct EventRouterOptions {
  std::size_t max_registrations{4096}; ///< Positive maximum; one registration per node.
};

namespace detail {
struct ActionRegistration;
struct ActionCancellation;
} // namespace detail

/** @brief Move-only scoped disconnect token; never owns a tree or widget.
 * Disconnect/destruction may run on any thread. It prevents future callback admission;
 * an already admitted callback finishes on UI. Captures are reclaimed on UI at the next
 * collecting UI operation or router destruction. Tokens may safely outlive the router.
 */
class ActionBinding {
public:
  /** @brief Empty disconnected token. */
  ActionBinding() = default;
  /** @brief Disconnect without invoking user code. */
  ~ActionBinding();
  /** @brief Transfer disconnect responsibility.
   * @param other Token to empty and transfer into this token; valid on any thread.
   */
  ActionBinding(ActionBinding &&other) noexcept;
  /** @brief Disconnect old registration, then transfer.
   * @param other Token to empty and transfer after disconnecting this registration; self-assignment is a no-op.
   * @return This token after the transfer.
   */
  ActionBinding &operator=(ActionBinding &&other) noexcept;
  ActionBinding(const ActionBinding &) = delete;
  ActionBinding &operator=(const ActionBinding &) = delete;
  /** @brief Idempotent; active calls retain their captures until return. */
  void disconnect() noexcept;

private:
  friend class EventRouter;
  explicit ActionBinding(std::weak_ptr<detail::ActionCancellation> registration);
  std::weak_ptr<detail::ActionCancellation> registration_;
};

/** @brief UI-owned focus and synchronous bubbling over an existing WidgetTree.
 * Tree and Scheduler must outlive the router. Destroy on its creating UI thread outside
 * a router callback; violation terminates. All operations reject wrong-thread calls before
 * touching user state. Dispatch/invoke/bind reject nested dispatch with WOULD_BLOCK.
 * References borrowed by handlers cannot escape. No terminal bytes or queue are produced.
 * Allocation failures become NO_MEMORY; callback exceptions become CALLBACK_FAILED.
 * Focus changes request paint; geometry invalidation never disables keyboard access.
 */
class EventRouter {
public:
  /** @brief Validate the tree's owner thread and create a bounded router.
   * @param tree Borrowed live tree whose UI thread owns this router; it must outlive the router.
   * @param options Positive maximum number of live registrations.
   * @return New router, the tree's root-access error, INVALID_ARGUMENT for zero capacity, or NO_MEMORY.
   */
  static std::expected<std::unique_ptr<EventRouter>, TreeError> create(WidgetTree &tree,
                                                                       EventRouterOptions options = {});
  /** @brief Disconnect and reclaim all captures on UI; tree must still exist. */
  ~EventRouter();
  EventRouter(const EventRouter &) = delete;
  EventRouter &operator=(const EventRouter &) = delete;

  /** @brief Register one typed handler; the scoped token owns disconnect responsibility.
   * Callback must be copy-constructible and return EventReply. Invocation uses tree.edit,
   * retaining the widget and callback captures even if either is removed during the call.
   * Duplicate registration is INVALID_ARGUMENT; full registry is NODE_LIMIT. Removed or
   * disconnected records are reclaimed before accepting a new registration.
   * @tparam T Widget or a publicly derived extension type represented by node.
   * @tparam Callback Copy-constructible callable invocable as an lvalue with T& and const RoutedEvent&,
   * returning exactly EventReply.
   * @param node Live target; hidden or disabled nodes can register but cannot receive input until eligible.
   * @param callback Owned handler; borrowed widget/event references must not escape the invocation.
   * @param focusable Whether this registration participates in keyboard focus and traversal.
   * @return Disconnect token; tree validation errors, WOULD_BLOCK during dispatch/frame/collection,
   * INVALID_ARGUMENT for a duplicate, NODE_LIMIT for capacity, NO_MEMORY, or CALLBACK_FAILED.
   */
  template <class T, class Callback>
  std::expected<ActionBinding, TreeError> bind(Node<T> node, Callback &&callback, bool focusable = true) {
    static_assert(std::same_as<std::invoke_result_t<Callback &, T &, const RoutedEvent &>, EventReply>);

    auto allowed = canBind(node);

    if (!allowed) {
      return std::unexpected(allowed.error());
    }

    try {
      return add(
        node,
        [this, node, function = std::forward<Callback>(callback)](
          const RoutedEvent &event) mutable -> std::expected<EventReply, TreeError> {
          EventReply reply{EventReply::IGNORED};
          auto status = tree().edit(node, [&](T &widget) {
            reply = std::invoke(function, widget, event);
          });

          if (!status) {
            return std::unexpected(status.error());
          }

          return reply;
        },
        focusable);
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    } catch (...) {
      return std::unexpected(TreeError::CALLBACK_FAILED);
    }
  }

  /** @brief Recover focus if needed; empty Node means no eligible focusable registration.
   * Recovery chooses the most recently focused eligible node, then the first preorder node.
   * @return Eligible focused node (possibly recovered), an empty observer if none exists,
   * or tree/router validation errors or NO_MEMORY. Recovery requests paint when focus changes.
   */
  std::expected<Node<Widget>, TreeError> focused();
  /** @brief Focus a live eligible registered node, or return INVALID_ARGUMENT; requests paint.
   * @param node Live, eligible node with a focusable registration.
   * @return Success, INVALID_ARGUMENT for an ineligible/unregistered/nonfocusable node,
   * tree/router validation errors, or NO_MEMORY.
   */
  std::expected<void, TreeError> setFocus(Node<Widget> node);
  /** @brief Traverse eligible registrations in tree preorder with wrap; reverse implements Shift+Tab.
   * @param reverse Traverse backward when true, forward otherwise; wrap at either end.
   * @return Newly focused node, empty if no eligible registration exists, or tree/router errors or NO_MEMORY.
   */
  std::expected<Node<Widget>, TreeError> traverse(bool reverse = false);
  /** @brief Hit target in paint order; descendants and later sibling subtrees take precedence.
   * Only fresh, clipped geometry participates. Returns an empty Node for a miss.
   * Nonregistered nodes may be targets and bubble to ancestors.
   * @param point Surface cell coordinate tested against arranged rectangles and ancestor clips.
   * @return Last hit in preorder (later siblings cover earlier ones), empty on a miss,
   * or tree/router validation errors or NO_MEMORY.
   */
  std::expected<Node<Widget>, TreeError> hitTest(CellPoint point);
  /** @brief Deliver decoded input synchronously; replies/issues are ignored.
   * Unmodified Tab/Shift+Tab traverse. Enter/Escape/arrows/Home/End/Backspace/Delete map to actions;
   * release and other keys remain KeyEvent. Text (including space), paste and mouse retain type.
   * Left press focuses the nearest focusable target ancestor before bubbling the mouse event.
   * @param event Decoded input borrowed for this call; handlers must not retain references.
   * @return Initial target, consuming handler and delivery status; NO_TARGET for a miss/ignored decoder event,
   * tree/router errors, WOULD_BLOCK for nested delivery, NO_MEMORY, or CALLBACK_FAILED.
   */
  std::expected<DispatchResult, TreeError> dispatch(const InputEvent &event);
  /** @brief Invoke the same handler path with a semantic command; target need not be focusable.
   * Foreign/stale targets return their tree error. Ineligible targets return DispatchStatus::NO_TARGET.
   * @param target Explicit live destination; focus is unchanged.
   * @param action Owning semantic command and modifier intent to bubble along the original ancestry.
   * @return Initial target, consuming handler and delivery status; tree/router errors,
   * WOULD_BLOCK for nested delivery, NO_MEMORY, or CALLBACK_FAILED.
   */
  std::expected<DispatchResult, TreeError> invoke(Node<Widget> target, ActionEvent action);

private:
  struct State;
  explicit EventRouter(std::unique_ptr<State> state);
  WidgetTree &tree();
  std::expected<void, TreeError> check() const;
  std::expected<void, TreeError> canBind(Node<Widget> node);
  std::expected<ActionBinding, TreeError>
  add(Node<Widget> node, std::function<std::expected<EventReply, TreeError>(const RoutedEvent &)> callback,
      bool focusable);
  std::expected<void, TreeError> collect();
  std::shared_ptr<detail::ActionRegistration> registration(Node<Widget> node) const;
  std::expected<std::vector<Node<Widget>>, TreeError> focusOrder();
  std::expected<void, TreeError> changeFocus(Node<Widget> node);
  std::expected<DispatchResult, TreeError> route(Node<Widget> target, const RoutedEvent &event);
  std::unique_ptr<State> state_;
};
} // namespace portals
