// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/widget.hpp>

#include <algorithm>
#include <limits>
#include <optional>
#include <thread>
#include <utility>

namespace portals {
/** @brief Immutable owning value/revision pair; revision zero is the initial value.
 * T must own its data transitively; mutable aliases must not change published snapshots.
 * @tparam T Value owning its data transitively; published reachable storage must remain immutable.
 */
template <class T> struct PropertyChange {
  T value;                  ///< Value captured at this revision, independent of the source lifetime.
  std::uint64_t revision{}; ///< Monotonic revision within this property, never wrapped.
};

/** @brief Finite observer endpoint storage, independent of Scheduler queue capacity. */
struct PropertyOptions {
  std::size_t max_observers{64}; ///< Maximum live or executing observers; must be positive.
};

/** @brief A successful set result; notification rejection does not undo the committed value.
 * Counts describe publication attempts, not callback execution. Accepted events can still
 * be cancelled by source destruction, Subscription disconnect, node removal or scheduler stop.
 * Equal set is a no-op and never retries notifications rejected by an earlier set.
 * @tparam T The property value type; its immutable snapshot owns all transitively reachable data.
 */
template <class T> struct PropertyUpdate {
  std::shared_ptr<const PropertyChange<T>> snapshot;  ///< Owning snapshot of this operation's revision.
  bool changed{};                                     ///< False means all publication counts are zero.
  std::size_t accepted{};                             ///< Events accepted by the existing Scheduler.
  std::size_t rejected{};                             ///< Live endpoints whose publication was rejected.
  std::size_t cancelled{};                            ///< Endpoints found closed during publication.
  std::optional<SchedulerError> notification_error{}; ///< First publication failure, including closed.
};

/** @brief UI-owned typed value with node-scoped observers and explicit partial delivery.
 * WidgetTree and its Scheduler are borrowed and must outlive this object. All operations
 * and destruction use the UI thread. Retained snapshots may outlive every UI owner.
 * Each changed set invalidates the source target synchronously, then publishes in observer
 * registration order on the data lane with reject overflow. No atomic multicast is promised.
 * Callbacks run only through Scheduler between frames; reentrant set appends notifications
 * after already accepted events. Scheduler budgets bound a self-updating callback chain.
 * T construction/copy/move/comparison and callback construction/move must not reenter UI APIs.
 * T destruction may reenter after replacement: the old value is retained until all publication
 * attempts finish. Destructors must not throw or call members of an object being destroyed.
 * @tparam T Equality-comparable value constructible from T&&, owning its data transitively. Published
 * aliases must remain immutable; construction/copy/move/comparison must not reenter UI APIs and destruction must not
 * throw. Operations of T must not terminate; a noexcept move that allocates cannot report failure through this API.
 * Const-reference overloads additionally require an actual valid copy operation, rather than relying on copy traits.
 */
template <std::equality_comparable T> class Property {
public:
  /** @brief Create a source bound to one checked invalidation target; no initial notification.
   * The target must belong to tree and be live outside a frame. Invalid kind/zero capacity
   * is INVALID_ARGUMENT; allocation failure is NO_MEMORY. T exceptions are CALLBACK_FAILED.
   * The const-reference overload copies directly inside the error boundary; T must support copying.
   * @param tree Borrowed UI-owned tree and scheduler, both required to outlive the property.
   * @param target Live node in tree invalidated by each changed set.
   * @param initial Borrowed value copied into immutable revision zero; retained only during this call.
   * @param kind Invalidation to request on change; none explicitly requests no work.
   * @param options Positive observer endpoint limit.
   * @return New property, tree validation/frame errors, INVALID_ARGUMENT, NO_MEMORY, or CALLBACK_FAILED.
   */
  static std::expected<std::unique_ptr<Property>, TreeError>
  create(WidgetTree &tree, Node<Widget> target, const T &initial, Invalidation kind, PropertyOptions options = {}) {
    return createImpl(tree, target, initial, kind, options);
  }

  /** @brief Create a source by moving its initial value directly into the snapshot.
   * Validation and lifetime match the const-reference overload. There is no intermediate T.
   * Escaping exceptions are translated. Termination inside a noexcept T move cannot be caught.
   *
   * @param tree Borrowed UI tree and scheduler; both must outlive this property.
   * @param target Live node invalidated by each changed set.
   * @param initial Candidate moved after target/options validation; borrowed until return.
   * @param kind Invalidation requested on change; NONE requests no work.
   * @param options Positive observer endpoint limit.
   * @return New property or validation/frame error, INVALID_ARGUMENT, NO_MEMORY, CALLBACK_FAILED.
   */
  static std::expected<std::unique_ptr<Property>, TreeError> create(WidgetTree &tree, Node<Widget> target, T &&initial,
                                                                    Invalidation kind, PropertyOptions options = {}) {
    return createImpl(tree, target, std::move(initial), kind, options);
  }

  /** @brief Logically close the source; executing callbacks retain their captures until return.
   * Existing Subscription tokens remain ordinary Scheduler tokens and may be disconnected
   * later. Their callbacks are suppressed after source closure. Wrong-thread destruction
   * terminates, matching the UI ownership contract of Subscription and WidgetTree.
   */
  ~Property() {
    if (state_->owner != std::this_thread::get_id()) {
      std::terminate();
    }

    state_->alive = false;
  }

  Property(const Property &) = delete;
  Property &operator=(const Property &) = delete;

  /** @brief Obtain an independent immutable owner, even after the target node was removed.
   * Access on another thread returns WRONG_THREAD. Transfer requires T to support destruction
   * on the thread releasing the last owner; UI reentry from T destruction requires a live UI context.
   * @return Owning immutable value/revision pair, or WRONG_THREAD without accessing the value.
   */
  std::expected<std::shared_ptr<const PropertyChange<T>>, TreeError> snapshot() const {
    if (state_->owner != std::this_thread::get_id()) {
      return std::unexpected(TreeError::WRONG_THREAD);
    }

    return state_->current;
  }

  /** @brief Commit a changed value and invalidate its target before attempting notifications.
   * unexpected means no library-controlled value, revision, queue or invalidation change. A successful
   * changed receipt may contain rejected/cancelled notifications. Never retry set as a way
   * to deliver the missing events: equal set is a no-op. Consumers can read snapshot to resync.
   * Tree affinity/session/lifetime/frame failures propagate; revision overflow is
   * IDENTITY_EXHAUSTED. Allocation or T comparison/copy failures occur before commit.
   * @param value Borrowed candidate compared first and copied directly into the snapshot only if changed; T must
   * support copying.
   * @return Receipt for this operation's immutable revision and per-observer queue acceptance, including
   * partial failure; an unexpected TreeError means no library-controlled commit occurred.
   */
  std::expected<PropertyUpdate<T>, TreeError> set(const T &value) {
    return setImpl(value);
  }

  /** @brief Commit a candidate by moving it directly into the immutable snapshot.
   * Commit and notification semantics match set(const T&). There is no intermediate T.
   * Escaping exceptions are translated. Termination inside a noexcept T move cannot be caught.
   *
   * @param value Candidate compared first and moved only for a changed value. After construction
   * it is moved-from, including when later invalidation failure rejects the commit.
   * @return Exact snapshot/notification receipt, or a TreeError without a property commit.
   */
  std::expected<PropertyUpdate<T>, TreeError> set(T &&value) {
    return setImpl(std::move(value));
  }

  /** @brief Register an ordinary Subscription tied to a receiving node in the same tree.
   * The callback owns its captures and receives (W&, shared_ptr<const PropertyChange<T>>).
   * The entire callback borrows W through WidgetTree::connect, so removal during invocation
   * defers widget destruction. Source destruction suppresses later callbacks independently.
   * Registration does not send the initial value or invalidate the receiving node; the source
   * target is invalidated by set. Additional receiver changes request their own invalidation.
   * Expired endpoint records are reclaimed before the finite limit is checked (NODE_LIMIT).
   * Reentrant destruction during registration rechecks capacity and source lifetime (closed).
   * Tree/Scheduler errors propagate; callback construction exceptions become CALLBACK_FAILED.
   * @tparam W Widget or a publicly derived extension type represented by receiver.
   * @tparam Callback Copy-constructible callable accepting W& and std::shared_ptr<const PropertyChange<T>>
   * as an lvalue; its return value is ignored.
   * @param receiver Live receiving node in the property's tree; removal cancels its scoped callback.
   * @param callback Owned callable; widget references must not escape, but the snapshot owner may be retained.
   * @return Subscription token with no initial delivery; tree/scheduler errors, NODE_LIMIT, closed,
   * NO_MEMORY, or CALLBACK_FAILED on registration failure.
   */
  template <class W, class Callback>
  std::expected<Subscription, TreeError> observe(Node<W> receiver, Callback &&callback) {
    auto state = state_;

    if (state->owner != std::this_thread::get_id()) {
      return std::unexpected(TreeError::WRONG_THREAD);
    }

    auto allowed = state->tree.withScope(receiver, [](const UiScope &) {
    });

    if (!allowed) {
      return std::unexpected(allowed.error());
    }

    state->prune();

    if (state->observers.size() >= state->limit) {
      return std::unexpected(TreeError::NODE_LIMIT);
    }

    try {
      auto lifetime = std::make_shared<Lifetime>();
      auto connection = state->tree.template connect<PropertyChange<T>>(
        receiver, [source = std::weak_ptr<State>(state), lifetime, function = std::forward<Callback>(callback)](
                    W &widget, std::shared_ptr<const PropertyChange<T>> event) mutable {
          auto locked = source.lock();

          if (locked && locked->alive) {
            std::invoke(function, widget, std::move(event));
          }
        });

      if (!connection) {
        return std::unexpected(connection.error());
      }

      // Moved-from callback destructors can reenter while connect materializes its captures.
      if (!state->alive) {
        return std::unexpected(TreeError::CLOSED);
      }

      state->prune();

      if (state->observers.size() >= state->limit) {
        return std::unexpected(TreeError::NODE_LIMIT);
      }

      state->observers.push_back(Observer{std::move(connection->publisher), lifetime});

      return std::move(connection->subscription);
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    } catch (...) {
      return std::unexpected(TreeError::CALLBACK_FAILED);
    }
  }

private:
  /** @brief Construct the initial snapshot within one shared exception boundary.
   * @tparam Value Deduced T or const T reference, preserving the selected overload's value category.
   * @param tree Borrowed UI owner used to validate target and retain its source association.
   * @param target Live invalidation target in tree.
   * @param initial Candidate forwarded directly into revision zero after validation.
   * @param kind Requested invalidation category for later changes.
   * @param options Positive observer storage bound.
   * @return Owning property, or a delegated validation/allocation/construction error.
   */
  template <class Value>
  static std::expected<std::unique_ptr<Property>, TreeError>
  createImpl(WidgetTree &tree, Node<Widget> target, Value &&initial, Invalidation kind, PropertyOptions options) {
    auto allowed = tree.withScope(target, [](const UiScope &) {
    });

    if (!allowed) {
      return std::unexpected(allowed.error());
    }

    if (!options.max_observers ||
        (kind != Invalidation::NONE && kind != Invalidation::PAINT && kind != Invalidation::LAYOUT)) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    try {
      auto state = std::make_shared<State>(tree, target, kind);

      if (options.max_observers > state->observers.max_size()) {
        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }

      state->observers.reserve(options.max_observers);
      state->limit = options.max_observers;
      state->current = std::make_shared<const PropertyChange<T>>(std::forward<Value>(initial), 0);

      return std::unique_ptr<Property>(new Property(std::move(state)));
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    } catch (...) {
      return std::unexpected(TreeError::CALLBACK_FAILED);
    }
  }

  /** @brief Prepare, commit and publish without creating an intermediate T.
   * @tparam Value Deduced T or const T reference, preserving the caller's copy/move choice.
   * @param value Candidate compared first and forwarded only when it differs from the current value.
   * @return Exact operation receipt, or an error before any library-controlled commit.
   */
  template <class Value> std::expected<PropertyUpdate<T>, TreeError> setImpl(Value &&value) {
    auto state = state_;

    if (state->owner != std::this_thread::get_id()) {
      return std::unexpected(TreeError::WRONG_THREAD);
    }

    auto allowed = state->tree.withScope(state->target, [](const UiScope &) {
    });

    if (!allowed) {
      return std::unexpected(allowed.error());
    }

    // Retain the old value through every publication: its final destructor may reenter set.
    // The receipt below must continue to identify this operation even if that reentry commits again.
    auto previous = state->current;
    std::shared_ptr<const PropertyChange<T>> next;

    try {
      if (previous->value == value) {
        if (auto *recorder = state->tree.diagnostics(); recorder && recorder->enabled()) {
          const auto id = state->target.id();

          recorder->note(DiagnosticEvent::PROPERTY, DiagnosticReason::VALUE_UNCHANGED,
                         {id.session, id.slot, id.generation});
        }

        return PropertyUpdate<T>{previous};
      }

      if (previous->revision == std::numeric_limits<std::uint64_t>::max()) {
        return std::unexpected(TreeError::IDENTITY_EXHAUSTED);
      }

      next = std::make_shared<const PropertyChange<T>>(std::forward<Value>(value), previous->revision + 1);
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    } catch (...) {
      return std::unexpected(TreeError::CALLBACK_FAILED);
    }

    auto invalidated = state->tree.invalidate(state->target, state->kind);

    if (!invalidated) {
      return std::unexpected(invalidated.error());
    }

    // Invalidation is the last fallible precommit step. Once current changes, bounded queue
    // rejection is part of the receipt, never a reason to roll back the visible value.
    state->current = next;

    if (auto *recorder = state->tree.diagnostics(); recorder && recorder->enabled()) {
      const auto id = state->target.id();

      recorder->note(DiagnosticEvent::PROPERTY, DiagnosticReason::VALUE_CHANGED, {id.session, id.slot, id.generation});
    }

    state->prune();

    PropertyUpdate<T> result{next, true};

    for (auto &observer : state->observers) {
      auto published = observer.publisher.publish(next);

      if (published) {
        ++result.accepted;
      } else {
        if (!result.notification_error) {
          result.notification_error = published.error();
        }

        if (published.error() == SchedulerError::CLOSED) {
          ++result.cancelled;
        } else {
          ++result.rejected;
        }
      }
    }

    state->prune();

    return result;
  }

  struct Lifetime {};

  struct Observer {
    Publisher<PropertyChange<T>> publisher;
    std::weak_ptr<Lifetime> lifetime;
  };

  struct State {
    State(WidgetTree &value, Node<Widget> node, Invalidation invalidation)
        : tree(value), target(node), kind(invalidation) {
    }

    void prune() {
      std::erase_if(observers, [](const Observer &observer) {
        return observer.lifetime.expired();
      });
    }

    WidgetTree &tree;
    Node<Widget> target;
    Invalidation kind;
    const std::thread::id owner{std::this_thread::get_id()};
    bool alive{true};
    std::size_t limit{};
    std::shared_ptr<const PropertyChange<T>> current;
    std::vector<Observer> observers = std::vector<Observer>(0);
  };

  explicit Property(std::shared_ptr<State> state) : state_(std::move(state)) {
  }

  std::shared_ptr<State> state_;
};
} // namespace portals
