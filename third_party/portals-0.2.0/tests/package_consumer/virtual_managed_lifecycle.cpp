// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "virtual_support.hpp"
#include <portals/managed.hpp>

#include <stdexcept>
#include <thread>

namespace {
using namespace virtualConsumer;

bool causedBy(const ManagedError &error, TreeError cause) {
  const auto *value = std::get_if<TreeError>(&error.cause);

  return value && *value == cause;
}

GraphemeText formatPrice(const ModelRow<Ohlc> &row) {
  return text(std::to_string(static_cast<int>(row.value.close)));
}

// The callbacks observe one borrowed frame, including retries caused by opening ranges.
// Only copied receipts survive the invocation; neither callbacks nor tests keep a model pin.
struct PreparedViews {
  const ModelFrame<Ohlc> *current{};
  std::uint64_t revision{};
  std::size_t passes{};
  std::vector<std::uint64_t> pinRevisions;
  std::array<std::vector<ModelRequest<Ohlc>>, 3> requests;

  void record(std::size_t index, const ModelFrame<Ohlc> &frame, const std::optional<ModelRequest<Ohlc>> &request) {
    if (index == 0) {
      current = &frame;
      revision = frame.revision();
      ++passes;
      pinRevisions.push_back(revision);
    } else {
      check(current == &frame && revision == frame.revision(), "Sibling views did not borrow one shared pin");
    }

    check(frame.dataRevision() == DATA_REVISION, "Managed preparation changed the data revision");

    if (request) {
      requests[index].push_back(*request);
    }
  }

  void checkRequests(std::size_t count, bool cancelled) const {
    for (const auto &history : requests) {
      check(history.size() == count && history.back().stopRequested() == cancelled,
            "Managed preparation lost, duplicated or incorrectly cancelled a request");
    }
  }
};

template <class W> void checkStyle(Application &app, Node<W> node, RgbColor foreground) {
  take(app.tree().read(node, [&](const W &view) {
    check(view.computedStyle() && view.computedStyle()->normal.foreground == foreground,
          "Factory-created view missed automatic style registration or theme replacement");
  }));
}

template <class W> void checkRange(Application &app, Node<W> node, RangeState expected, bool hidden = false) {
  take(app.tree().read(node, [&](const W &view) {
    if (hidden) {
      check(view.visibleRange().count == 0, "Hidden managed view retained a visible range");
    } else {
      check(view.rangeState() == expected && view.visibleRange().count > 0,
            "Managed view readiness or automatically prepared viewport is incorrect");
    }
  }));
}

void managedLifecycle() {
  constexpr std::size_t CHECKPOINTS = 9;
  ReplayTerminal terminal(CHECKPOINTS);
  ApplicationOptions options;

  options.scheduler.data_capacity = 1;

  auto app = take(Application::create(options, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto root = scene->root();
  const auto column = take(scene->emplace<Column>(root));
  const auto button = take(scene->emplace<Button>(column, text("Remove views")));
  SizePolicy fill;

  fill.width = {.preferred = 0, .mode = SizeMode::FLEX};
  fill.height = {.preferred = 0, .mode = SizeMode::FLEX};

  const auto body = take(scene->emplace<Row>(column, LinearOptions{}, fill));
  const auto chartPane = take(scene->emplace<Column>(body, LinearOptions{}, fill));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>(false)));
  const auto publisher = take(model.publisher());
  PreparedViews prepared;
  auto capture = std::make_shared<int>(42);
  const std::weak_ptr<int> captured = capture;
  bool registryChecked{};
  Node<VirtualList<Ohlc>> list;

  const auto handler = [&](std::size_t index) -> ManagedViewHandler<Ohlc> {
    return [&, index, capture](const ModelFrame<Ohlc> &frame,
                               const std::optional<ModelRequest<Ohlc>> &request) -> std::expected<void, TreeError> {
      prepared.record(index, frame, request);

      if (!registryChecked) {
        const auto rejected = scene->remove(list);

        check(!rejected && causedBy(rejected.error(), TreeError::WOULD_BLOCK),
              "Preparation allowed managed registry mutation");
        registryChecked = true;
      }

      return {};
    };
  };

  list = take(scene->virtualList(body, model, formatPrice, {}, fill, handler(0)));

  const auto columns = priceColumns();
  const auto table = take(scene->virtualTable(body, model, columns, {}, fill, handler(1)));
  const auto chart = take(scene->chart(
    chartPane, model,
    [](const Ohlc &value) {
      return std::optional{value};
    },
    {}, {}, handler(2)));

  capture.reset();
  take(scene->onClick(button, [&]() -> std::expected<void, TreeError> {
    take(scene->remove(body));
    take(scene->remove(button));

    return {};
  }));
  take(scene->theme(take(parsePss("List { foreground: #010203; } Table { foreground: #010203; } "
                                  "Chart { foreground: #010203; }"))));
  checkStyle(*app, list, {1, 2, 3});
  checkStyle(*app, table, {1, 2, 3});
  checkStyle(*app, chart, {1, 2, 3});

  bool wrongThread{};
  std::thread worker([&] {
    const auto result = model.publisher();

    wrongThread = !result && result.error() == TreeError::WRONG_THREAD;
  });

  worker.join();
  check(wrongThread, "Managed model handle lost UI affinity");

  std::size_t settledPasses{};
  std::size_t settledSubmissions{};

  terminal.checkpoint = [&](std::size_t next, TerminalRead &) {
    switch (next) {
    case 0:
      prepared.checkRequests(1, false);
      check(prepared.passes > terminal.submissions, "Pending ranges did not exercise layout retries");
      check(std::ranges::all_of(prepared.pinRevisions,
                                [&](auto revision) {
                                  return revision == prepared.pinRevisions.front();
                                }),
            "Layout retries repinned the model after opening requests");
      take(app->tree().setVisible(body, false));

      break;
    case 1:
      prepared.checkRequests(1, true);
      checkRange(*app, list, RangeState::PENDING, true);
      checkRange(*app, table, RangeState::PENDING, true);
      checkRange(*app, chart, RangeState::PENDING, true);

      for (const auto &history : prepared.requests) {
        const auto late = history.back().complete(std::make_shared<const Prices>());

        check(!late && late.error() == SchedulerError::CLOSED, "Hidden view accepted a cancelled completion");
      }

      take(app->tree().setVisible(body, true));
      take(app->tree().setEnabled(body, false));

      break;
    case 2: {
      prepared.checkRequests(2, false);
      check(take(app->router().invoke(list, {Action::MOVE_DOWN})).status == DispatchStatus::NO_TARGET,
            "Disabled managed list still accepted input");
      checkRange(*app, list, RangeState::PENDING);
      checkRange(*app, table, RangeState::PENDING);
      checkRange(*app, chart, RangeState::PENDING);
      take(prepared.requests[0].back().complete(std::make_shared<const Prices>()));

      const auto rejected = prepared.requests[1].back().complete(std::make_shared<const Prices>());

      check(!rejected && rejected.error() == SchedulerError::WOULD_BLOCK,
            "Full existing scheduler did not expose completion rejection");

      break;
    }
    case 3:
      checkRange(*app, list, RangeState::READY);
      checkRange(*app, table, RangeState::PENDING);
      take(prepared.requests[1].back().complete(std::make_shared<const Prices>()));

      break;
    case 4:
      checkRange(*app, table, RangeState::READY);
      checkRange(*app, chart, RangeState::PENDING);
      take(prepared.requests[2].back().complete(std::make_shared<const Prices>()));

      break;
    case 5:
      checkRange(*app, chart, RangeState::READY);
      take(app->tree().setEnabled(body, true));
      check(take(app->router().invoke(list, {Action::MOVE_DOWN})).status == DispatchStatus::HANDLED,
            "Managed list did not install standard input");
      check(take(app->router().invoke(table, {Action::MOVE_DOWN})).status == DispatchStatus::HANDLED,
            "Managed table did not install standard input");
      take(scene->theme(take(parsePss("List { foreground: #070809; } Table { foreground: #070809; } "
                                      "Chart { foreground: #070809; }"))));
      checkStyle(*app, list, {7, 8, 9});
      checkStyle(*app, table, {7, 8, 9});
      checkStyle(*app, chart, {7, 8, 9});

      break;
    case 6:
      checkRange(*app, list, RangeState::READY);
      checkRange(*app, table, RangeState::READY);
      settledPasses = prepared.passes;
      settledSubmissions = terminal.submissions;

      break;
    case 7:
      check(prepared.passes == settledPasses && terminal.submissions == settledSubmissions,
            "Settled managed virtual views caused idle preparation or redraw");
      take(app->router().invoke(button, {Action::ACTIVATE}));
      check(captured.expired(), "Removed virtual view callbacks retained captures");
      check(!app->tree().inspect(body) && !app->tree().inspect(button), "Input self-removal retained nodes");

      break;
    case 8:
      check(prepared.passes == settledPasses, "Removed virtual views were prepared again");

      break;
    default:
      check(false, "Unexpected managed lifecycle checkpoint");
    }
  };

  take(scene->run(terminal));

  const auto closed = model.publisher();
  const auto latePublish = publisher.publish(std::make_shared<const Prices>());

  check(terminal.closed && !app->tree().inspect(root), "Managed run did not clean up its subtree and terminal");
  check(!closed && closed.error() == TreeError::CLOSED, "Weak model handle survived scene cleanup");
  check(!latePublish && latePublish.error() == SchedulerError::CLOSED, "Publisher survived scene cleanup");
}

// The router rejects a nested registration after the factory already mounted its node.
// Its rollback must return both the managed node budget and the model's only range slot.
void partialFactoryFailure() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  ManagedOptions managedOptions;

  managedOptions.maxNodes = 3;

  auto scene = take(ManagedScene::create(*app, managedOptions));
  const auto column = take(scene->emplace<Column>(scene->root()));
  const auto button = take(scene->emplace<Button>(column, text("Attempt nested factory")));
  ModelOptions modelOptions;

  modelOptions.max_consumers = 1;

  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>(), modelOptions));
  bool rejected{};
  std::weak_ptr<int> capture;

  take(scene->onClick(button, [&]() -> std::expected<void, TreeError> {
    auto lifetime = std::make_shared<int>(1);

    capture = lifetime;

    const auto failed =
      scene->virtualList(column, model, formatPrice, {}, {},
                         [lifetime](const ModelFrame<Ohlc> &,
                                    const std::optional<ModelRequest<Ohlc>> &) -> std::expected<void, TreeError> {
                           return {};
                         });

    rejected = !failed && failed.error().phase == ManagedPhase::BIND &&
               causedBy(failed.error(), TreeError::WOULD_BLOCK) && !failed.error().cleanup;

    return {};
  }));
  take(app->router().invoke(button, {Action::ACTIVATE}));
  check(rejected && capture.expired(), "Partial factory failure lost its typed error or retained callback captures");
  check(take(app->tree().inspect(column)).children.size() == 1, "Failed factory left a partial child mounted");

  const auto list = take(scene->virtualList(column, model, formatPrice));

  check(take(app->tree().inspect(column)).children.size() == 2, "Factory rollback lost previous children or budget");
  take(scene->remove(list));
  take(scene->close());
}

void foreignModelAndBudget() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  ManagedOptions options;
  ModelOptions modelOptions;

  options.maxModels = 1;
  modelOptions.max_consumers = 1;

  auto owner = take(ManagedScene::create(*app, options));
  auto other = take(ManagedScene::create(*app));
  const auto model = take(owner->model<Ohlc>(std::make_shared<const Prices>(), modelOptions));
  const auto overflow = owner->model<Ohlc>(std::make_shared<const Prices>());

  check(!overflow && overflow.error().phase == ManagedPhase::STATE && causedBy(overflow.error(), TreeError::NODE_LIMIT),
        "Managed model registry exceeded its configured budget");

  const auto foreign = other->virtualList(other->root(), model, formatPrice);

  check(!foreign && foreign.error().phase == ManagedPhase::MOUNT &&
          causedBy(foreign.error(), TreeError::INVALID_ARGUMENT) && !foreign.error().cleanup,
        "Factory accepted a model belonging to another live scene");
  check(take(app->tree().inspect(other->root())).children.empty(), "Foreign model left a partial child");

  // The rejected factory must not consume the owner's sole range slot.
  const auto list = take(owner->virtualList(owner->root(), model, formatPrice));

  take(other->close());
  terminal.checkpoint = [&](std::size_t, TerminalRead &) {
    checkRange(*app, list, RangeState::READY);
  };
  take(owner->run(terminal));
  check(terminal.submissions == 1, "Rejecting a foreign model disturbed the owner scene");
}

void removeNextViewDuringPreparation() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto column = take(scene->emplace<Column>(scene->root()));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>()));
  Node<VirtualList<Ohlc>> later;
  bool removed{};
  std::size_t laterCalls{};
  auto lifetime = std::make_shared<int>(1);
  const std::weak_ptr<int> captured = lifetime;
  const auto first = take(scene->virtualList(
    column, model, formatPrice, {}, {},
    [&](const ModelFrame<Ohlc> &, const std::optional<ModelRequest<Ohlc>> &) -> std::expected<void, TreeError> {
      if (!removed) {
        auto result = app->tree().remove(later);

        if (!result) {
          return result;
        }

        removed = true;
      }

      return {};
    }));

  later =
    take(scene->virtualList(column, model, formatPrice, {}, {},
                            [&, lifetime](const ModelFrame<Ohlc> &,
                                          const std::optional<ModelRequest<Ohlc>> &) -> std::expected<void, TreeError> {
                              ++laterCalls;

                              return {};
                            }));
  lifetime.reset();
  terminal.checkpoint = [&](std::size_t, TerminalRead &) {
    check(removed && laterCalls == 0 && !app->tree().inspect(later), "Preparation visited a removed later sibling");
    checkRange(*app, first, RangeState::READY);
  };

  const auto result = scene->run(terminal);

  check(result.has_value() && terminal.closed && terminal.submissions == 1,
        "Removing a later sibling interrupted the frame or caused idle redraw");
  check(captured.expired(), "Cleanup retained the skipped view's callback");
}

enum class FailureMode {
  TYPED,
  ALLOCATION,
  EXCEPTION,
  REMOVAL,
  ORIGIN,
  NO_PROVIDER
};

void preparationFailure(FailureMode mode, TreeError expected) {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto root = scene->root();
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>(mode != FailureMode::NO_PROVIDER)));
  Node<Widget> node;
  auto lifetime = std::make_shared<int>(1);
  const std::weak_ptr<int> captured = lifetime;

  if (mode == FailureMode::ORIGIN) {
    node = take(scene->chart(
      root, model,
      [](const Ohlc &value) {
        return std::optional{value};
      },
      {},
      [lifetime]() -> std::expected<std::uint64_t, TreeError> {
        return std::unexpected(TreeError::INVALID_GEOMETRY);
      }));
  } else {
    ManagedViewHandler<Ohlc> callback;

    if (mode != FailureMode::NO_PROVIDER) {
      callback = [&, lifetime](const ModelFrame<Ohlc> &,
                               const std::optional<ModelRequest<Ohlc>> &) -> std::expected<void, TreeError> {
        if (mode == FailureMode::ALLOCATION) {
          throw std::bad_alloc();
        }

        if (mode == FailureMode::EXCEPTION) {
          throw std::runtime_error("provider callback failed");
        }

        if (mode == FailureMode::REMOVAL) {
          take(app->tree().remove(node));
          check(!captured.expired(), "Low-level self-removal destroyed an admitted preparation callback");
        }

        return std::unexpected(TreeError::INVALID_GEOMETRY);
      };
    }

    node = take(scene->virtualList(root, model, formatPrice, {}, {}, std::move(callback)));
  }

  lifetime.reset();

  const auto result = scene->run(terminal);

  check(!result && result.error().phase == ManagedPhase::PREPARE && result.error().node == node.id() &&
          causedBy(result.error(), expected),
        "Virtual callback failure lost its phase, node or typed cause");
  check(terminal.closed && terminal.submissions == 0 && !app->tree().inspect(root) && captured.expired(),
        "Failed virtual preparation submitted a partial frame or retained scene resources");
}
} // namespace

int main() {
  managedLifecycle();
  partialFactoryFailure();
  foreignModelAndBudget();
  removeNextViewDuringPreparation();
  preparationFailure(FailureMode::TYPED, TreeError::INVALID_GEOMETRY);
  preparationFailure(FailureMode::ALLOCATION, TreeError::NO_MEMORY);
  preparationFailure(FailureMode::EXCEPTION, TreeError::CALLBACK_FAILED);
  preparationFailure(FailureMode::REMOVAL, TreeError::INVALID_GEOMETRY);
  preparationFailure(FailureMode::ORIGIN, TreeError::INVALID_GEOMETRY);
  preparationFailure(FailureMode::NO_PROVIDER, TreeError::INVALID_ARGUMENT);
  std::cout << "managed virtual lifecycle, retries, shared pins, rollback, affinity and typed failures: verified\n";

  return EXIT_SUCCESS;
}
