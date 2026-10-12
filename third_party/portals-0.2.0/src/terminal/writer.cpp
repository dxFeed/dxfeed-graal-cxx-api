// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/diff.hpp>
#include <utility>

namespace portals {
TerminalFrameOutput::TerminalFrameOutput(TerminalSession &session) noexcept : session_(session) {
}

std::expected<void, TerminalError> TerminalFrameOutput::submit(std::string bytes) {
  return session_.submit(std::move(bytes));
}

TerminalWrite TerminalFrameOutput::flush(TerminalDeadline deadline, std::stop_token stop) {
  return session_.flush(deadline, stop);
}

FrameWriter::FrameWriter(FrameOutput &output, FrameWriterLimits limits,
                         std::shared_ptr<Diagnostics> diagnostics) noexcept
    : diagnostics_(std::move(diagnostics)), output_(output), limits_(limits) {
}

void FrameWriter::trace(DiagnosticEvent event, DiagnosticReason reason) noexcept {
  if (diagnostics_ && diagnostics_->enabled()) {
    diagnostics_->record(
      {.context = diagnostic_submission_ ? diagnostic_ : DiagnosticContext{}, .event = event, .reason = reason});
  }
}

std::expected<void, FrameError> FrameWriter::submit(CellBuffer &&frame, DiagnosticContext context) {
  const auto rejected = [&](FrameError error) -> std::expected<void, FrameError> {
    if (diagnostics_ && diagnostics_->enabled()) {
      diagnostics_->record(
        {.context = context, .event = DiagnosticEvent::OUTPUT, .reason = DiagnosticReason::OUTPUT_FAILED});
    }

    return std::unexpected(error);
  };

  if (poisoned_) {
    return rejected(FrameError::POISONED);
  }

  if (pending_) {
    return rejected(FrameError::PENDING);
  }

  if (frame.memory().total_bytes > limits_.max_frame_storage_bytes) {
    return rejected(FrameError::FRAME_LIMIT);
  }

  diagnostic_submission_ = diagnostics_ && diagnostics_->enabled();

  if (diagnostic_submission_) {
    diagnostic_ = context;

    if (!diagnostic_.frameId) {
      diagnostic_.frameId = diagnostics_->nextIdentity();
    }
  }

  // Admission while disabled cannot reuse the preceding frame's causal token if
  // recording resumes before flush. Its later output is explicitly incomplete.
  // Encode before consuming the caller's frame. Rejection must preserve both its
  // storage and the last committed belief, and submit must never touch the transport.
  auto encoded = encodeDiff(committedFrame(), frame, limits_.max_output_bytes, limits_.colorMode);

  if (!encoded) {
    trace(DiagnosticEvent::OUTPUT, DiagnosticReason::OUTPUT_FAILED);

    return std::unexpected(encoded.error() == DiffError::OUTPUT_LIMIT ? FrameError::OUTPUT_LIMIT
                                                                      : FrameError::ALLOCATION_FAILURE);
  }

  trace(DiagnosticEvent::FRAME_SUBMITTED, DiagnosticReason::NONE);

  if (encoded->empty()) {
    trace(DiagnosticEvent::OUTPUT, DiagnosticReason::DIFF_EMPTY);

    committed_.emplace(std::move(frame));

    return {};
  }

  bytes_ = std::move(*encoded);
  remaining_ = bytes_.size();
  pending_.emplace(std::move(frame));

  return {};
}

void FrameWriter::poison() noexcept {
  trace(DiagnosticEvent::OUTPUT, DiagnosticReason::OUTPUT_FAILED);

  poisoned_ = true;
  committed_.reset();
  pending_.reset();
  bytes_.clear();
  remaining_ = 0;
}

TerminalWrite FrameWriter::flush(TerminalDeadline deadline, std::stop_token stop) {
  const auto failure = [&](TerminalError error) {
    poison();

    return TerminalWrite{TerminalStatus::FAILED, 0, 0, error};
  };

  if (poisoned_) {
    return failure({TerminalErrorKind::CLOSED});
  }

  if (!pending_) {
    return {};
  }

  try {
    // Submit once, then retry only the transport-owned suffix after a known stop/deadline.
    // Re-submitting the original batch would duplicate bytes already accepted.
    if (!submitted_) {
      auto accepted = output_.submit(std::move(bytes_));

      if (!accepted) {
        return failure(accepted.error());
      }

      submitted_ = true;
    }

    auto result = output_.flush(deadline, stop);

    if (result.status == TerminalStatus::FAILED || result.error) {
      result.status = TerminalStatus::FAILED;

      if (!result.error) {
        result.error = TerminalError{TerminalErrorKind::SYSTEM};
      }

      poison();

      return result;
    }

    // Transport accounting is a trust boundary: inconsistent progress is just as
    // unsafe as an explicit error. Never infer a displayed frame from accepted bytes.
    if (result.completed > remaining_ || result.pending != remaining_ - result.completed ||
        result.status == TerminalStatus::END_OF_INPUT ||
        (result.status == TerminalStatus::PROGRESS && result.pending != 0)) {
      return failure({TerminalErrorKind::SYSTEM});
    }

    remaining_ = result.pending;

    // Only complete, consistent output promotes the desired frame to our historical
    // terminal belief. Partial writes retain the previous committed frame.
    if (result.status == TerminalStatus::PROGRESS && remaining_ == 0) {
      committed_ = std::move(pending_);
      pending_.reset();
      submitted_ = false;
      trace(DiagnosticEvent::OUTPUT, DiagnosticReason::OUTPUT_ACCEPTED);
    } else {
      trace(DiagnosticEvent::OUTPUT, DiagnosticReason::OUTPUT_PENDING);
    }

    return result;
  } catch (...) {
    poison();
    throw;
  }
}

const CellBuffer *FrameWriter::committedFrame() const noexcept {
  return committed_ ? &*committed_ : nullptr;
}

bool FrameWriter::pending() const noexcept {
  return pending_.has_value();
}

bool FrameWriter::poisoned() const noexcept {
  return poisoned_;
}
} // namespace portals
