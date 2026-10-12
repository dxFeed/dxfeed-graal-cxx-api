// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/application.hpp>

#include <algorithm>
#include <thread>
#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#endif

namespace portals {
namespace {
struct Failure {
  ApplicationCause cause;
};

template <class T, class E> T require(std::expected<T, E> value) {
  if (!value) {
    throw Failure{value.error()};
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*value);
  }
}

void captureFailure(ApplicationPhase phase, std::optional<ApplicationError> &error) {
  if (error) {
    return;
  }

  try {
    throw;
  } catch (const Failure &failure) {
    error.emplace(ApplicationError{phase, failure.cause, {}});
  } catch (const std::bad_alloc &) {
    error.emplace(ApplicationError{phase, ApplicationFailure::NO_MEMORY, {}});
  } catch (...) {
    error.emplace(ApplicationError{phase, ApplicationFailure::CALLBACK_FAILED, {}});
  }
}

TerminalDeadline deadlineAt(SchedulerTime time) {
  return TerminalDeadline(std::chrono::milliseconds(time));
}

class SurfacePaint final : public PaintContext {
public:
  SurfacePaint(NodeGeometry geometry, std::vector<RenderCommand> &commands, std::size_t limit)
      : geometry_(geometry), commands_(commands), limit_(limit) {
  }

  std::expected<void, TreeError> submit(std::span<const RenderCommand> commands) override {
    if (commands.size() > limit_ - commands_.size()) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    for (const auto &command : commands) {
      auto result = std::visit(
        [&](const auto &value) -> std::expected<void, TreeError> {
          auto clip = value.clip.translated(geometry_.surface.origin());

          if (!clip) {
            return std::unexpected(TreeError::INVALID_GEOMETRY);
          }

          if constexpr (std::same_as<std::decay_t<decltype(value)>, TextCommand>) {
            auto origin = translated(value.origin, geometry_.surface.origin());

            if (!origin) {
              return std::unexpected(TreeError::INVALID_GEOMETRY);
            }

            commands_.emplace_back(
              TextCommand{*origin, value.text, clip->intersection(geometry_.clip), value.style, value.z});
          } else {
            auto area = value.area.translated(geometry_.surface.origin());

            if (!area) {
              return std::unexpected(TreeError::INVALID_GEOMETRY);
            }

            commands_.emplace_back(FillCommand{*area, clip->intersection(geometry_.clip), value.style, value.z});
          }

          return {};
        },
        command);

      if (!result) {
        return result;
      }
    }

    return {};
  }

private:
  NodeGeometry geometry_;
  std::vector<RenderCommand> &commands_;
  std::size_t limit_;
};

void paintTree(WidgetTree &tree, Node<Widget> node, std::vector<RenderCommand> &commands, std::size_t limit) {
  auto snapshot = require(tree.inspect(node));

  if (!snapshot.visible || !snapshot.geometry.valid) {
    return;
  }

  SurfacePaint sink(snapshot.geometry, commands, limit);

  require(tree.paint(node, sink));

  for (auto child : snapshot.children) {
    paintTree(tree, child, commands, limit);
  }
}

DiagnosticInput inputCategory(const InputEvent &event) {
  return std::visit(
    [](const auto &value) {
      using T = std::decay_t<decltype(value)>;

      if constexpr (std::same_as<T, KeyEvent>) {
        return DiagnosticInput::KEY;
      } else if constexpr (std::same_as<T, TextInputEvent>) {
        return DiagnosticInput::TEXT;
      } else if constexpr (std::same_as<T, PasteInputEvent>) {
        return DiagnosticInput::PASTE;
      } else if constexpr (std::same_as<T, MouseInputEvent>) {
        return DiagnosticInput::MOUSE;
      }

      else {
        return DiagnosticInput::OTHER;
      }
    },
    event);
}
} // namespace

LayoutSnapshot::LayoutSnapshot(std::uint64_t session, std::shared_ptr<const std::vector<Record>> records)
    : session_(session), records_(std::move(records)) {
}

std::expected<LayoutSnapshot, TreeError> LayoutSnapshot::capture(WidgetTree &tree, Node<Widget> node) {
  try {
    auto ancestor = node;
    bool ancestorsVisible = true;

    // A subtree capture must also respect hidden ancestors outside that subtree.
    while (ancestor != Node<Widget>{}) {
      auto state = tree.inspect(ancestor);

      if (!state) {
        return std::unexpected(state.error());
      }

      ancestorsVisible = ancestorsVisible && state->visible;
      ancestor = state->parent;
    }

    // Sharing immutable storage keeps snapshot moves allocation-free, including
    // MSVC Debug where moving a vector may allocate an iterator proxy in noexcept.
    auto records = std::make_shared<std::vector<Record>>(0);
    auto collect = [&](auto &self, Node<Widget> current, bool parentVisible) -> std::expected<void, TreeError> {
      auto state = tree.inspect(current);

      if (!state) {
        return std::unexpected(state.error());
      }

      const bool visible = parentVisible && state->visible;

      if (visible && !state->geometry.valid) {
        return std::unexpected(TreeError::INVALID_GEOMETRY);
      }

      records->push_back({current.id(), visible ? state->geometry.local.size() : CellSize{}});

      // Hidden descendants still need identity records so all views can prepare
      // against this snapshot after an earlier view invalidates the live tree.
      for (const auto child : state->children) {
        auto result = self(self, child, visible);

        if (!result) {
          return result;
        }
      }

      return {};
    };
    auto captured = collect(collect, node, ancestorsVisible);

    if (!captured) {
      return std::unexpected(captured.error());
    }

    std::ranges::sort(*records, {}, [](const Record &record) {
      return record.id.slot;
    });

    return LayoutSnapshot(node.id().session, std::move(records));
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<CellSize, TreeError> LayoutSnapshot::viewport(Node<Widget> node) const noexcept {
  const auto id = node.id();

  if (!id.session || !records_) {
    return std::unexpected(TreeError::STALE_NODE);
  }

  if (id.session != session_) {
    return std::unexpected(TreeError::WRONG_SESSION);
  }

  const auto found = std::ranges::lower_bound(*records_, id.slot, {}, [](const Record &record) {
    return record.id.slot;
  });

  if (found == records_->end() || found->id != id) {
    return std::unexpected(TreeError::STALE_NODE);
  }

  return found->size;
}

struct Application::State {
  struct Run;
  ApplicationOptions options;
  SchedulerClock clock;
  std::thread::id owner = std::this_thread::get_id();
  std::unique_ptr<Scheduler> scheduler;
  std::unique_ptr<WidgetTree> tree;
  std::unique_ptr<EventRouter> router;
  Node<Widget> root;
  std::stop_source stop;
  bool ran{};

  State(ApplicationOptions policy, SchedulerClock time) : options(std::move(policy)), clock(time) {
  }

  void observe(ApplicationPhase phase, std::chrono::nanoseconds elapsed) {
    if (!options.observer.callback) {
      return;
    }

    try {
      options.observer.callback(options.observer.context, phase, elapsed);
    } catch (...) {
      throw Failure{ApplicationFailure::OBSERVER_FAILED};
    }
  }

  template <class Callback> void measure(ApplicationPhase phase, Callback &&callback) {
    if (!options.observer.callback) {
      callback();

      return;
    }

    const auto begin = std::chrono::steady_clock::now();

    callback();

    observe(phase, std::chrono::steady_clock::now() - begin);
  }
};

struct Application::State::Run {
  using Control = std::variant<InputEvent, CellSize>;
  Application &app;
  State &state;
  ApplicationScene &scene;
  ApplicationTerminal &terminal;
  ApplicationStats &stats;
  ApplicationPhase &phase;
  std::optional<ApplicationError> &error;
  CellSize viewport;
  InputDecoder decoder;
  UiScope scope;
  SchedulerConnection<Control> channel;
  FrameWriter writer;
  std::optional<TerminalRead> batch;
  std::shared_ptr<const Control> pending;
  DiagnosticContext pendingContext;
  std::size_t byteOffset{}, eventOffset{};
  SchedulerTime arrival{}, nextFrame{};
  bool ending{}, finished{}, drain{};
  const std::uint32_t frameInterval;

  Run(Application &application, State &owner, ApplicationScene &content, ApplicationTerminal &transport,
      ApplicationStats &totals, ApplicationPhase &currentPhase, std::optional<ApplicationError> &failure)
      : app(application), state(owner), scene(content), terminal(transport), stats(totals), phase(currentPhase),
        error(failure), viewport(require(terminal.size())), decoder(require(InputDecoder::create(state.options.input))),
        scope(require(app.scheduler().makeScope())),
        writer(terminal, state.options.writer, state.options.scheduler.diagnostics),
        frameInterval((1000u + state.options.framesPerSecond - 1) / state.options.framesPerSecond) {
    require(app.tree().invalidate(app.root(), Invalidation::LAYOUT));
    channel = require(app.scheduler().connect<Control>(
      scope,
      [this](auto control) {
        try {
          if (state.options.observer.callback) {
            state.observe(ApplicationPhase::CONTROL_LATENCY,
                          std::chrono::milliseconds(app.scheduler().metrics().control.last_latency));
          }

          if (const auto *size = std::get_if<CellSize>(control.get())) {
            viewport = *size;
            require(app.tree().invalidate(app.root(), Invalidation::LAYOUT));

            return;
          }

          const auto &event = std::get<InputEvent>(*control);

          ++stats.inputs;

          if (!require(scene.input(app, event))) {
            require(app.router().dispatch(event));
          }
        } catch (...) {
          phase = ApplicationPhase::INPUT;
          captureFailure(phase, error);
          app.requestStop();
        }
      },
      SchedulerLane::CONTROL));
  }

  void readInput() {
    if (!batch && !pending && !drain && !ending) {
      arrival = app.now();

      auto deadline = deadlineAt(arrival) + state.options.inputSlice;

      if (const auto decodeDeadline = decoder.deadline()) {
        if (*decodeDeadline <= arrival) {
          drain = true;

          return;
        }

        deadline = std::min(deadline, deadlineAt(*decodeDeadline));
      }

      batch.emplace(terminal.read(deadline, state.stop.get_token()));
      arrival = app.now();
      byteOffset = eventOffset = 0;

      if (batch->error) {
        throw Failure{*batch->error};
      }

      if (batch->status == TerminalStatus::STOPPED) {
        app.requestStop();

        return;
      }

      if (batch->status == TerminalStatus::FAILED) {
        throw Failure{TerminalError{TerminalErrorKind::SYSTEM}};
      }

      ending = batch->status == TerminalStatus::END_OF_INPUT;
    }
  }

  void admitInput() {
    // Keep exactly one native batch/raw suffix and one rejected CONTROL owner. A full
    // queue never consumes the retained event; dispatch below releases queue capacity.

    for (std::size_t work = 0; work < state.options.inputBudget; ++work) {
      if (pending) {
        DiagnosticScope diagnostic(app.tree().diagnostics(), pendingContext);

        auto published = channel.publisher.publish(pending);

        if (!published && published.error() == SchedulerError::WOULD_BLOCK) {
          break;
        }

        require(std::move(published));
        pending.reset();
      }

      std::optional<Control> control;

      if (batch && batch->resized) {
        control.emplace(*batch->resized);
        batch->resized.reset();
      } else if (batch && eventOffset < batch->events.size()) {
        control.emplace(batch->events[eventOffset++]);
      } else {
        if (finished) {
          break;
        }

        const auto bytes = batch ? std::string_view(batch->bytes).substr(byteOffset) : std::string_view{};
        const auto stamp = bytes.empty() ? app.now() : arrival;
        auto step = require(ending && bytes.empty() ? decoder.finish(stamp) : decoder.next(bytes, stamp));

        byteOffset += step.consumed;
        drain = step.status == InputStatus::WORK_LIMIT || step.event.has_value();
        finished = step.status == InputStatus::FINISHED;

        if (step.status == InputStatus::FAILED) {
          throw Failure{InputError::FAILED};
        }

        if (step.event) {
          control.emplace(std::move(*step.event));
        }

        if (batch && byteOffset == batch->bytes.size() && eventOffset == batch->events.size()) {
          batch.reset();
        }

        if (!control && !drain) {
          break;
        }
      }

      if (control) {
        pendingContext = {};

        if (auto *diagnostics = app.tree().diagnostics()) {
          const auto *event = std::get_if<InputEvent>(&*control);
          pendingContext = diagnostics->beginInput(event ? inputCategory(*event) : DiagnosticInput::OTHER);
        }

        pending = std::make_shared<const Control>(std::move(*control));
      }
    }
  }

  bool dispatch() {
    phase = ApplicationPhase::DISPATCH;
    const auto dispatched = require(app.scheduler().dispatch());

    if (dispatched.callback_failures) {
      throw Failure{SchedulerError::CALLBACK_FAILED};
    }

    return !dispatched.stopped && !state.stop.stop_requested();
  }

  CellBuffer buildFrame() {
    // Commands borrow widget text. Consume them into owning cells before releasing
    // the tree frame; keep original component errors across its callback boundary.
    std::optional<CellBuffer> frame;
    std::optional<Failure> failure;

    auto framed = app.tree().frame([&] {
      try {
        const auto rect = require(CellRect::from({}, viewport).transform_error([](auto) {
          return TreeError::INVALID_GEOMETRY;
        }));
        std::vector<RenderCommand> commands(std::initializer_list<RenderCommand>{});

        require(layoutTree(app.tree(), app.root(), rect, rect));
        paintTree(app.tree(), app.root(), commands, state.options.render.max_commands);
        frame.emplace(require(renderFullFrame(viewport, commands, state.options.widthPolicy, state.options.render)));
      } catch (const Failure &value) {
        failure.emplace(value);
      }
    });

    if (failure) {
      throw *failure;
    }

    require(std::move(framed));

    return std::move(*frame);
  }

  LayoutSnapshot captureLayout() {
    std::optional<LayoutSnapshot> snapshot;
    std::optional<Failure> failure;
    auto framed = app.tree().frame([&] {
      try {
        const auto rect = require(CellRect::from({}, viewport).transform_error([](auto) {
          return TreeError::INVALID_GEOMETRY;
        }));

        require(layoutTree(app.tree(), app.root(), rect, rect));
        snapshot.emplace(require(LayoutSnapshot::capture(app.tree(), app.root())));
      } catch (const Failure &value) {
        failure.emplace(value);
      }
    });

    if (failure) {
      throw *failure;
    }

    require(std::move(framed));

    return std::move(*snapshot);
  }

  Invalidation consumePreparationRequest(DiagnosticContext &frameContext) {
    const auto dirty = require(app.tree().consumeFrameRequest());
    const auto requestedContext = app.tree().diagnosticFrame();

    // Layout's in-frame PAINT has no new causal token. Keep the last real request
    // so an otherwise unchanged preparation still links input to submitted output.
    if (requestedContext.frameId) {
      frameContext = requestedContext;
    }

    return dirty;
  }

  void prepareViews(DiagnosticContext &frameContext) {
    for (std::size_t pass = 0; pass < state.options.maxPreparationPasses; ++pass) {
      const auto snapshot = captureLayout();

      // Arrangement requests PAINT. Only fresh LAYOUT work from view preparation
      // repeats the pass; all hooks share the pin established by prepare once.
      consumePreparationRequest(frameContext);
      require(scene.prepareViews(app, snapshot));

      if (consumePreparationRequest(frameContext) != Invalidation::LAYOUT) {
        return;
      }
    }

    throw Failure{TreeError::WOULD_BLOCK};
  }

  void renderReady() {
    if (writer.pending() || app.now() < nextFrame) {
      return;
    }

    const auto dirty = require(app.tree().consumeFrameRequest());

    if (dirty == Invalidation::NONE) {
      return;
    }

    auto frameContext = app.tree().diagnosticFrame();
    DiagnosticScope preparing(app.tree().diagnostics(), frameContext);

    phase = ApplicationPhase::PREPARE;
    state.measure(phase, [&] {
      require(scene.prepare(app, viewport));
      consumePreparationRequest(frameContext);

      if (scene.usesPreparedLayout()) {
        prepareViews(frameContext);
      }
    });

    std::optional<CellBuffer> frame;

    phase = ApplicationPhase::RENDER;
    state.measure(phase, [&] {
      frame.emplace(buildFrame());
    });

    // arrange requests paint inside layout, but this frame has already painted it.
    // Consume that work without losing the actual submission's causal token.
    require(app.tree().consumeFrameRequest());

    phase = ApplicationPhase::SUBMIT;
    state.measure(phase, [&] {
      require(writer.submit(std::move(*frame), frameContext));
    });
    ++stats.frames;
    nextFrame = app.now() + frameInterval;
  }

  void flushOutput() {
    if (writer.pending()) {
      phase = ApplicationPhase::FLUSH;
      state.measure(phase, [&] {
        const auto written = writer.flush(deadlineAt(app.now()) + state.options.outputSlice, state.stop.get_token());

        ++stats.flushes;
        stats.bytes += written.completed;

        if (written.error) {
          throw Failure{*written.error};
        }

        if (written.status == TerminalStatus::STOPPED) {
          app.requestStop();
        }

        if (written.status == TerminalStatus::FAILED) {
          throw Failure{TerminalError{TerminalErrorKind::SYSTEM}};
        }

        if (!writer.pending()) {
          nextFrame = app.now() + frameInterval;
        }
      });
    }
  }

  void execute() {
    while (!state.stop.stop_requested()) {
      ++stats.iterations;
      phase = ApplicationPhase::INPUT;
      readInput();

      if (state.stop.stop_requested()) {
        break;
      }

      admitInput();

      if (!dispatch()) {
        break;
      }

      renderReady();
      flushOutput();

      if (finished && !pending && !batch && app.scheduler().metrics().control.pending == 0 && !writer.pending()) {
        break;
      }
    }
  }
};

std::expected<void, TreeError> ApplicationScene::prepare(Application &, CellSize) {
  return {};
}

bool ApplicationScene::usesPreparedLayout() const noexcept {
  return false;
}

std::expected<void, TreeError> ApplicationScene::prepareViews(Application &, const LayoutSnapshot &) {
  return {};
}

std::expected<bool, TreeError> ApplicationScene::input(Application &, const InputEvent &) {
  return false;
}

void ApplicationScene::stop(Application &) {
}

Application::Application(std::unique_ptr<State> state) : state_(std::move(state)) {
}

Application::~Application() = default;

Scheduler &Application::scheduler() noexcept {
  return *state_->scheduler;
}

WidgetTree &Application::tree() noexcept {
  return *state_->tree;
}

EventRouter &Application::router() noexcept {
  return *state_->router;
}

Node<Widget> Application::root() const noexcept {
  return state_->root;
}

WidthPolicy Application::widthPolicy() const noexcept {
  return state_->options.widthPolicy;
}

SchedulerTime Application::now() const noexcept {
  if (state_->clock.read) {
    return state_->clock.read(state_->clock.context);
  }

  return static_cast<SchedulerTime>(
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

void Application::requestStop() noexcept {
  state_->stop.request_stop();
  state_->scheduler->requestStop();
}

std::expected<std::unique_ptr<Application>, ApplicationError> Application::create(ApplicationOptions options,
                                                                                  SchedulerClock clock) {
  if (!options.framesPerSecond || options.framesPerSecond > 1000 || !options.inputBudget ||
      !options.maxPreparationPasses || options.inputSlice.count() <= 0 || options.outputSlice.count() <= 0 ||
      (options.nativeScreen != NativeScreenMode::MAIN_SCREEN &&
       options.nativeScreen != NativeScreenMode::ALTERNATE_SCREEN)) {
    return std::unexpected(ApplicationError{ApplicationPhase::CREATE, ApplicationFailure::INVALID_OPTIONS, {}});
  }

  try {
    auto state = std::make_unique<State>(std::move(options), clock);

    state->scheduler = require(Scheduler::create(state->options.scheduler, clock));
    state->tree = require(WidgetTree::create(*state->scheduler));
    state->root = require(state->tree->root());
    state->router = require(EventRouter::create(*state->tree));
    require(InputDecoder::create(state->options.input));

    return std::unique_ptr<Application>(new Application(std::move(state)));
  } catch (const Failure &error) {
    return std::unexpected(ApplicationError{ApplicationPhase::CREATE, error.cause, {}});
  } catch (const std::bad_alloc &) {
    return std::unexpected(ApplicationError{ApplicationPhase::CREATE, ApplicationFailure::NO_MEMORY, {}});
  }
}

std::expected<ApplicationStats, ApplicationError> Application::run(ApplicationScene &scene, std::stop_token stop) {
  if (state_->owner != std::this_thread::get_id()) {
    return std::unexpected(ApplicationError{ApplicationPhase::OPEN, TreeError::WRONG_THREAD, {}});
  }

  if (state_->clock.read) {
    return std::unexpected(ApplicationError{ApplicationPhase::OPEN, ApplicationFailure::INVALID_OPTIONS, {}});
  }

  if (state_->ran) {
    return std::unexpected(ApplicationError{ApplicationPhase::OPEN, ApplicationFailure::ALREADY_RUN, {}});
  }

  try {
#ifdef _WIN32
    NativeTerminalHandles handles{reinterpret_cast<std::intptr_t>(GetStdHandle(STD_INPUT_HANDLE)),
                                  reinterpret_cast<std::intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE))};
#else
    NativeTerminalHandles handles{0, 1};
#endif
    auto opened = openNativeApplicationTerminal(handles, state_->options.terminal, state_->options.outputSlice,
                                                state_->options.nativeMouseReporting, state_->options.nativeScreen);

    if (!opened) {
      state_->ran = true;

      return std::unexpected(ApplicationError{ApplicationPhase::OPEN, opened.error(), {}});
    }

    return run(scene, **opened, stop);
  } catch (const std::bad_alloc &) {
    state_->ran = true;

    return std::unexpected(ApplicationError{ApplicationPhase::OPEN, ApplicationFailure::NO_MEMORY, {}});
  }
}

std::expected<ApplicationStats, ApplicationError>
Application::run(ApplicationScene &scene, ApplicationTerminal &terminal, std::stop_token externalStop) {
  if (state_->owner != std::this_thread::get_id()) {
    return std::unexpected(ApplicationError{ApplicationPhase::OPEN, TreeError::WRONG_THREAD, {}});
  }

  if (state_->ran) {
    return std::unexpected(ApplicationError{ApplicationPhase::OPEN, ApplicationFailure::ALREADY_RUN, {}});
  }

  state_->ran = true;

  ApplicationStats stats;
  ApplicationPhase phase = ApplicationPhase::OPEN;
  std::optional<ApplicationError> error;
  auto capture = [&] {
    captureFailure(phase, error);
  };
  std::stop_callback cancellation(externalStop, [&] {
    requestStop();
  });

  try {
    if (!state_->stop.stop_requested()) {
      State::Run run(*this, *state_, scene, terminal, stats, phase, error);
      run.execute();
    }
  } catch (...) {
    capture();
  }

  phase = ApplicationPhase::STOP;

  try {
    state_->measure(phase, [&] {
      requestStop();
      require(scheduler().stop());
      scene.stop(*this);
    });
  } catch (...) {
    capture();
  }

  phase = ApplicationPhase::CLOSE;
  std::optional<TerminalError> restoration;

  try {
    state_->measure(phase, [&] {
      auto closed = terminal.close();

      if (!closed) {
        restoration = closed.error();
      }
    });
  } catch (...) {
    capture();
  }

  if (restoration) {
    if (!error) {
      error.emplace(ApplicationError{phase, *restoration, {}});
    }

    error->restoration = restoration;
  }

  if (error) {
    return std::unexpected(*error);
  }

  return stats;
}
} // namespace portals
