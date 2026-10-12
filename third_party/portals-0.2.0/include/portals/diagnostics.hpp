// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace portals {
/** @brief Metadata stages; no stage contains application text or terminal bytes. */
enum class DiagnosticEvent {
  INPUT,               ///< Decoded input observed at the application or router boundary.
  QUEUE_ACCEPTED,      ///< Publication admitted to the existing scheduler.
  QUEUE_REJECTED,      ///< Publication rejected before delivery.
  QUEUE_DISPATCH,      ///< Callback admitted on the UI executor.
  QUEUE_REPLACED,      ///< New publication links to one displaced publication.
  QUEUE_CANCELLED,     ///< Queued publication will not be delivered.
  TARGET,              ///< Actual target selection or eligibility result.
  FOCUS,               ///< Focus selection or change.
  ACTION,              ///< Semantic action resolution or unhandled result.
  HANDLER_STARTED,     ///< User event callback admitted.
  HANDLER_RETURNED,    ///< Callback returned or failed; model effects are separate evidence.
  PROPERTY,            ///< Observed equality or committed property change.
  INVALIDATION,        ///< Tree work decision.
  FRAME_REQUESTED,     ///< A cause linked to a coalesced frame.
  FRAME_NOT_REQUESTED, ///< No new frame was requested in this scope.
  FRAME_SUBMITTED,     ///< Actual frame encoded and accepted by FrameWriter.
  OUTPUT,              ///< Diff or transport result, never proof of physical display.
  COMPLETED,           ///< Synchronous action/dispatch returned.
  QUEUE_RETURNED       ///< Scheduler callback returned or failed; application effects remain separate evidence.
};

/** @brief Enumerated evidence, never an exception message or inferred model value. */
enum class DiagnosticReason {
  NONE,                ///< No additional detail is available.
  QUEUE_FULL,          ///< Configured queue capacity rejected the publication.
  REPLACED,            ///< A later snapshot displaced this publication.
  CANCELLED,           ///< Scope, endpoint or scheduler closed before delivery.
  NO_TARGET,           ///< No destination was selected.
  TARGET_DISABLED,     ///< Explicit target is hidden or disabled.
  STALE_TARGET,        ///< Target identity is no longer live.
  UNBOUND_ACTION,      ///< No eligible handler was registered along the route.
  UNHANDLED,           ///< Registered handlers did not consume the event.
  HANDLER_FAILED,      ///< Handler boundary caught a failure; prior effects remain.
  VALUE_UNCHANGED,     ///< An observed Property comparison was equal.
  VALUE_CHANGED,       ///< An observed Property commit or tree work change occurred.
  OUTCOME_UNKNOWN,     ///< Return alone cannot establish application effects.
  FRAME_NOT_REQUESTED, ///< No new frame was requested in this scope.
  DIFF_EMPTY,          ///< The actual encoded diff was empty.
  OUTPUT_FAILED,       ///< Encoding or output failed; no rollback is implied.
  OUTPUT_ACCEPTED,     ///< Transport accepted the complete consistent transaction.
  OUTPUT_PENDING       ///< Transport retained an unfinished suffix.
};

/** @brief Input category only; scalar, clipboard, modifiers and coordinates are omitted. */
enum class DiagnosticInput {
  NONE,  ///< No additional detail is available.
  KEY,   ///< Key category without scalar or modifiers.
  TEXT,  ///< Text category without content.
  PASTE, ///< Paste category without clipboard or payload.
  MOUSE, ///< Mouse category without coordinates.
  OTHER  ///< Another instrumented input category.
};

/** @brief Recording state, including a terminal identity exhaustion condition. */
enum class DiagnosticState {
  DISABLED,          ///< Recording is stopped; retained evidence remains available.
  ENABLED,           ///< Ring admission is active.
  IDENTITY_EXHAUSTED ///< Recording permanently stopped before identifier reuse.
};

/** @brief Configuration/export failure. */
enum class DiagnosticError {
  INVALID_ARGUMENT,   ///< Invalid capacity or attempt to restart an exhausted recorder.
  NO_MEMORY,          ///< Ring allocation failed without replacing the old interval.
  UNSUPPORTED_VERSION ///< The export does not declare the supported schema.
};

/** @brief Session-local identity; never an object address. */
struct DiagnosticNodeId {
  std::uint64_t session{};    ///< Tree session.
  std::size_t slot{};         ///< Tree registry slot.
  std::uint64_t generation{}; ///< Slot generation.
};

/** @brief Fixed-size causal token; zero fields mean unobserved, never a reused identity. */
struct DiagnosticContext {
  std::uint64_t correlationId{};       ///< Action identity in this recorder's lifetime.
  std::uint64_t parentCorrelationId{}; ///< Causal action, if known.
  std::uint64_t period{};              ///< Recording interval identity.
  std::uint64_t publicationId{};       ///< Queue publication, not subscription identity.
  std::uint64_t frameId{};             ///< Coalesced frame identity, if known.
};

/** @brief Fixed-size bounded record. */
struct DiagnosticRecord {
  std::uint64_t sequence{};             ///< Monotonic retained record number.
  DiagnosticContext context{};          ///< Causal metadata.
  DiagnosticNodeId stableNodeId{};      ///< Full generation-aware node identity.
  std::uint64_t relatedPublicationId{}; ///< Replaced publication, at most one per record.
  DiagnosticEvent event{};              ///< Observed stage.
  DiagnosticReason reason{};            ///< Evidence at this stage.
  DiagnosticInput input{};              ///< Category for INPUT only.
  std::uint32_t semanticAction{};       ///< Action enum ordinal plus one; zero means unavailable.
  bool controlQueue{};                  ///< True for the control lane; false for data/timer.
  bool incomplete{};                    ///< Cross-period or missing-context evidence.
};

/** @brief Allocation budget applied when recording is enabled. */
struct DiagnosticOptions {
  std::size_t capacity{1024}; ///< Positive maximum retained records; oldest records are overwritten.
};

/** @brief Owning bounded snapshot; copy only between frames, serialize after releasing the recorder lock. */
struct DiagnosticSnapshot {
  std::uint32_t schemaVersion{1};    ///< Supported metadata schema; unrelated to PORTALS-TRACE.
  DiagnosticOptions options{};       ///< Active ring capacity.
  DiagnosticState state{};           ///< State at the snapshot boundary.
  std::uint64_t period{};            ///< Current or last recording interval.
  std::uint64_t periodBegin{};       ///< First possible sequence in this interval.
  std::uint64_t periodEnd{};         ///< Last sequence when disabled; zero while recording.
  std::uint64_t droppedOverflow{};   ///< Overwritten records in this interval, saturating.
  std::uint64_t droppedContention{}; ///< Failed nonblocking write attempts in this interval, saturating.
  bool countersSaturated{};          ///< A loss counter reached its representable limit.
  bool incomplete{};                 ///< Conservative: losses, open chains, or recording boundaries may hide work.
  // The count constructor lets Debug iterator-proxy allocation failures propagate.
  std::vector<DiagnosticRecord> records = std::vector<DiagnosticRecord>(0); ///< Oldest-to-newest bounded records.
};
class DiagnosticScope;

/** @brief Explicit, thread-safe metadata recorder. Disabled by default; no clocks or I/O.
 * enable/disable/snapshot are control-plane operations outside measure/layout/paint.
 * Record admission tries the mutex once; it never waits, allocates, or calls user code.
 * The recorder must outlive every DiagnosticScope borrowing it. A scheduler can share ownership.
 */
class Diagnostics {
public:
  /** @brief Construct a disabled recorder without allocating ring storage. */
  Diagnostics() = default;
  Diagnostics(const Diagnostics &) = delete;
  Diagnostics &operator=(const Diagnostics &) = delete;
  /** @brief Start a fresh recording interval; retain monotonic identities across intervals.
   * @param options Positive bounded ring capacity, reserved before recording starts.
   * @return Success, INVALID_ARGUMENT, or NO_MEMORY. Exhausted recorders cannot restart.
   */
  std::expected<void, DiagnosticError> enable(DiagnosticOptions options = {});
  /** @brief Stop admission and preserve retained evidence for export. */
  void disable() noexcept;
  /** @brief Fast recording gate; safe on any thread.
   * @return True only while record admission is enabled.
   */
  bool enabled() const noexcept;
  /** @brief Issue a never-reused identity only while enabled.
   * @return New identity, or zero when disabled/exhausted.
   */
  std::uint64_t nextIdentity() noexcept;
  /** @brief Record the input boundary before publishing to the scheduler.
   * @param input Enumerated input category; no original payload is inspected.
   * @return Token to install with DiagnosticScope and propagate through publication.
   */
  DiagnosticContext beginInput(DiagnosticInput input) noexcept;
  /** @brief Obtain this thread's explicitly installed context for this recorder.
   * @return Current token, or an empty token when disabled/unscoped.
   */
  DiagnosticContext current() const noexcept;
  /** @brief Attempt one bounded append; disabled calls return before touching counters.
   * @param record Fixed-size metadata; sequence is assigned internally.
   */
  void record(DiagnosticRecord record) noexcept;
  /** @brief Record a common synchronous stage using the current context.
   * @param event Stage actually reached.
   * @param reason Known result, or OUTCOME_UNKNOWN for unobserved user effects.
   * @param node Generation-aware target identity; empty when unavailable.
   */
  void note(DiagnosticEvent event, DiagnosticReason reason = DiagnosticReason::NONE,
            DiagnosticNodeId node = {}) noexcept;
  /** @brief Snapshot outside frame hooks; may allocate and throw std::bad_alloc.
   * @return Bounded owning evidence; missing events must never be interpreted as absence of work.
   */
  DiagnosticSnapshot snapshot() const;

private:
  friend class DiagnosticScope;
  void lostContention() noexcept;
  mutable std::mutex mutex_;
  std::atomic<DiagnosticState> state_{DiagnosticState::DISABLED};
  std::atomic<std::uint64_t> identity_{};
  std::atomic<std::uint64_t> period_{};
  std::atomic<std::uint64_t> contention_{};
  std::atomic<bool> saturated_{};
  DiagnosticOptions options_{};
  std::vector<DiagnosticRecord> ring_;
  std::size_t start_{};
  std::size_t count_{};
  std::uint64_t sequence_{};
  std::uint64_t begin_{};
  std::uint64_t end_{};
  std::uint64_t overflow_{};
};

/** @brief Stack-only causal context on one thread; restores the previous scope on every exit.
 * No global recorder is installed: lookups require the same explicitly supplied recorder.
 * Must be destroyed in stack order on the constructing thread, before the recorder.
 */
class DiagnosticScope {
public:
  /** @brief Install a borrowed context without allocation.
   * @param diagnostics Recorder that outlives this scope; null/disabled is a no-op.
   * @param context Token issued by that recorder, or empty for an unobserved continuation.
   */
  DiagnosticScope(Diagnostics *diagnostics, DiagnosticContext context) noexcept;
  /** @brief Restore the enclosing context. */
  ~DiagnosticScope();
  DiagnosticScope(const DiagnosticScope &) = delete;
  DiagnosticScope &operator=(const DiagnosticScope &) = delete;
  /** @brief Summarize only property evidence observed in this scope.
   * @return VALUE_CHANGED, VALUE_UNCHANGED, or OUTCOME_UNKNOWN; no inference from handler return.
   */
  DiagnosticReason outcome() const noexcept;
  /** @brief Inspect actual invalidation evidence in this scope.
   * @return True if a frame request was recorded by an instrumented tree.
   */
  bool frameRequested() const noexcept;

private:
  friend class Diagnostics;
  static thread_local DiagnosticScope *active_;
  Diagnostics *diagnostics_{};
  DiagnosticContext context_{};
  DiagnosticScope *previous_{};
  bool changed_{};
  bool unchanged_{};
  bool frame_requested_{};
};

/** @brief Serialize metadata in memory; performs no stream/file/terminal I/O.
 * @param snapshot Owning bounded snapshot obtained outside frame hooks.
 * @return Versioned human-readable text. Allocation failures propagate to the caller.
 */
std::string exportDiagnostics(const DiagnosticSnapshot &snapshot);
/** @brief Check the version line before interpreting an application's diagnostic export.
 * @param text Export bytes borrowed only for this check; no payload is executed.
 * @return Success for schema 1; UNSUPPORTED_VERSION otherwise.
 */
std::expected<void, DiagnosticError> validateDiagnosticVersion(std::string_view text) noexcept;
} // namespace portals
