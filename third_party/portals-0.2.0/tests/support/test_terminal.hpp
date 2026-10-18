// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include "vt_oracle.hpp"
#include <portals/diff.hpp>
#include <portals/input.hpp>
#include <span>
#include <variant>

namespace test_support {
/** @brief Fixed resource envelope for this private, versioned test format. */
struct TraceLimits {
  static constexpr std::size_t CELLS = 4096;       ///< Maximum cells in one replay viewport.
  static constexpr std::size_t BYTES = 256 * 1024; ///< Maximum encoded file and pending output.
  static constexpr std::size_t RECORDS = 4096;     ///< Maximum operations per trace.
  static constexpr std::size_t EVENTS = 65536;     ///< Maximum retained semantic events.
  static constexpr std::size_t SNAPSHOTS = 64;     ///< Maximum retained checkpoints.
};

/** @brief Explicit monotonic clock; no sleeping, scheduling, callbacks or OS clock. */
class FakeClock {
public:
  /** @brief Largest millisecond time representable by TerminalDeadline. */
  static constexpr portals::InputTime MAX_TIME = static_cast<portals::InputTime>(
    std::chrono::duration_cast<std::chrono::milliseconds>(portals::TerminalDeadline::duration::max()).count());
  portals::InputTime now() const noexcept;             ///< Milliseconds since the fake epoch.
  portals::TerminalDeadline deadline() const noexcept; ///< Same instant for FrameOutput.
  void advanceTo(portals::InputTime time);             ///< Reject backward or unrepresentable time.
private:
  portals::InputTime now_{};
};

/** @brief Owning operations; tick explicitly drains input deadlines, snapshot captures output. */
enum class TraceKind {
  INPUT,
  RESIZE,
  OUTPUT,
  FAIL_AFTER,
  TICK,
  FINISH,
  SNAPSHOT
};

/** @brief One operation at an absolute fake millisecond; unused payload fields must be empty. */
struct TraceRecord {
  portals::InputTime time{};                            ///< Equal times retain file order.
  TraceKind kind{};                                     ///< Operation discriminator.
  std::string bytes;                                    ///< Raw input/output bytes only.
  portals::CellSize size{};                             ///< Resize dimensions only.
  std::size_t count{};                                  ///< Accepted prefix before injected failure only.
  bool operator==(const TraceRecord &) const = default; ///< Value equality.
};

using Trace = std::vector<TraceRecord>; ///< Owning decoded records.
/** @brief Strict version-1 text parser; throws runtime_error with line diagnostics. */
Trace parseTrace(std::string_view text);
/** @brief Canonical version-1 recording; applies the same resource/field contract as parsing. */
std::string recordTrace(std::span<const TraceRecord> trace);

/** @brief Private headless transport. One executor owns calls; no implicit flush or callbacks.
 * Clock must outlive this noncopyable adapter. Input results and snapshots own their data.
 * Invalid test operations throw runtime_error; allocation failures propagate. After output
 * failure, discard the terminal/writer pair. No production API or native terminal is involved.
 */
class TestTerminal final : public portals::FrameOutput {
public:
  explicit TestTerminal(FakeClock &clock, portals::CellSize size = {1, 1},
                        std::size_t output_chunk = TraceLimits::BYTES); ///< Borrow clock.
  TestTerminal(const TestTerminal &) = delete;                          ///< Exclusive state cannot be copied.
  TestTerminal &operator=(const TestTerminal &) = delete;               ///< Exclusive state cannot be assigned.
  void resize(portals::CellSize size);                            ///< Preserve overlap; clear clipped wide glyphs.
  std::vector<portals::InputEvent> input(std::string_view bytes); ///< Drain this fragment at now().
  std::vector<portals::InputEvent> tick();                        ///< Explicitly deliver expired input timers.
  std::vector<portals::InputEvent> finish();                      ///< Explicit EOF, independent of trace end.
  VtOracle::Snapshot snapshot() const;                            ///< Owning cells, spans, style and cursor.
  void failAfter(std::size_t accepted);                           ///< Next flush fails after this many accepted bytes.
  /** @brief Own bounded bytes without output; reject pending/failed transport. */
  std::expected<void, portals::TerminalError> submit(std::string bytes) override;
  /** @brief Apply pending bytes unless stopped/expired; injected errors prohibit retries. */
  portals::TerminalWrite flush(portals::TerminalDeadline deadline, std::stop_token stop = {}) override;

private:
  FakeClock &clock_;
  VtOracle oracle_;
  portals::InputDecoder decoder_;
  std::string pending_;
  std::optional<std::size_t> fail_after_;
  bool failed_{};
  std::size_t output_chunk_;
};

/** @brief Semantic delivery order, including resize between partial input sequences. */
struct ReplayEvent {
  portals::InputTime time{};                                  ///< Delivery time in fake milliseconds.
  std::variant<portals::InputEvent, portals::CellSize> value; ///< Owning input or resize.
  bool operator==(const ReplayEvent &) const = default;       ///< Value equality.
};

/** @brief Output outcome independent of feed partitioning. */
struct ReplayWrite {
  portals::TerminalStatus status{};                     ///< Completion/failure reason.
  std::size_t completed{};                              ///< Accepted bytes.
  std::size_t pending{};                                ///< Unsent bytes.
  bool error{};                                         ///< Whether an injected transport failure occurred.
  bool operator==(const ReplayWrite &) const = default; ///< Value equality.
};

/** @brief Bounded owning replay evidence; implicit final snapshot plus explicit checkpoints. */
struct ReplayResult {
  std::vector<ReplayEvent> events;                       ///< Ordered input and resize deliveries.
  std::vector<VtOracle::Snapshot> snapshots;             ///< Full output state, not text rows.
  std::vector<ReplayWrite> writes;                       ///< One result per output record.
  bool operator==(const ReplayResult &) const = default; ///< Value equality.
};

/** @brief Replay without sleeps; positive chunks split byte records at the same timestamp. */
ReplayResult replay(std::span<const TraceRecord> trace, std::size_t input_chunk, std::size_t output_chunk);
} // namespace test_support
