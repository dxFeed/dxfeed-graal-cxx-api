// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <memory>
#include <portals/diagnostics.hpp>

#include <portals/cell_buffer.hpp>
#include <portals/terminal.hpp>

#include <expected>
#include <optional>
#include <string>

namespace portals {
/** @brief Color encoding selected explicitly by the terminal owner. */
enum class ColorMode {
  TRUE_COLOR, ///< RGB SGR 38;2/48;2, preserving the existing default.
  ANSI16      ///< Nearest standard RGB palette entry encoded as SGR 30-37/90-97 and 40-47/100-107.
};

/** @brief Bounded encoding failure; input buffers are unchanged. */
enum class DiffError {
  OUTPUT_LIMIT,      ///< The complete transaction cannot fit the configured byte cap.
  ALLOCATION_FAILURE ///< Output allocation failed.
};
/** @brief Encode changed complete rows into one owning VT transaction.
 * @param previous Borrowed last completed frame, or null for a complete repaint.
 * @param next Borrowed desired frame; no view is retained or invalidated.
 * @param max_bytes Maximum encoded bytes, including control sequences.
 * @param colorMode Fixed output palette. Use
 * the same mode for previous and next; changing mode requires previous=nullptr.
 * ANSI16 uses the conventional RGB
 * palette; a terminal may customize its physical palette.
 * @return Owning bytes, empty for semantically unchanged frames, or explicit failure.
 * @details Any thread with stable inputs; no I/O. IDs are never compared across
 * buffers. Size/policy changes force a clear and complete repaint. The surface
 * must match the terminal viewport. The owner supplies a UTF-8 VT terminal in
 * ground parser state, with no concurrent writers and matching width policy.
 * Transactions establish replacement mode, full scrolling region, absolute
 * origin, and disabled autowrap; finish at home with default SGR and wrap on.
 * The owner is responsible for restoring these modes on exit. This is not a
 * general parser recovery sequence. Full output starts with CAN, which aborts
 * incomplete CSI/UTF-8 on supported terminals after the old transport is quiescent.
 * Recovery covers only prefixes emitted by this encoder, never arbitrary OSC/DCS.
 */
[[nodiscard]] std::expected<std::string, DiffError> encodeDiff(const CellBuffer *previous, const CellBuffer &next,
                                                               std::size_t max_bytes = 1024 * 1024,
                                                               ColorMode colorMode = ColorMode::TRUE_COLOR);

/** @brief Exclusive transport boundary, borrowed by FrameWriter for its lifetime.
 * Implementations own submitted bytes, perform no I/O in submit, and preserve the
 * exact unsent suffix on deadline/stop without an error. An accompanying error
 * marks uncertain progress even on deadline/stop. All errors may have unknown effects.
 * One executor calls all methods; no callbacks or concurrent calls are supported.
 * A successful flush means bytes accepted, not hardware/display acknowledgement.
 */
class FrameOutput {
public:
  virtual ~FrameOutput() = default; ///< Destroy the adapter, without an implicit flush.
  /** @brief Take an owning batch, or reject without output; allocation may throw.
   * @param bytes Complete encoded transaction, transferred to the transport on success.
   * @return Success when the batch is retained, or a transport rejection before output.
   */
  [[nodiscard]] virtual std::expected<void, TerminalError> submit(std::string bytes) = 0;
  /** @brief Flush the owned suffix; report bytes for this call and total pending.
   * @param deadline Absolute monotonic deadline for this call.
   * @param stop Cooperative cancellation request; implementations preserve retryable suffixes when effects are known.
   * @return This call's completed byte count, remaining suffix, status and any uncertain-output error.
   */
  [[nodiscard]] virtual TerminalWrite flush(TerminalDeadline deadline, std::stop_token stop) = 0;
};

/** @brief Non-owning adapter; session must outlive adapter and writer.
 * The writer exclusively controls session output until it is destroyed. Session
 * read/size remain available to the same executor. Close is always owner-managed.
 */
class TerminalFrameOutput final : public FrameOutput {
public:
  /** @brief Borrow a stable session.
   * @param session Open session that outlives this adapter and its FrameWriter.
   */
  explicit TerminalFrameOutput(TerminalSession &session) noexcept;
  /** @brief Forward ownership.
   * @param bytes Encoded bytes transferred to TerminalSession::submit.
   * @return The session's submission result; no terminal I/O is performed.
   */
  [[nodiscard]] std::expected<void, TerminalError> submit(std::string bytes) override;
  /** @brief Forward I/O.
   * @param deadline Absolute deadline forwarded to TerminalSession::flush.
   * @param stop Cancellation request forwarded unchanged.
   * @return Exact session progress and error information for this call.
   */
  [[nodiscard]] TerminalWrite flush(TerminalDeadline deadline, std::stop_token stop) override;

private:
  TerminalSession &session_;
};

/** @brief Owning writer resource caps. At most two capped frames coexist. */
struct FrameWriterLimits {
  std::size_t max_output_bytes{1024 * 1024};             ///< Complete VT transaction cap.
  std::size_t max_frame_storage_bytes{64 * 1024 * 1024}; ///< Per-frame CellBuffer::memory().total_bytes cap.
  ColorMode colorMode{ColorMode::TRUE_COLOR};            ///< Fixed output palette for this writer's complete lifetime.
};
/** @brief Submission failure; the supplied frame remains owned by the caller. */
enum class FrameError {
  PENDING,           ///< Complete or abandon the owning session before submitting another frame.
  POISONED,          ///< Unknown output state; close/reestablish terminal before creating a new writer.
  FRAME_LIMIT,       ///< Frame allocation accounting exceeds the configured cap.
  OUTPUT_LIMIT,      ///< Encoded transaction exceeds the configured cap.
  ALLOCATION_FAILURE ///< Encoding allocation failed.
};

/** @brief Bounded owner of last committed and pending frames; never performs I/O in submit.
 * One executor owns every call and destruction; external synchronization is needed
 * for transfer. The output reference must outlive this non-movable writer. No
 * callbacks, background work or implicit destructor flush. Destroying with pending
 * output requires closing that output session before any other writer is used.
 * A deadline/stop without error retains the exact transaction. Any transport error or inconsistent
 * progress permanently poisons this writer and invalidates its committed frame.
 * Recovery requires closing/joining the old transport, then a new writer whose
 * first full transaction sends CAN and the mode baseline. This requires terminal
 * support for CAN abort of this encoder's CSI/UTF-8 prefixes; arbitrary external
 * output requires an owner-established fresh parser instead. Reopening alone does
 * not reset the parser. Never restart output while the old native worker can write.
 */
class FrameWriter {
public:
  /** @brief Borrow exclusive output.
   * @param output Transport that outlives the writer and receives no competing submissions.
   * @param limits Per-transaction output and per-frame storage caps; submit reports any excess.
   * @param diagnostics Optional shared metadata recorder; records no terminal bytes.
   */
  explicit FrameWriter(FrameOutput &output, FrameWriterLimits limits = {},
                       std::shared_ptr<Diagnostics> diagnostics = {}) noexcept;
  FrameWriter(const FrameWriter &) = delete;            ///< Exclusive output cannot be shared by copying.
  FrameWriter &operator=(const FrameWriter &) = delete; ///< Exclusive output cannot be reassigned.
  /** @brief Encode and take frame only on success; reject busy/poisoned/limits without I/O.
   * Semantically unchanged frames commit immediately without output. Accepted frames
   * invalidate caller views through move; rejection retains them. Allocation errors
   * are returned. Submission invalidates any previously borrowed committed-frame view.
   * @param frame Desired owning frame, consumed only after successful validation and encoding.
   * @param context Token from WidgetTree::diagnosticFrame, linking the actual output to invalidation.
   * @return Success when accepted, or pending, poisoned, FRAME_LIMIT, OUTPUT_LIMIT or ALLOCATION_FAILURE.
   */
  [[nodiscard]] std::expected<void, FrameError> submit(CellBuffer &&frame, DiagnosticContext context = {});
  /** @brief Perform transport I/O; commit only after complete, consistent progress.
   * Must run outside paint/measure/layout. Stop/deadline without errors retain the frame.
   * An accompanying error normalizes the result status to error and preserves its details.
   * Transport exceptions poison state then propagate; no automatic retry occurs.
   * @param deadline Absolute monotonic deadline passed to the transport.
   * @param stop Cooperative stop request for this call.
   * @return Completed and pending bytes with the final status; errors mean the committed belief is invalid.
   * @note Exceptions from the output adapter propagate only after the writer is poisoned.
   */
  [[nodiscard]] TerminalWrite flush(TerminalDeadline deadline, std::stop_token stop = {});
  /** @brief Borrow last fully transmitted frame; null initially/after error.
   * This is historical during partial output, not the current physical screen.
   * Expires on successful submit, completing/error flush, or writer destruction.
   * @return Borrowed last completed frame, or null when no valid committed belief exists.
   */
  [[nodiscard]] const CellBuffer *committedFrame() const noexcept;
  /** @brief Whether a transaction still requires completion; no allocation/errors.
   * @return True while a desired frame still has an incomplete transaction.
   */
  [[nodiscard]] bool pending() const noexcept;
  /** @brief Whether unknown side effects permanently prohibit further output.
   * @return True after an uncertain or inconsistent transport result; this state cannot be reset.
   */
  [[nodiscard]] bool poisoned() const noexcept;

private:
  void poison() noexcept;
  void trace(DiagnosticEvent event, DiagnosticReason reason) noexcept;
  std::shared_ptr<Diagnostics> diagnostics_;
  DiagnosticContext diagnostic_{};
  bool diagnostic_submission_{};
  FrameOutput &output_;
  FrameWriterLimits limits_;
  std::optional<CellBuffer> committed_;
  std::optional<CellBuffer> pending_;
  std::string bytes_;
  std::size_t remaining_{};
  bool submitted_{};
  bool poisoned_{};
};
} // namespace portals
