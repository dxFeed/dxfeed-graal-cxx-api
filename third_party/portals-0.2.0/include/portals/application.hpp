// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/action.hpp>
#include <portals/diff.hpp>
#include <portals/layout.hpp>
#include <portals/layout_snapshot.hpp>
#include <portals/render.hpp>

namespace portals {

/** @brief Synchronous runner phase, also used to identify a failure boundary. */
enum class ApplicationPhase {
  CREATE,         ///< Validate options and construct the application and its core services.
  OPEN,           ///< Enter a run and initialize its transport and runner state.
  INPUT,          ///< Read, decode and enqueue terminal input or resize events.
  DISPATCH,       ///< Deliver queued work through the existing scheduler.
  PREPARE,        ///< Synchronize scene state and, when enabled, prepare views after preliminary layout.
  RENDER,         ///< Lay out and paint the final frame, then consume commands into an owning cell buffer.
  SUBMIT,         ///< Give the owning frame to FrameWriter for diff encoding; no transport write occurs here.
  FLUSH,          ///< Write pending encoded output within the configured deadline.
  STOP,           ///< Request cancellation, stop the scheduler and invoke the scene's stop hook.
  CLOSE,          ///< Close the transport and attempt restoration of terminal state.
  CONTROL_LATENCY ///< Observer-only logical queue delay for the runner's input/resize channel, not a work phase.
};

/** @brief Failures introduced by the runner itself. */
enum class ApplicationFailure {
  INVALID_OPTIONS, ///< Runner options are invalid, or a native run was requested with an injected clock.
  ALREADY_RUN,     ///< This application has already entered its single permitted run.
  NO_MEMORY,       ///< Allocation failed at a runner exception boundary.
  CALLBACK_FAILED, ///< A non-allocation exception escaped into a runner callback boundary.
  OBSERVER_FAILED  ///< The optional measurement observer threw an exception.
};

/** @brief Original component error; no formatted text or borrowed exception is retained. */
using ApplicationCause =
  std::variant<ApplicationFailure, TerminalError, SchedulerError, TreeError, InputError, RenderError, FrameError>;

/** @brief Failure after orderly shutdown, including a separate restoration failure. */
struct ApplicationError {
  ApplicationPhase phase{};                 ///< Operation which first failed.
  ApplicationCause cause{};                 ///< Original failure, or explicit callback/allocation classification.
  std::optional<TerminalError> restoration; ///< Close failure retained even when an earlier operation failed.
};

/** @brief Optional synchronous measurement sink; disabled by default.
 * The borrowed observer must outlive run. It must not perform I/O, mutate/reenter the application,
 * or retain borrowed state. It may collect bounded samples. Exceptions fail run explicitly.
 */
struct ApplicationObserver {
  /** @brief Receive one completed phase duration; called outside tree frames.
   * @param context Borrowed observer state.
   * @param phase Completed measured operation.
   * @param elapsed Wall duration measured with steady_clock, independent of an injected logical clock.
   * CONTROL_LATENCY instead reports the logical queue delay of this runner's input/resize channel only.
   */
  using Callback = void (*)(void *context, ApplicationPhase phase, std::chrono::nanoseconds elapsed);
  Callback callback{}; ///< Null disables measurements and their clock reads.
  void *context{};     ///< Borrowed synchronous callback state.
};

/** @brief Screen ownership for the native application transport. */
enum class NativeScreenMode {
  MAIN_SCREEN,     ///< Paint the caller's screen directly; its contents and cursor are not restored.
  ALTERNATE_SCREEN ///< Own an alternate screen and return to the caller's main screen on close.
};

/** @brief Finite runner and component policies; one owner services all I/O. */
struct ApplicationOptions {
  SchedulerOptions scheduler{}; ///< Bounded existing scheduler queues and optional diagnostics.
  TerminalOptions terminal{};   ///< Native terminal batch and wait limits.
  NativeScreenMode nativeScreen{
    NativeScreenMode::ALTERNATE_SCREEN};       ///< Preserve the caller's main screen in native run.
  bool nativeMouseReporting{true};             ///< Manage POSIX mouse reporting in the native run overload.
  InputOptions input{};                        ///< POSIX streaming decoder limits.
  FrameWriterLimits writer{};                  ///< Output encoding and owned-frame limits.
  RenderLimits render{};                       ///< Paint command and full-frame limits.
  WidthPolicy widthPolicy{};                   ///< Shared text/canvas/frame width policy.
  std::uint32_t framesPerSecond{30};           ///< Positive cap, at most 1000; no catch-up frames.
  std::size_t inputBudget{64};                 ///< Positive maximum decoder/native event pulls per iteration.
  std::chrono::milliseconds inputSlice{10};    ///< Positive future read deadline; never read(now).
  std::chrono::milliseconds outputSlice{2000}; ///< Positive flush budget; Windows timeout can be uncertain.
  ApplicationObserver observer{};              ///< Optional borrowed measurement callback.
  std::size_t maxPreparationPasses{3};         ///< Positive layout/view preparation budget for opt-in scenes per frame.
};

/** @brief Owning totals from a completed run, without samples or terminal data. */
struct ApplicationStats {
  std::uint64_t iterations{}; ///< Bounded service cycles executed.
  std::uint64_t frames{};     ///< Accepted frame submissions, including semantically unchanged frames.
  std::uint64_t flushes{};    ///< Calls which serviced pending output.
  std::uint64_t bytes{};      ///< Bytes confirmed by output flushes.
  std::uint64_t inputs{};     ///< Events delivered through the runner CONTROL channel.
};

/** @brief Custom/replay terminal borrowed for exactly one run; no competing I/O is allowed.
 * The owner retains its storage and must outlive run. Application always calls close after run starts,
 * including exceptions and stop. Values returned by read own their bytes/events and respect the configured
 * terminal batch limits. An injected clock must advance consistently with this terminal's deadlines.
 */
class ApplicationTerminal : public FrameOutput {
public:
  /** @param deadline Future absolute deadline in the application's clock.
   * @param stop Cooperative cancellation request.
   * @return One bounded owning input batch, timeout, stop, EOF, or original error.
   */
  virtual TerminalRead read(TerminalDeadline deadline, std::stop_token stop) = 0;
  /** @return Current nonnegative viewport, or an original terminal error. */
  virtual std::expected<CellSize, TerminalError> size() const = 0;
  /** @brief Restore owned native screen/modes and close; custom transports define their policy. Idempotent, never
   * throws.
   * @return Success or original restoration/cleanup error. No hard native close deadline is promised.
   */
  virtual std::expected<void, TerminalError> close() noexcept = 0;
};

/**
 * @brief Open an exclusive native transport with managed screen and optional mouse reporting.
 *
 * POSIX setup queries coordinate encodings and saves xterm tracking modes before enabling SGR button reports. Windows
 * uses native mouse records and sends no mouse setup bytes.
 *
 * Successful setup preserves non-query input for the first read; failed setup consumes already read input and returns
 * no replay adapter. Replies are matched by mode/status, not authenticated. The adapter requires the steady-clock epoch
 * and exclusive terminal I/O.
 *
 * Close gives retryable output drain and final managed screen/mouse restoration separate fresh I/O budgets, then closes
 * the OS session. Joining uncertain Windows I/O can exceed the drain budget; the restoration budget starts after that
 * join. These budgets do not bound total close duration. Its noexcept destructor performs best effort.
 *
 * XTSAVE overwrites one-level tracking/visibility caches; confirmed DECSC capture overwrites saved cursor state.
 * Nested screen owners, previously saved caches and competing mode changes are unsupported. Setup/cleanup bypass frame
 * byte statistics.
 *
 * @param handles Borrowed native input/output handles, duplicated by TerminalSession.
 * @param options Positive resource limits, including retained non-query startup input bytes.
 * @param modeTimeout Positive I/O budget for startup, shutdown drain and final restoration, applied separately in
 * milliseconds. It does not bound a native worker join or total shutdown duration.
 * @param mouseReporting Enable managed POSIX mouse modes; false skips only mouse setup I/O.
 * @param screen ALTERNATE_SCREEN preserves the caller's main screen/cursor/rendition; MAIN_SCREEN paints it directly.
 * Alternate screen ownership starts on the main screen and is not nestable. POSIX saved cursor-visibility storage
 * and DEC cursor-save state are borrowed exclusively; their previous saved caches are not preserved. Window resize
 * keeps the new geometry; the host reflows/crops primary content and adjusts the saved cursor. Startup/cleanup require
 * compatible VT1049 support. Cleanup cancels and joins any uncertain Windows write before sending its final restoration
 * batch, then restores OS modes even if that output fails. Broken/stalled hosts and abrupt process death can prevent
 * screen restoration.
 * @return Owning adapter, or setup error with cleanup failure in restoration_code. Unsupported/missing queries, input
 * pressure, and incomplete restoration remain explicit errors.
 * @throws std::bad_alloc If allocation fails; acquired native/screen/mouse state receives best-effort cleanup.
 */
[[nodiscard]] std::expected<std::unique_ptr<ApplicationTerminal>, TerminalError>
openNativeApplicationTerminal(NativeTerminalHandles handles, TerminalOptions options = {},
                              std::chrono::milliseconds modeTimeout = std::chrono::milliseconds{2000},
                              bool mouseReporting = true, NativeScreenMode screen = NativeScreenMode::ALTERNATE_SCREEN);

class Application;

/** @brief Borrowed scene extension; bindings/models/providers belong to the caller.
 * Construct the Application first, then the scene; destroy the scene before Application. The scene and
 * application must survive every callback and run. No hook may destroy either owner or reenter run.
 * Hooks execute on the creating UI thread; thrown exceptions become explicit failures followed by cleanup.
 */
class ApplicationScene {
public:
  virtual ~ApplicationScene() = default; ///< Destroy bindings/providers before their Application owner.
  /// @brief Synchronize controls and pin each model once for a dirty frame.
  /// Called only when a dirty frame can be submitted, outside tree.frame.
  /// Retained view preparations own pins; custom widgets retain any other borrowed
  /// render data until rendering completes. Opt-in scenes retain the same pins
  /// through every prepareViews pass and final rendering.
  /// @param application Borrowed UI services.
  /// @param size Terminal viewport for this frame; legacy scenes prepare views with
  /// their final sizes here, while opt-in scenes obtain sizes in prepareViews.
  /// @return Success or preparation error; no frame is submitted on failure.
  virtual std::expected<void, TreeError> prepare(Application &application, CellSize size);
  /** @brief Optional global command interception inside the bounded CONTROL callback.
   * @param application Borrowed UI services; requestStop is valid here.
   * @param event Owning queued event borrowed for this callback only.
   * @return True if handled, false to route through EventRouter, or a scene error.
   */
  virtual std::expected<bool, TreeError> input(Application &application, const InputEvent &event);
  /** @brief Stop and join only scene-owned producers; called once before terminal restoration.
   * @param application Still-live UI services; scheduler has already been stopped.
   */
  virtual void stop(Application &application);
  /// @brief Opt into bounded layout-driven view preparation; default is false.
  /// @return True to run preliminary layout and prepareViews before the final frame.
  /// False preserves the original single layout/paint/render path.
  virtual bool usesPreparedLayout() const noexcept;
  /// @brief Prepare views from one owning layout snapshot; default is a no-op.
  /// Called only for opt-in scenes, on UI outside tree.frame, after prepare has
  /// pinned models once. Use those same pins for all views and repeated passes;
  /// do not dispatch the scheduler or replace pins here. Snapshot reads survive
  /// invalidations caused by earlier views. A new LAYOUT request repeats layout
  /// and this hook up to ApplicationOptions::maxPreparationPasses; PAINT does not.
  /// @param application Borrowed UI services, available for checked view mutation.
  /// @param snapshot Immutable owning geometry for this pass, borrowed for the call;
  /// copy it if retention is needed. Hidden subtree viewports are zero.
  /// @return Success or the original view error; failure submits no frame.
  /// Exhausting the pass budget returns WOULD_BLOCK in PREPARE.
  /// Hook exceptions also fail PREPARE.
  virtual std::expected<void, TreeError> prepareViews(Application &application, const LayoutSnapshot &snapshot);
};

/** @brief One UI owner for scheduler, tree, routing and a single terminal run.
 * All calls/destruction except requestStop require the creating thread. Scene owns typed nodes and
 * bindings; release them before this object. Terminal I/O never runs inside layout/paint. No automatic
 * recovery follows uncertain output. Run is single-use, including failed/closed sessions.
 */
class Application {
public:
  /** @param options Positive finite policies, copied into the owner.
   * @param clock Borrowed monotonic millisecond service; null reader uses steady_clock.
   * @return UI owner or explicit options/component/allocation error. Caller argument allocations are outside
   * this boundary; arbitrary noexcept user constructors cannot be made recoverable.
   */
  static std::expected<std::unique_ptr<Application>, ApplicationError> create(ApplicationOptions options = {},
                                                                              SchedulerClock clock = {});
  ~Application(); ///< Release routing/tree before the scheduler; scene must already be destroyed.
  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;
  /** @return Borrowed UI scheduler; publishers may retain its closed channel state. */
  Scheduler &scheduler() noexcept;
  /** @return Borrowed UI tree; observing handles do not extend its lifetime. */
  WidgetTree &tree() noexcept;
  /** @return Borrowed UI event router. */
  EventRouter &router() noexcept;
  /** @return Checked root observer valid until the tree closes. */
  Node<Widget> root() const noexcept;
  /** @return Current monotonic logical milliseconds; injected clock must not move backwards. */
  SchedulerTime now() const noexcept;
  /** @return Shared width policy for text created by the scene. */
  WidthPolicy widthPolicy() const noexcept;
  /** @brief Worker-safe while this owner lives; wakes scheduler and cancels terminal waits. */
  void requestStop() noexcept;
  /** @brief Open standard input/output and restore them before returning.
   * Requires the default steady clock; an injected clock returns INVALID_OPTIONS before opening.
   * EOF finishes decoding, delivers admitted CONTROL and flushes submitted output. It does not
   * wait for future frame deadlines or promise a final repaint of accepted changes.
   * @param scene Borrowed scene which must outlive this call.
   * @param stop Optional caller cancellation signal.
   * @return Owning totals or detailed first failure and separate restoration error.
   */
  std::expected<ApplicationStats, ApplicationError> run(ApplicationScene &scene, std::stop_token stop = {});
  /** @brief Run against a borrowed custom terminal; always close before returning once accepted.
   * EOF has the same cutoff as the native overload: no final repaint or frame-cap exception.
   * Finite replay must leave time before EOF when it needs a final visible frame.
   * @param scene Borrowed scene which must outlive this call.
   * @param terminal Exclusive transport borrowed through cleanup; no ownership transfer.
   * @param stop Optional caller cancellation signal.
   * @return Owning totals or detailed first failure and separate restoration error.
   */
  std::expected<ApplicationStats, ApplicationError> run(ApplicationScene &scene, ApplicationTerminal &terminal,
                                                        std::stop_token stop = {});

private:
  struct State;
  explicit Application(std::unique_ptr<State> state);
  std::unique_ptr<State> state_;
};
} // namespace portals
