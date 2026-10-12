// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <portals/geometry.hpp>
#include <portals/input.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

namespace portals {

/** @brief Absolute monotonic time used by terminal waits. */
using TerminalDeadline = std::chrono::steady_clock::time_point;

/** @brief Borrowed native descriptors supplied to open(); no Windows headers required. */
struct NativeTerminalHandles {
  std::intptr_t input{};  ///< POSIX fd or borrowed Windows HANDLE converted to intptr_t.
  std::intptr_t output{}; ///< POSIX fd or borrowed Windows HANDLE converted to intptr_t.
};

/** @brief Session resource bounds and cancellation/resize sampling cadence. */
struct TerminalOptions {
  std::size_t max_input_bytes{4096};          ///< Positive maximum POSIX raw bytes per read; unused on Windows.
  std::size_t max_input_events{256};          ///< Positive Windows event and native record work bound per read.
  std::size_t max_output_bytes{1024 * 1024};  ///< Positive maximum size of one owned output batch.
  std::chrono::milliseconds wait_cadence{10}; ///< Positive maximum individual poll interval.
  InputOptions windows_input{};               ///< Windows VT decoder limits/timeouts; unused on POSIX.
};

/** @brief Terminal failure classification; native_code provides platform detail. */
enum class TerminalErrorKind {
  INVALID_OPTIONS,  ///< An option or handle is outside its supported range.
  UNSUPPORTED,      ///< Handles are not terminals, or no backend is implemented.
  SYSTEM,           ///< A native system operation failed.
  CLOSED,           ///< Session was closed or moved from.
  OUTPUT_PENDING,   ///< The previous output batch is not complete.
  OUTPUT_LIMIT,     ///< The new batch exceeds max_output_bytes.
  INPUT_DECODE,     ///< Windows input decoder failed; close and reopen the session.
  OUTPUT_UNCERTAIN, ///< Windows output was cancelled/failed; close before further submission.
  INPUT_LIMIT       ///< Retained managed startup input exceeds max_input_bytes.
};

/** @brief Owning error value, independent of session lifetime. */
struct TerminalError {
  TerminalErrorKind kind{}; ///< Stable failure category.
  int native_code{};        ///< errno on POSIX, GetLastError on Windows; zero for contract errors.
  int restoration_code{};   ///< First rollback native error on failed open; zero on success.
};

/** @brief Why an input or output operation returned. */
enum class TerminalStatus {
  PROGRESS,     ///< Input/resize available, or the entire pending output batch completed.
  DEADLINE,     ///< Absolute deadline reached; inspect error for Windows output uncertainty.
  STOPPED,      ///< Stop requested; inspect error for Windows output uncertainty.
  END_OF_INPUT, ///< Input ended after previously readable bytes were delivered.
  FAILED        ///< Inspect the accompanying error; no hidden output retry occurs.
};

/** @brief Bounded owning transport result; earlier results are never invalidated. */
struct TerminalRead {
  TerminalStatus status{TerminalStatus::PROGRESS}; ///< Completion reason.
  std::string bytes;                               ///< POSIX raw bytes; decode with InputDecoder.
  std::vector<InputEvent> events;                  ///< Native semantic events; empty on POSIX.
  std::optional<CellSize> resized;                 ///< Newly observed dimensions, if changed.
  std::optional<TerminalError> error;              ///< Present exactly when status is error.
};

/** @brief Owning output progress for this flush call, not a rendered-frame commit. */
struct TerminalWrite {
  TerminalStatus status{TerminalStatus::PROGRESS}; ///< Completion reason.
  std::size_t completed{};                         ///< Bytes successfully written in this call only.
  std::size_t pending{};              ///< Unconfirmed bytes still owned; Windows cancellation forbids replay.
  std::optional<TerminalError> error; ///< Native error, or OUTPUT_UNCERTAIN with Windows deadline/stopped.
};

namespace detail {
class NativeTerminal;
}

/**
 * @brief Move-only terminal mode and I/O owner; no I/O belongs in paint/measure.
 * @details One executor owns all calls including destruction; concurrent calls are
 * unsupported. Transfer between executors requires external synchronization. POSIX
 * and Windows open duplicate both native handles and own the duplicates; caller handles stay
 * open. All aliases of their terminals/open-file descriptions must be exclusively
 * used by this owner until close, because termios and file status flags are shared.
 * Windows console modes and output code page are console-wide, including all aliases.
 * Input/output values own storage. Allocation exceptions propagate with RAII cleanup;
 * no caller buffer/view is retained. This backend installs no signal handlers.
 * No guarantee is made for abrupt process death or externally closed/modified handles.
 */
class TerminalSession {
public:
  /** @brief Save both original states before mutation and enter raw nonblocking mode.
   * Both POSIX descriptors must be TTYs; both Windows handles must be console handles.
   * Windows enables VT input/output and UTF-8 output, preserving the input code page.
   * On failure, independently restore everything
   * already changed; restoration_code reports rollback failure. Allocation can throw.
   * @param handles Borrowed native input/output handles to duplicate; both must identify supported terminals.
   * @param options Positive storage/work caps and polling cadence, copied into the session.
   * @return Exclusive session ownership, or an error including any failed rollback in restoration_code.
   * @throws std::bad_alloc If session or bounded working storage cannot be allocated.
   */
  [[nodiscard]] static std::expected<TerminalSession, TerminalError> open(NativeTerminalHandles handles,
                                                                          TerminalOptions options = {});
  TerminalSession(const TerminalSession &) = delete;            ///< Exclusive state cannot be copied.
  TerminalSession &operator=(const TerminalSession &) = delete; ///< Exclusive state cannot be copied.
  /** @brief Transfer ownership; source becomes closed.
   * @param other Session whose owned descriptors and mode-restoration responsibility are transferred.
   */
  TerminalSession(TerminalSession &&other) noexcept;
  /** @brief Best-effort close old state, then transfer.
   * @param other Session to consume; assignment first releases this session's previous native state.
   * @return This session; a distinct source becomes closed. Restoration errors during assignment cannot be reported.
   */
  TerminalSession &operator=(TerminalSession &&other) noexcept;
  ~TerminalSession() noexcept; ///< Best-effort restoration and descriptor cleanup; never flushes.

  /** @brief Read one bounded owning batch, changed size, deadline, stop, EOF or error.
   * Stop takes precedence over an expired deadline. Resize is sampled on the owner,
   * at most wait_cadence apart while waiting; changes can coalesce. No callbacks.
   * Windows returns events only, using windows_input for VT/paste decoding. An empty
   * progress batch yields after the native record work budget. Paste is emitted at
   * its end delimiter; interleaved native events may precede that completed event.
   * @param deadline Absolute steady-clock deadline bounding this read operation.
   * @param stop Cooperative cancellation request, checked before the deadline.
   * @return An owning input/resize batch, or deadline, stopped, eof or an explicit terminal error.
   * @throws std::bad_alloc If owning batch storage cannot be allocated.
   */
  [[nodiscard]] TerminalRead read(TerminalDeadline deadline, std::stop_token stop = {});
  /** @brief Query current columns/rows; returns closed/system error without mutation.
   * @return Current viewport columns/rows, or closed, unsupported or system failure as applicable.
   */
  [[nodiscard]] std::expected<CellSize, TerminalError> size() const noexcept;
  /** @brief Transfer one bounded owned byte batch; reject while output is pending.
   * No I/O. Empty batch is allowed. Rejection leaves session state unchanged.
   * @param bytes Complete byte batch, moved into session storage only on acceptance.
   * @return Success, or closed/OUTPUT_PENDING/OUTPUT_LIMIT/OUTPUT_UNCERTAIN without replacing pending output.
   */
  [[nodiscard]] std::expected<void, TerminalError> submit(std::string bytes);
  /** @brief Write until batch completion, deadline, stop or error; retain unconfirmed bytes.
   * Only successful native writes count as completed. Deadline/stop are checked
   * between nonblocking syscalls on POSIX, preserving its retryable suffix.
   * Windows uses one owning worker. Cancelling an issued write returns deadline or
   * stopped with error=OUTPUT_UNCERTAIN; bytes remain owned, but cannot be retried.
   * Further submit/flush fail until close. A pre-expired/pre-stopped call that has
   * issued no write preserves retryability and has no uncertainty error.
   * Successful writes do not prove terminal interpretation or rendered-frame state.
   * @param deadline Absolute steady-clock deadline for completing the current byte batch.
   * @param stop Cooperative cancellation; cancellation of issued Windows I/O can make progress uncertain.
   * @return Bytes completed in this call, remaining owned bytes, completion status and optional error.
   */
  [[nodiscard]] TerminalWrite flush(TerminalDeadline deadline, std::stop_token stop = {});
  /** @brief Number of pending owned bytes; zero for a closed/moved-from session.
   * @return Bytes whose successful transmission has not been confirmed; uncertain bytes must not be replayed.
   */
  [[nodiscard]] std::size_t pendingBytes() const noexcept;
  /** @brief Discard pending bytes and restore all changed modes/flags independently.
   * Always closes owned duplicate descriptors, even on restoration error. Returns
   * first native restoration/close error. Idempotent; destructor cannot report errors.
   * Windows first requests cancellation until the worker actually completes, joins,
   * then restores modes/code page. Console-host progress is required: restoration
   * can wait for a paused host to resume output drain. No hard deadline is promised
   * for close/destruction or arbitrary stalled drivers/hosts; nothing is detached.
   * Incomplete/failed output leaves renderer belief uncertain, owned by the caller.
   * @return Success, including repeated close, or the first native restoration/cleanup error.
   */
  [[nodiscard]] std::expected<void, TerminalError> close() noexcept;

private:
  friend class detail::NativeTerminal;
  /** @brief Snapshot Windows cursor visibility for the native application; POSIX saves it through VT.
   * @return Success or original snapshot error; the ordinary raw-session close contract remains unchanged.
   */
  [[nodiscard]] std::expected<void, TerminalError> saveCursorVisibility() noexcept;

  /** @brief Settle native output before the application starts its final cleanup budget.
   * Pending bytes remain owned by the session and are never replayed by this operation.
   * Windows also synchronizes with the console host after an uncertain write: joining
   * the cancelled client worker alone does not settle host processing of accepted bytes.
   * Host/driver progress is required; this boundary has no hard deadline.
   * @return Success, including an already closed session, or a native worker/host synchronization error.
   */
  [[nodiscard]] std::expected<void, TerminalError> joinOutputWriter() noexcept;

  /** @brief Application-only final batch after discarding pending/uncertain output, then always restore OS modes.
   * Windows first cancels and joins the previous writer. No old frame suffix is replayed. Host/driver progress
   * is still required for join/restoration; a hard close deadline is not promised.
   * @param bytes Preallocated bounded restoration batch; its VT policy belongs to the native application adapter.
   * @param deadline Absolute steady-clock deadline for the final batch; joining a worker does not extend it.
   * @return Success, including repeated close, or output error retaining a subsequent native cleanup error.
   */
  [[nodiscard]] std::expected<void, TerminalError> closeWithOutput(std::string bytes,
                                                                   TerminalDeadline deadline) noexcept;
  struct Impl;
  explicit TerminalSession(std::unique_ptr<Impl> impl) noexcept;
  std::unique_ptr<Impl> impl_;
};

} // namespace portals
