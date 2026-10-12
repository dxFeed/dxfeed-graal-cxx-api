// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/widget.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace portals {
/** @brief Provider-assigned row identity, independent of position; increment generation on reuse. */
struct RowId {
  std::uint64_t value{};      ///< Nonzero identity within one model.
  std::uint64_t generation{}; ///< Nonzero generation; never wrap or reuse an exhausted pair.
  /** @brief Compare both identity components.
   * @param other Row identity to compare.
   * @return True when all compared fields are equal.
   */
  bool operator==(const RowId &other) const noexcept = default;
};

/** @brief Provider-assigned column identity; typed row accessors define column values. */
struct ColumnId {
  std::uint64_t value{};      ///< Nonzero identity within one model.
  std::uint64_t generation{}; ///< Nonzero generation, independent of visual order.
  /** @brief Compare identity.
   * @param other Column identity to compare.
   * @return True when all compared fields are equal.
   */
  bool operator==(const ColumnId &other) const noexcept = default;
};

/** @brief Half-open logical row interval; arithmetic is checked before provider access. */
struct RowRange {
  std::uint64_t first{}; ///< First logical row.
  std::size_t count{};   ///< Number of rows, bounded independently of logical model size.
  /** @brief Compare interval.
   * @param other Interval to compare.
   * @return True when all compared fields are equal.
   */
  bool operator==(const RowRange &other) const noexcept = default;
};
/** @brief Observable asynchronous range readiness. */
enum class RangeState {
  PENDING, ///< Data is not ready; reading must not block or start work.
  READY,   ///< Exactly the requested immutable rows are available.
  FAILED   ///< Provider failure is represented by snapshot-owned text and no rows.
};

/** @brief A typed domain row and its stable identity; a table uses a typed record as T.
 * @tparam T Domain row value type; snapshot-owned reachable data must remain immutable and support destruction
 * on the final owner's thread. Library range/frame/request operations do not copy T values.
 */
template <class T> struct ModelRow {
  RowId id; ///< Stable provider identity, never the row index.
  T value;  ///< Typed value; all reachable data must be immutable while published.
};

/** @brief Provider result borrowing from its immutable snapshot.
 * @tparam T Domain row value type; snapshot-owned reachable data must remain immutable and support destruction
 * on the final owner's thread. Library range/frame/request operations do not copy T values.
 */
template <class T> struct ModelRows {
  RangeState state{RangeState::PENDING}; ///< Pending never blocks or initiates I/O.
  std::span<const ModelRow<T>> rows;     ///< Exactly the requested count when ready; otherwise empty.
  std::string_view error;                ///< Snapshot-owned description when error; otherwise empty.
};

/** @brief Identity lookup capability and outcome for a pinned provider snapshot. */
enum class RowLookupState {
  UNSUPPORTED, ///< Provider does not implement an identity index; ordinal scrolling remains available.
  NOT_FOUND,   ///< Provider supports lookup, but this full RowId is absent.
  FOUND        ///< The index identifies this RowId in the same snapshot.
};

/** @brief Owning lookup result; no row storage is borrowed. */
struct RowLookup {
  RowLookupState state{RowLookupState::UNSUPPORTED}; ///< Explicit capability/result, never inferred from an index.
  std::uint64_t index{};                             ///< Logical index, meaningful only for FOUND.
};

/** @brief Immutable list/table provider storage, independent of view/source lifetime.
 * Implement read as bounded in-memory access, without I/O, waiting or domain mutation.
 * Every ready range of one data revision must agree, including overlapping ranges from
 * different snapshot objects. Shared ownership does not make mutable aliases safe.
 * Snapshots and transitively owned T values must support destruction on the last owner's
 * thread; UI reentry from a destructor requires a still-live UI context.
 * @tparam T Domain row value type; snapshot-owned reachable data must remain immutable and support destruction
 * on the final owner's thread. Library range/frame/request operations do not copy T values.
 */
template <class T> class ModelSnapshot {
public:
  /** @brief Define immutable metadata.
   * @param revision Provider data revision; full replacements apply only when strictly newer.
   * @param rows Logical row count, independent of how much data is materialized.
   */
  ModelSnapshot(std::uint64_t revision, std::uint64_t rows) : revision_(revision), rows_(rows) {
  }

  /** @brief Reclaim storage after all ranges, frames and workers release it. */
  virtual ~ModelSnapshot() = default;

  /** @brief Provider data revision; strictly increasing on replacement.
   * @return Immutable provider data revision supplied at construction.
   */
  std::uint64_t revision() const noexcept {
    return revision_;
  }

  /** @brief Logical count, without materializing rows.
   * @return Logical number of rows supplied at construction; no rows are loaded.
   */
  std::uint64_t rowCount() const noexcept {
    return rows_;
  }

  /** @brief Borrow a checked interval; may return pending/error.
   * @param range Interval already validated by ModelRange::read; direct callers must validate bounds.
   * @return Borrowed rows/error text owned by this snapshot: ready has exactly range.count rows and no error;
   * pending has no rows or error; error has no rows. The provider must keep returned storage alive.
   */
  virtual ModelRows<T> read(RowRange range) const = 0;

  /** @brief Resolve stable identity without materializing rows or blocking the UI.
   * @param id Full nonzero row identity, including generation.
   * @return FOUND with this snapshot's index, NOT_FOUND if absent, or UNSUPPORTED by default.
   * @note Override with a bounded identity index to support stable viewport anchoring and ensure-visible.
   * Do not scan the full model, perform I/O, mutate domain data, or retain the argument.
   * Called during preparation on UI, never paint; exceptions are translated by ModelFrame::locateRow.
   */
  virtual RowLookup locateRow(RowId id) const {
    (void)id;

    return {};
  }

private:
  const std::uint64_t revision_;
  const std::uint64_t rows_;
};

/** @brief Stable identity of a scoped range consumer, distinct even for equal row intervals. */
struct ModelRangeId {
  std::uint64_t session{}; ///< Process-unique model session.
  std::uint64_t value{};   ///< Never-reused consumer identity.
  /** @brief Compare source and consumer.
   * @param other Consumer identity to compare.
   * @return True when all compared fields are equal.
   */
  bool operator==(const ModelRangeId &other) const noexcept = default;
};

/** @brief Finite per-model storage and work budgets; all fields must be positive. */
struct ModelOptions {
  std::size_t max_range_rows{4096}; ///< Largest single read/request.
  std::size_t max_consumers{32};    ///< Live scoped range slots, regardless of readiness.
  std::size_t max_in_flight{64};    ///< Request owners, including cancelled and queued work not yet released.
};

/** @brief Independent owner for borrowed rows and provider error text.
 * @tparam T Domain row value type; snapshot-owned reachable data must remain immutable and support destruction
 * on the final owner's thread. Library range/frame/request operations do not copy T values.
 */
template <class T> class ModelRange {
public:
  /** @brief Empty pending result. */
  ModelRange() = default;

  /** @brief Retain the same immutable ownership.
   * @param other Owner whose immutable storage and metadata are shared; domain rows are not copied.
   */
  ModelRange(const ModelRange &other) = default;
  /** @brief Transfer ownership without releasing old storage.
   * @param other Owner whose storage is transferred; do not read moved-from borrowed data.
   */
  ModelRange(ModelRange &&other) noexcept = default;

  /** @brief Replace all fields before releasing old storage, whose destructor may reenter.
   * @param other Replacement owner passed by value; old storage is released after all fields are replaced.
   * @return This owner; do not use the reference if reentrant destruction of old storage destroyed this object.
   */
  ModelRange &operator=(ModelRange other) noexcept {
    std::swap(owner_, other.owner_);
    std::swap(range_, other.range_);
    std::swap(data_, other.data_);

    return *this;
  }

  /** @brief Read readiness without loading.
   * @return Captured pending/ready/error readiness; this query never invokes the provider.
   */
  RangeState state() const noexcept {
    return data_.state;
  }

  /** @brief Borrow until this owner is released.
   * @return Borrowed ready row span, empty for pending/error; retain this range or its snapshot owner.
   */
  std::span<const ModelRow<T>> rows() const noexcept {
    return data_.rows;
  }

  /** @brief Borrow error text for this owner's lifetime.
   * @return Borrowed provider error text, empty outside error state; retain the owning snapshot.
   */
  std::string_view error() const noexcept {
    return data_.error;
  }

  /** @brief Logical interval of this result.
   * @return Logical interval whose provider result is retained by this owner.
   */
  RowRange range() const noexcept {
    return range_;
  }

  /** @brief Provider revision.
   * @return Provider revision, or zero for an empty range owner.
   */
  std::uint64_t dataRevision() const noexcept {
    return owner_ ? owner_->revision() : 0;
  }

  /** @brief Read only memory and retain its snapshot. Bounds/provider contract failures are explicit.
   * Allocation exceptions become NO_MEMORY, other provider exceptions CALLBACK_FAILED.
   * @param owner Non-null immutable provider snapshot retained by the returned range.
   * @param range Half-open interval to validate and read synchronously.
   * @param max_rows Maximum accepted count; zero allows only empty ranges.
   * @return Independent range owner; INVALID_ARGUMENT for bounds or malformed result shape,
   * NO_MEMORY for allocation exceptions, or CALLBACK_FAILED for other provider exceptions.
   */
  static std::expected<ModelRange, TreeError> read(std::shared_ptr<const ModelSnapshot<T>> owner, RowRange range,
                                                   std::size_t max_rows) {
    if (!owner || range.count > max_rows || range.first > owner->rowCount() ||
        range.count > owner->rowCount() - range.first) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    try {
      auto rows = owner->read(range);

      if ((rows.state == RangeState::READY && (rows.rows.size() != range.count || !rows.error.empty())) ||
          (rows.state != RangeState::READY && !rows.rows.empty()) ||
          (rows.state == RangeState::PENDING && !rows.error.empty()) ||
          (rows.state != RangeState::READY && rows.state != RangeState::PENDING && rows.state != RangeState::FAILED)) {
        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }

      return ModelRange(std::move(owner), range, rows);
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    } catch (...) {
      return std::unexpected(TreeError::CALLBACK_FAILED);
    }
  }

private:
  template <class> friend class ModelConsumer;
  template <class> friend class Model;
  template <class> friend class ModelFrame;

  ModelRange(std::shared_ptr<const ModelSnapshot<T>> owner, RowRange range, ModelRows<T> data)
      : owner_(std::move(owner)), range_(range), data_(data) {
  }

  std::shared_ptr<const ModelSnapshot<T>> owner_;
  RowRange range_;
  ModelRows<T> data_;
};

/// @cond INTERNAL
namespace detail {
std::expected<std::uint64_t, TreeError> modelSession();

struct ModelLifetime {
  std::atomic<bool> alive{true};
  std::atomic<std::size_t> outstanding{};
};

struct ModelLease {
  ModelLease(std::shared_ptr<ModelLifetime> life, std::shared_ptr<const void> storage)
      : lifetime(std::move(life)), storage(std::move(storage)) {
    lifetime->outstanding.fetch_add(1);
  }

  ~ModelLease() {
    // Reentrant storage destruction must still see the outstanding reservation.
    storage.reset();
    lifetime->outstanding.fetch_sub(1);
  }

  std::shared_ptr<ModelLifetime> lifetime;
  std::shared_ptr<const void> storage;
  std::atomic<bool> cancelled{};
  std::atomic<bool> submitted{};
};

template <class T> struct ModelEntry {
  ModelRangeId id;
  std::uint64_t generation{};
  ModelRange<T> result;
  std::weak_ptr<ModelLease> work;
};

template <class T> struct ModelFrameData {
  explicit ModelFrameData(std::shared_ptr<const ModelSnapshot<T>> initial) : snapshot(std::move(initial)) {
  }

  std::shared_ptr<const ModelSnapshot<T>> snapshot;
  std::vector<ModelEntry<T>> entries = std::vector<ModelEntry<T>>(0);
  std::uint64_t revision{};
};

template <class T> struct ModelCompletion {
  ModelRangeId id;
  std::uint64_t generation{};
  std::shared_ptr<ModelLease> lease;
  ModelRange<T> result;
};

template <class T> struct ModelState {
  ModelState(WidgetTree &value, Node<Widget> node) : tree(value), target(node) {
  }

  std::expected<void, TreeError> allowed() const {
    if (owner != std::this_thread::get_id()) {
      return std::unexpected(TreeError::WRONG_THREAD);
    }

    if (!lifetime->alive.load()) {
      return std::unexpected(TreeError::CLOSED);
    }

    return tree.withScope(target, [](const UiScope &) {
    });
  }

  std::expected<std::shared_ptr<ModelFrameData<T>>, TreeError> next() const {
    if (current->revision == std::numeric_limits<std::uint64_t>::max()) {
      return std::unexpected(TreeError::IDENTITY_EXHAUSTED);
    }

    try {
      auto result = std::make_shared<ModelFrameData<T>>(*current);

      ++result->revision;

      return result;
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    }
  }

  std::expected<void, TreeError> commit(std::shared_ptr<ModelFrameData<T>> next) {
    auto invalidated = tree.invalidate(target, Invalidation::LAYOUT);

    if (!invalidated) {
      return std::unexpected(invalidated.error());
    }

    // Keep old storage until the new state and all bookkeeping are committed.
    auto previous = std::exchange(current, std::move(next));

    return {};
  }

  WidgetTree &tree;
  Node<Widget> target;
  const std::thread::id owner{std::this_thread::get_id()};
  ModelOptions options;
  std::uint64_t session{};
  std::uint64_t identity{};
  std::vector<ModelRangeId> retired = std::vector<ModelRangeId>(0);
  std::shared_ptr<ModelLifetime> lifetime = std::make_shared<ModelLifetime>();
  std::shared_ptr<const ModelFrameData<T>> current;
  Publisher<ModelCompletion<T>> publisher;
  std::optional<TreeError> application_error;
};
} // namespace detail

/// @endcond
/** @brief One immutable model revision retained by the frame owner for all its views.
 * Obtain once before WidgetTree::frame, pass the same value to every view, and retain it
 * until borrowed rendering finishes. Different models require application transactions.
 * @tparam T Domain row value type; snapshot-owned reachable data must remain immutable and support destruction
 * on the final owner's thread. Library range/frame/request operations do not copy T values.
 */
template <class T> class ModelFrame {
public:
  /** @brief Retain the same immutable ownership.
   * @param other Owner whose immutable storage and metadata are shared; domain rows are not copied.
   */
  ModelFrame(const ModelFrame &other) = default;
  /** @brief Transfer ownership without releasing old storage.
   * @param other Owner whose storage is transferred; do not read moved-from borrowed data.
   */
  ModelFrame(ModelFrame &&other) noexcept = default;

  /** @brief Replace all fields before releasing old storage, whose destructor may reenter.
   * @param other Replacement owner passed by value; old storage is released after all fields are replaced.
   * @return This owner; do not use the reference if reentrant destruction of old storage destroyed this object.
   */
  ModelFrame &operator=(ModelFrame other) noexcept {
    std::swap(data_, other.data_);
    std::swap(max_rows_, other.max_rows_);

    return *this;
  }

  /** @brief UI publication revision, including range readiness.
   * @return UI publication revision, including range request/readiness changes.
   */
  std::uint64_t revision() const noexcept {
    return data_->revision;
  }

  /** @brief Provider data revision.
   * @return Provider data revision retained by this frame.
   */
  std::uint64_t dataRevision() const noexcept {
    return data_->snapshot->revision();
  }

  /** @brief Logical rows.
   * @return Logical row count of the pinned base snapshot.
   */
  std::uint64_t rowCount() const noexcept {
    return data_->snapshot->rowCount();
  }

  /** @brief Read a consumer's exact result; stale/foreign consumer returns STALE_NODE. No spatial merge.
   * @param id Consumer identity looked up in this immutable table; older pins can retain retired consumers.
   * @return That consumer's independent result owner, or STALE_NODE if absent from this table.
   */
  std::expected<ModelRange<T>, TreeError> range(ModelRangeId id) const {
    auto entry = std::ranges::find_if(data_->entries, [id](const auto &value) {
      return value.id == id;
    });

    if (entry == data_->entries.end()) {
      return std::unexpected(TreeError::STALE_NODE);
    }

    return entry->result;
  }

  /** @brief Direct bounded read from the base snapshot, without asynchronous requests or overlays.
   * @param range Interval read from the pinned base snapshot under this model's row limit.
   * @return Independent range owner, INVALID_ARGUMENT for bounds/result shape, NO_MEMORY, or CALLBACK_FAILED.
   */
  std::expected<ModelRange<T>, TreeError> read(RowRange range) const {
    return ModelRange<T>::read(data_->snapshot, range, max_rows_);
  }

  /** @brief Resolve identity against the same retained snapshot as this frame's base rows.
   * @param id Full nonzero row identity to locate; no ownership is transferred.
   * @return Owning capability/result, INVALID_ARGUMENT for invalid identity/result or out-of-bounds index,
   * NO_MEMORY for allocation failure, or CALLBACK_FAILED for other provider exceptions.
   * @note UI preparation only, outside a tree frame. No requests or tree invalidation are performed.
   */
  std::expected<RowLookup, TreeError> locateRow(RowId id) const {
    if (id.value == 0 || id.generation == 0) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    try {
      const auto result = data_->snapshot->locateRow(id);

      switch (result.state) {
      case RowLookupState::UNSUPPORTED:
      case RowLookupState::NOT_FOUND:
        return result;
      case RowLookupState::FOUND:
        if (result.index < rowCount()) {
          return result;
        }

        break;
      }

      return std::unexpected(TreeError::INVALID_ARGUMENT);
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    } catch (...) {
      return std::unexpected(TreeError::CALLBACK_FAILED);
    }
  }

private:
  template <class> friend class Model;
  template <class> friend class VirtualList;
  template <class> friend class VirtualTable;
  template <class> friend class Chart;

  // A view may reuse its own immutable completion snapshot, without merging consumer results.
  // The current pin still supplies the domain revision, row count and synchronous read budget.
  std::expected<ModelRange<T>, TreeError> readCached(const ModelRange<T> &completed, const ModelRange<T> *retained,
                                                     RowRange range) const {
    const auto readOwner = [&](const ModelRange<T> &cached) -> std::expected<ModelRange<T>, TreeError> {
      if (!cached.owner_ || cached.owner_ == data_->snapshot || cached.dataRevision() != dataRevision() ||
          cached.owner_->rowCount() != rowCount()) {
        return ModelRange<T>{};
      }

      return ModelRange<T>::read(cached.owner_, range, max_rows_);
    };

    auto cached = readOwner(completed);

    if (!cached || cached->state() != RangeState::PENDING || !retained) {
      return cached;
    }

    auto previous = readOwner(*retained);

    if (!previous) {
      return std::unexpected(previous.error());
    }

    // A completion for another viewport can arrive just before a direction change.
    // Preserve the ready retained page; never turn provider failure into cached success.
    if (cached->dataRevision() == 0 || previous->state() != RangeState::PENDING) {
      return previous;
    }

    return cached;
  }

  ModelFrame(std::shared_ptr<const detail::ModelFrameData<T>> data, std::size_t max_rows)
      : data_(std::move(data)), max_rows_(max_rows) {
  }

  std::shared_ptr<const detail::ModelFrameData<T>> data_;
  std::size_t max_rows_;
};

/** @brief Copyable worker token; retains work storage and one reservation until its last owner dies.
 * Cancellation is a stop request, never a join. Worker code must finish/release its own
 * owners. Completion queues an owning result; no view callback runs on the worker.
 * @tparam T Domain row value type; snapshot-owned reachable data must remain immutable and support destruction
 * on the final owner's thread. Library range/frame/request operations do not copy T values.
 */
template <class T> class ModelRequest {
public:
  /** @brief Empty cancelled token. */
  ModelRequest() = default;

  /** @brief Retain the same immutable ownership.
   * @param other Owner whose immutable storage and metadata are shared; domain rows are not copied.
   */
  ModelRequest(const ModelRequest &other) = default;
  /** @brief Transfer ownership without releasing old storage.
   * @param other Owner whose storage is transferred; do not read moved-from borrowed data.
   */
  ModelRequest(ModelRequest &&other) noexcept = default;

  /** @brief Replace all fields before releasing old storage, whose destructor may reenter.
   * @param other Replacement owner passed by value; old storage is released after all fields are replaced.
   * @return This owner; do not use the reference if reentrant destruction of old storage destroyed this object.
   */
  ModelRequest &operator=(ModelRequest other) noexcept {
    std::swap(id_, other.id_);
    std::swap(generation_, other.generation_);
    std::swap(revision_, other.revision_);
    std::swap(row_count_, other.row_count_);
    std::swap(range_, other.range_);
    std::swap(lease_, other.lease_);
    std::swap(publisher_, other.publisher_);

    return *this;
  }

  /** @brief Requested rows.
   * @return Exact requested interval for this work generation; empty for a default token.
   */
  RowRange range() const noexcept {
    return range_;
  }

  /** @brief Required provider revision.
   * @return Required provider revision for completion; zero for a default token.
   */
  std::uint64_t dataRevision() const noexcept {
    return revision_;
  }

  /** @brief Worker-safe cooperative stop query.
   * @return True for an empty token, cancelled generation or destroyed source; context removal is
   * discovered by completion publication and need not make this query true.
   */
  bool stopRequested() const noexcept {
    return !lease_ || lease_->cancelled.load() || !lease_->lifetime->alive.load();
  }

  /** @brief Cancel only this generation; does not release its work budget. */
  void cancel() const noexcept {
    if (lease_) {
      lease_->cancelled.store(true);
    }
  }

  /** @brief Queue ready/error storage from the requested data revision; pending is invalid.
   * Exactly one successful submission per generation. Full mailbox returns WOULD_BLOCK
   * and permits retry; closed means cancellation/source closure/already submitted.
   * Snapshot metadata must match the original revision and logical row count.
   * @param snapshot Non-null immutable ready/error provider result with the requested revision and row count.
   * @return Queue acceptance (not UI application); closed for cancellation/duplicate/unavailable endpoint,
   * WOULD_BLOCK for a full queue, INVALID_ARGUMENT for metadata/result shape, NO_MEMORY, or CALLBACK_FAILED.
   */
  std::expected<PublishResult, SchedulerError> complete(std::shared_ptr<const ModelSnapshot<T>> snapshot) const {
    auto request = *this;

    if (request.stopRequested()) {
      return std::unexpected(SchedulerError::CLOSED);
    }

    if (!snapshot || snapshot->revision() != request.revision_ || snapshot->rowCount() != request.row_count_) {
      return std::unexpected(SchedulerError::INVALID_ARGUMENT);
    }

    auto result = ModelRange<T>::read(std::move(snapshot), request.range_, request.range_.count);

    if (!result || result->state() == RangeState::PENDING) {
      if (!result && result.error() == TreeError::NO_MEMORY) {
        return std::unexpected(SchedulerError::NO_MEMORY);
      }

      if (!result && result.error() == TreeError::CALLBACK_FAILED) {
        return std::unexpected(SchedulerError::CALLBACK_FAILED);
      }

      return std::unexpected(SchedulerError::INVALID_ARGUMENT);
    }

    // Admission is shared by all token copies. A failed enqueue releases the claim so the same
    // generation can retry; successful enqueue retains the lease until UI delivery releases it.
    bool expected = false;

    if (!request.lease_->submitted.compare_exchange_strong(expected, true)) {
      return std::unexpected(SchedulerError::CLOSED);
    }

    try {
      auto event = std::make_shared<const detail::ModelCompletion<T>>(request.id_, request.generation_, request.lease_,
                                                                      std::move(*result));
      auto published = request.publisher_.publish(std::move(event));

      if (!published) {
        request.lease_->submitted.store(false);
      }

      return published;
    } catch (const std::bad_alloc &) {
      request.lease_->submitted.store(false);

      return std::unexpected(SchedulerError::NO_MEMORY);
    }
  }

private:
  template <class> friend class ModelConsumer;
  ModelRangeId id_;
  std::uint64_t generation_{};
  std::uint64_t revision_{};
  std::uint64_t row_count_{};
  RowRange range_;
  std::shared_ptr<detail::ModelLease> lease_;
  Publisher<detail::ModelCompletion<T>> publisher_;
};

/** @brief Move-only UI-owned range slot. Equal/overlapping consumers are independent.
 * Close/destruction retires only this consumer; outstanding worker reservations survive.
 * Model and tree may close first. Destruction/move assignment on another thread terminates.
 * @tparam T Domain row value type; snapshot-owned reachable data must remain immutable and support destruction
 * on the final owner's thread. Library range/frame/request operations do not copy T values.
 */
template <class T> class ModelConsumer {
public:
  /** @brief Empty consumer. */
  ModelConsumer() = default;

  /** @brief Logically retire without waiting for workers. */
  ~ModelConsumer() {
    retire();
  }

  /** @brief Transfer on UI.
   * @param other UI-owned consumer to empty and transfer; a live consumer requires its owner thread.
   */
  ModelConsumer(ModelConsumer &&other) noexcept : state_(std::move(other.state_)), id_(std::exchange(other.id_, {})) {
    if (state_ && state_->owner != std::this_thread::get_id()) {
      std::terminate();
    }
  }

  /** @brief Transfer on UI; retire old ownership only after the full replacement is complete.
   * @param other Replacement consumer moved into this call; old ownership retires after all fields are replaced.
   * @return This consumer; do not use the reference if reentrant old-storage destruction destroyed this object.
   */
  ModelConsumer &operator=(ModelConsumer other) noexcept {
    if (state_ && state_->owner != std::this_thread::get_id()) {
      std::terminate();
    }

    std::swap(state_, other.state_);
    std::swap(id_, other.id_);

    return *this;
  }

  ModelConsumer(const ModelConsumer &) = delete;

  /** @brief Stable handle for ModelFrame::range.
   * @return Stable session/consumer identity, or zero identity for an empty/moved-from consumer.
   */
  ModelRangeId id() const noexcept {
    return id_;
  }

  /** @brief UI-only request or replace this slot's work; never starts a worker or reads provider data.
   * Caller schedules the returned owning token. A null work owner retains the base snapshot.
   * Oversized/out-of-bounds ranges are INVALID_ARGUMENT; live work budget is WOULD_BLOCK.
   * All rejected requests leave this consumer unchanged. Mutations during frame are rejected.
   * @param range Bounded interval in the current base snapshot's logical row count.
   * @param work_owner Optional immutable provider work/storage owner; null retains the base snapshot.
   * @return Worker token after committing pending readiness and layout invalidation; closed, tree/frame errors,
   * INVALID_ARGUMENT, WOULD_BLOCK at the work limit, IDENTITY_EXHAUSTED, or NO_MEMORY without commit.
   */
  std::expected<ModelRequest<T>, TreeError> request(RowRange range, std::shared_ptr<const void> work_owner = {}) {
    auto state = state_;

    if (!state || id_.value == 0) {
      return std::unexpected(TreeError::CLOSED);
    }

    auto allowed = state->allowed();

    if (!allowed) {
      return std::unexpected(allowed.error());
    }

    auto current = state->current;
    auto entry = find(*current);

    if (entry == current->entries.end()) {
      return std::unexpected(TreeError::CLOSED);
    }

    if (range.count > state->options.max_range_rows || range.first > current->snapshot->rowCount() ||
        range.count > current->snapshot->rowCount() - range.first) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    if (state->lifetime->outstanding.load() >= state->options.max_in_flight) {
      return std::unexpected(TreeError::WOULD_BLOCK);
    }

    if (entry->generation == std::numeric_limits<std::uint64_t>::max()) {
      return std::unexpected(TreeError::IDENTITY_EXHAUSTED);
    }

    // Prepare a complete replacement table before changing this consumer or cancelling its old work.
    auto next = state->next();

    if (!next) {
      return std::unexpected(next.error());
    }

    try {
      auto lease =
        std::make_shared<detail::ModelLease>(state->lifetime, work_owner ? std::move(work_owner) : current->snapshot);
      auto &updated = *std::ranges::find_if((*next)->entries, [this](auto &value) {
        return value.id == id_;
      });

      ++updated.generation;
      updated.work = lease;
      updated.result = ModelRange<T>(current->snapshot, range, {});

      ModelRequest<T> result;

      result.id_ = id_;
      result.generation_ = updated.generation;
      result.revision_ = current->snapshot->revision();
      result.row_count_ = current->snapshot->rowCount();
      result.range_ = range;
      result.lease_ = std::move(lease);
      result.publisher_ = state->publisher;

      auto committed = state->commit(std::move(*next));

      if (!committed) {
        return std::unexpected(committed.error());
      }

      if (auto old = entry->work.lock()) {
        old->cancelled.store(true);
      }

      return result;
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    }
  }

  /** @brief Retire this consumer on UI. Idempotent; no allocation, cancellation never joins.
   * Retained tables keep their old result; slot reuse removes the retired entry.
   * @return Success after retirement, including repeated close; WRONG_THREAD leaves ownership unchanged.
   */
  std::expected<void, TreeError> close() {
    if (state_ && state_->owner != std::this_thread::get_id()) {
      return std::unexpected(TreeError::WRONG_THREAD);
    }

    retire();

    return {};
  }

private:
  template <class> friend class Model;

  ModelConsumer(std::shared_ptr<detail::ModelState<T>> state, ModelRangeId id) : state_(std::move(state)), id_(id) {
  }

  auto find(const detail::ModelFrameData<T> &data) const {
    return std::ranges::find_if(data.entries, [this](const auto &value) {
      return value.id == id_;
    });
  }

  void retire() noexcept {
    auto state = std::move(state_);
    auto id = std::exchange(id_, {});

    if (!state) {
      return;
    }

    if (state->owner != std::this_thread::get_id()) {
      std::terminate();
    }

    // Retire identity before a temporary strong lease can release reentrant user storage.
    state->retired.push_back(id);

    auto entry = std::ranges::find_if(state->current->entries, [id](const auto &value) {
      return value.id == id;
    });

    if (entry != state->current->entries.end()) {
      if (auto work = entry->work.lock()) {
        work->cancelled.store(true);
      }
    }
  }

  std::shared_ptr<detail::ModelState<T>> state_;
  ModelRangeId id_;
};

/** @brief UI-owned source connecting immutable providers and scoped ranges to a real WidgetTree.
 * The tree must outlive the source; use a long-lived context when views are disposable.
 * Publisher and request tokens may outlive source/tree/scheduler. Read one pin per model
 * per frame. Scheduler dispatch and source mutation belong between frames.
 * @tparam T Domain row value type; snapshot-owned reachable data must remain immutable and support destruction
 * on the final owner's thread. Library range/frame/request operations do not copy T values.
 */
template <class T> class Model {
public:
  /** @brief Create two bounded Scheduler endpoints in the context's scope; initial snapshot is required.
   * @param tree Borrowed UI-owned tree and scheduler that must outlive the source.
   * @param context Live node whose scope owns the endpoints and whose layout is invalidated on publication.
   * @param initial Non-null immutable provider snapshot retained as the initial base revision.
   * @param options Positive independent row, consumer and in-flight-work limits.
   * @return New source; INVALID_ARGUMENT for null/invalid limits, tree/scheduler errors, IDENTITY_EXHAUSTED,
   * or NO_MEMORY on failure without a source.
   */
  static std::expected<std::unique_ptr<Model>, TreeError> create(WidgetTree &tree, Node<Widget> context,
                                                                 std::shared_ptr<const ModelSnapshot<T>> initial,
                                                                 ModelOptions options = {}) {
    if (!initial || options.max_range_rows == 0 || options.max_consumers == 0 || options.max_in_flight == 0) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    auto allowed = tree.withScope(context, [](const UiScope &) {
    });

    if (!allowed) {
      return std::unexpected(allowed.error());
    }

    auto session = detail::modelSession();

    if (!session) {
      return std::unexpected(session.error());
    }

    try {
      auto state = std::make_shared<detail::ModelState<T>>(tree, context);

      state->session = *session;
      state->options = options;
      state->retired.reserve(options.max_consumers);
      state->current = std::make_shared<const detail::ModelFrameData<T>>(std::move(initial));

      auto completed = tree.template connect<detail::ModelCompletion<T>>(
        context, [weak = std::weak_ptr(state)](Widget &, std::shared_ptr<const detail::ModelCompletion<T>> value) {
          auto state = weak.lock();

          if (!state || !state->lifetime->alive.load() || value->lease->cancelled.load()) {
            return;
          }

          auto previous = state->current;
          auto entry = std::ranges::find_if(previous->entries, [&](const auto &item) {
            return item.id == value->id;
          });

          // Identity, generation and provider revision are independent stale-result barriers:
          // closing a consumer, replacing its request, or publishing new data invalidates different work.
          if (entry == previous->entries.end() || entry->generation != value->generation ||
              value->result.dataRevision() != previous->snapshot->revision() || retired(*state, value->id)) {
            return;
          }

          auto next = state->next();

          if (!next) {
            state->application_error = next.error();

            return;
          }

          (*next)->entries[static_cast<std::size_t>(entry - previous->entries.begin())].result = value->result;

          auto result = state->commit(std::move(*next));

          if (!result) {
            state->application_error = result.error();
          }
        });

      if (!completed) {
        return std::unexpected(completed.error());
      }

      state->publisher = completed->publisher;

      auto snapshots = tree.template connect<ModelSnapshot<T>>(
        context, [weak = std::weak_ptr(state)](Widget &, std::shared_ptr<const ModelSnapshot<T>> value) {
          auto state = weak.lock();

          if (!state || !state->lifetime->alive.load() || value->revision() <= state->current->snapshot->revision()) {
            return;
          }

          auto previous = state->current;
          auto next = state->next();

          if (!next) {
            state->application_error = next.error();

            return;
          }

          (*next)->snapshot = std::move(value);

          // A new provider revision invalidates every old range as one table publication.
          // Cancel old leases only after commit succeeds, preserving old work on rejection.
          for (auto &entry : (*next)->entries) {
            entry.result = ModelRange<T>((*next)->snapshot, {}, {});
            entry.work.reset();
          }

          auto result = state->commit(std::move(*next));

          if (!result) {
            state->application_error = result.error();

            return;
          }

          for (const auto &entry : previous->entries) {
            if (auto work = entry.work.lock()) {
              work->cancelled.store(true);
            }
          }
        });

      if (!snapshots) {
        return std::unexpected(snapshots.error());
      }

      return std::unique_ptr<Model>(new Model(std::move(state), std::move(*completed), std::move(*snapshots)));
    } catch (const std::length_error &) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    }
  }

  /** @brief Close publication delivery on UI, without waiting for worker ownership. */
  ~Model() {
    if (state_->owner != std::this_thread::get_id()) {
      std::terminate();
    }

    state_->lifetime->alive.store(false);
  }

  Model(const Model &) = delete;
  Model &operator=(const Model &) = delete;

  /** @brief Copy worker-safe full snapshot endpoint on UI.
   * @return Copied endpoint for strictly newer immutable provider snapshots; acceptance does not imply application.
   */
  Publisher<ModelSnapshot<T>> publisher() const {
    return snapshots_.publisher;
  }

  /** @brief Capture one immutable frame owner on UI; never reads provider ranges or copies rows.
   * @return Immutable base snapshot/consumer table owner, or WRONG_THREAD. Existing pins survive later publications.
   */
  std::expected<ModelFrame<T>, TreeError> pin() const {
    if (state_->owner != std::this_thread::get_id()) {
      return std::unexpected(TreeError::WRONG_THREAD);
    }

    return ModelFrame<T>(state_->current, state_->options.max_range_rows);
  }

  /** @brief Register one independent consumer; NODE_LIMIT reports its finite capacity.
   * @return Independent UI-owned consumer; tree/frame errors, closed, NODE_LIMIT, IDENTITY_EXHAUSTED, or NO_MEMORY.
   */
  std::expected<ModelConsumer<T>, TreeError> openRange() {
    auto state = state_;
    auto allowed = state->allowed();

    if (!allowed) {
      return std::unexpected(allowed.error());
    }

    if (state->current->entries.size() - state->retired.size() >= state->options.max_consumers) {
      return std::unexpected(TreeError::NODE_LIMIT);
    }

    if (state->identity == std::numeric_limits<std::uint64_t>::max()) {
      return std::unexpected(TreeError::IDENTITY_EXHAUSTED);
    }

    auto previous = state->current;
    auto next = state->next();

    if (!next) {
      return std::unexpected(next.error());
    }

    try {
      std::erase_if((*next)->entries, [&](const auto &entry) {
        return retired(*state, entry.id);
      });

      ModelRangeId id{state->session, state->identity + 1};

      (*next)->entries.push_back({id, 0, ModelRange<T>((*next)->snapshot, {}, {}), {}});

      auto committed = state->commit(std::move(*next));

      if (!committed) {
        return std::unexpected(committed.error());
      }

      state->identity = id.value;
      state->retired.clear();

      return ModelConsumer<T>(std::move(state), id);
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    }
  }

  /** @brief Last UI application failure; queue acceptance alone is not proof of application.
   * @return Sticky UI application error, an empty optional before any such failure, or WRONG_THREAD.
   */
  std::expected<std::optional<TreeError>, TreeError> lastError() const {
    if (state_->owner != std::this_thread::get_id()) {
      return std::unexpected(TreeError::WRONG_THREAD);
    }

    return state_->application_error;
  }

private:
  static bool retired(const detail::ModelState<T> &state, ModelRangeId id) {
    return std::ranges::contains(state.retired, id);
  }

  Model(std::shared_ptr<detail::ModelState<T>> state, SchedulerConnection<detail::ModelCompletion<T>> completion,
        SchedulerConnection<ModelSnapshot<T>> snapshots)
      : state_(std::move(state)), completion_(std::move(completion)), snapshots_(std::move(snapshots)) {
  }

  std::shared_ptr<detail::ModelState<T>> state_;
  SchedulerConnection<detail::ModelCompletion<T>> completion_;
  SchedulerConnection<ModelSnapshot<T>> snapshots_;
};
} // namespace portals
