// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <cstdlib>
#include <iostream>
#include <limits>
#include <portals/widget.hpp>
#include <stdexcept>
#include <thread>

namespace allocation_test {
thread_local std::ptrdiff_t remaining = -1;
}

void *operator new(std::size_t size) {
  if (allocation_test::remaining == 0) {
    throw std::bad_alloc();
  }

  if (allocation_test::remaining > 0) {
    --allocation_test::remaining;
  }

  if (void *p = std::malloc(size ? size : 1)) {
    return p;
  }

  throw std::bad_alloc();
}

void operator delete(void *p) noexcept {
  std::free(p);
}

void operator delete(void *p, std::size_t) noexcept {
  std::free(p);
}

void *operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete[](void *p) noexcept {
  ::operator delete(p);
}

void operator delete[](void *p, std::size_t) noexcept {
  ::operator delete(p);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new[](size);
  } catch (...) {
    return nullptr;
  }
}

void operator delete(void *p, const std::nothrow_t &) noexcept {
  ::operator delete(p);
}

void operator delete[](void *p, const std::nothrow_t &) noexcept {
  ::operator delete[](p);
}

namespace {
using namespace portals;
int failures{};

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

template <class T, class E> T take(std::expected<T, E> value) {
  if (!value) {
    std::cerr << "fixture failed\n";
    std::abort();
  }

  return std::move(*value);
}

template <class T> bool error(const std::expected<T, TreeError> &value, TreeError wanted) {
  return !value && value.error() == wanted;
}

struct Life {
  int mounts{}, unmounts{}, destroyed{};
  std::thread::id thread;
};

struct Probe : Widget {
  Life &life;
  int value{17};
  std::function<void()> on_mount, on_unmount, on_destroy;

  explicit Probe(Life &l, std::function<void()> mount = {}) : life(l), on_mount(std::move(mount)) {
  }

  void mount() override {
    ++life.mounts;

    if (on_mount) {
      on_mount();
    }
  }

  void unmount() override {
    ++life.unmounts;

    if (on_unmount) {
      on_unmount();
    }
  }

  ~Probe() override {
    if (on_destroy) {
      on_destroy();
    }

    ++life.destroyed;
    life.thread = std::this_thread::get_id();
  }
};

struct Fixture {
  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create());
  std::unique_ptr<WidgetTree> tree;
  Node<Widget> root;

  explicit Fixture(WidgetTreeOptions options = {})
      : tree(take(WidgetTree::create(*scheduler, options))), root(take(tree->root())) {
  }
};

void schedulerLifetime() {
  Life parent_life, child_life;
  Fixture f;
  auto parent = take(f.tree->emplace<Probe>(f.root, parent_life));
  auto child = take(f.tree->emplace<Probe>(parent, child_life));
  int delivered{}, timers{};
  std::vector<int> order;

  order.reserve(4);
  (void)f.tree->edit(child, [&](Probe &p) {
    p.on_unmount = [&] {
      order.push_back(1);
    };
  });
  (void)f.tree->edit(parent, [&](Probe &p) {
    p.on_unmount = [&] {
      order.push_back(2);
    };
  });

  auto connection = take(f.tree->connect<int>(
    child,
    [&](Probe &p, auto message) {
      ++delivered;
      check(*message == 9 && p.value == 17, "typed publication borrows target");

      auto nested = f.tree->edit(parent, [&](Probe &) {
        check(f.tree->remove(parent).has_value(), "nested callback removes ancestor");
        check(error(f.tree->edit(child,
                                 [](Probe &) {
                                 }),
                    TreeError::STALE_NODE),
              "logical death immediate");
        check(child_life.destroyed == 0 && parent_life.destroyed == 0, "nested borrow retains subtree");
      });

      check(nested.has_value() && p.value == 17, "current borrow survives ancestor removal");
      check(error(f.tree->remove(parent), TreeError::STALE_NODE), "repeat removal stale");
    },
    SchedulerLane::CONTROL));
  auto timer = take(f.tree->at(child, 0, [&](Probe &) {
    ++timers;
  }));

  (void)connection.publisher.publish(std::make_shared<const int>(9));
  (void)connection.publisher.publish(std::make_shared<const int>(10));

  auto cycle = take(f.scheduler->dispatch());

  check(cycle.control == 1 && delivered == 1 && timers == 0,
        "queued control publications and already due timer cancelled");
  check(parent_life.mounts == 1 && child_life.mounts == 1 && parent_life.unmounts == 1 && child_life.unmounts == 1,
        "mount/unmount exactly once");
  check(order == std::vector<int>({1, 2}), "children before parent teardown");
  check(child_life.destroyed == 1 && parent_life.destroyed == 1 && child_life.thread == std::this_thread::get_id(),
        "destruction on UI after callback");
  check(connection.publisher.publish(std::make_shared<const int>(11)).error() == SchedulerError::CLOSED,
        "late publication closed");
}

void timerAndEditException() {
  Life life, other_life;
  Fixture f;
  auto child = take(f.tree->emplace<Probe>(f.root, life));
  auto timer = take(f.tree->at(child, 0, [&](Probe &p) {
    check(f.tree->remove(child).has_value(), "timer self-removal");
    check(life.destroyed == 0 && p.value == 17, "timer retains active widget");
    throw std::runtime_error("application");
  }));
  auto cycle = take(f.scheduler->dispatch());

  check(cycle.timers == 1 && cycle.callback_failures == 1 && life.destroyed == 1,
        "throwing timer reclaims and reaches scheduler boundary");

  auto other = take(f.tree->emplace<Probe>(f.root, other_life));
  auto edited = f.tree->edit(other, [&](Probe &) {
    (void)f.tree->remove(other);
    throw 1;
  });

  check(error(edited, TreeError::CALLBACK_FAILED) && other_life.destroyed == 1, "throwing edit releases borrow");
}

void identityLimitsThreads() {
  Fixture f({.max_nodes = 3, .max_depth = 2});
  auto first = take(f.tree->emplace<Widget>(f.root));

  check(error(f.tree->emplace<Widget>(first), TreeError::DEPTH_LIMIT), "depth cap");

  auto second = take(f.tree->emplace<Widget>(f.root));

  check(error(f.tree->emplace<Widget>(f.root), TreeError::NODE_LIMIT), "node cap");
  (void)f.tree->remove(first);

  auto reused = take(f.tree->emplace<Widget>(f.root));

  check(reused.id().slot == first.id().slot && reused.id().generation != first.id().generation,
        "reuse changes generation");
  check(error(f.tree->edit(first,
                           [](Widget &) {
                           }),
              TreeError::STALE_NODE),
        "stale generation rejected");
  check(take(f.tree->inspect(f.root)).children == std::vector<Node<Widget>>({second, reused}),
        "stable insertion order");
  check(take(f.tree->inspect(reused)).parent == f.root, "parent identity");

  Fixture foreign;
  bool called{}, rejected{};

  check(error(foreign.tree->edit(reused,
                                 [&](Widget &) {
                                   called = true;
                                 }),
              TreeError::WRONG_SESSION),
        "foreign session rejected");
  check(error(f.tree->edit(Node<Widget>{},
                           [&](Widget &) {
                             called = true;
                           }),
              TreeError::STALE_NODE),
        "default observer rejected");

  std::thread worker([&] {
    rejected = error(f.tree->edit(reused,
                                  [&](Widget &) {
                                    called = true;
                                  }),
                     TreeError::WRONG_THREAD) &&
               error(f.tree->remove(reused), TreeError::WRONG_THREAD) &&
               error(f.tree->root(), TreeError::WRONG_THREAD) && error(f.tree->close(), TreeError::WRONG_THREAD) &&
               error(WidgetTree::create(*f.scheduler), TreeError::WRONG_THREAD);
  });

  worker.join();
  check(rejected && !called, "wrong-thread rejection precedes callback");

  auto saved = reused;

  f.tree.reset();
  f.tree = take(WidgetTree::create(*f.scheduler));
  check(error(f.tree->edit(saved,
                           [](Widget &) {
                           }),
              TreeError::WRONG_SESSION),
        "session identity survives address reuse");
  check(error(WidgetTree::create(*f.scheduler, {.max_nodes = 0}), TreeError::INVALID_ARGUMENT), "zero nodes invalid");
  check(error(WidgetTree::create(*f.scheduler, {.max_depth = 257}), TreeError::INVALID_ARGUMENT),
        "stack bound explicit");
}

void reentrantLifecycle() {
  Life self_life, parent_life, first_life, last_life, destructor_life, survivor_life;
  Fixture f;
  auto self = take(f.tree->emplace<Probe>(f.root, self_life, [&] {
    auto children = take(f.tree->inspect(f.root)).children;

    check(children.size() == 1, "mount sees committed node");
    check(error(f.tree->emplace<Widget>(f.root), TreeError::WOULD_BLOCK), "lifecycle insertion rejected explicitly");
    (void)f.tree->remove(children[0]);
    check(self_life.destroyed == 0, "mount self-removal retains storage");
  }));

  check(error(f.tree->inspect(self), TreeError::STALE_NODE) && self_life.mounts == 1 && self_life.unmounts == 1 &&
          self_life.destroyed == 1,
        "self-removing mount returns committed stale identity and balanced lifecycle");

  auto parent = take(f.tree->emplace<Probe>(f.root, parent_life));
  auto first = take(f.tree->emplace<Probe>(parent, first_life));
  auto last = take(f.tree->emplace<Probe>(parent, last_life));

  (void)f.tree->edit(last, [&](Probe &p) {
    p.on_unmount = [&] {
      check(f.tree->remove(parent).has_value(), "unmount removes ancestor");
    };
  });
  (void)f.tree->remove(last);
  check(first_life.unmounts == 1 && first_life.destroyed == 1 && last_life.unmounts == 1 && parent_life.unmounts == 1,
        "reentrant teardown visits previous sibling once");
  check(error(f.tree->inspect(first), TreeError::STALE_NODE), "ancestor removal invalidates siblings");

  auto survivor = take(f.tree->emplace<Probe>(f.root, survivor_life));
  auto dying = take(f.tree->emplace<Probe>(survivor, destructor_life));

  (void)f.tree->edit(dying, [&](Probe &p) {
    p.on_destroy = [&] {
      check(take(f.tree->inspect(survivor)).children.empty(), "destructor snapshot skips empty ownership slot");
      check(f.tree->invalidate(survivor, Invalidation::LAYOUT).has_value(), "destructor invalidates parent");
      check(f.tree->remove(survivor).has_value(), "destructor removes ancestor");
    };
  });
  (void)f.tree->remove(dying);
  check(destructor_life.destroyed == 1 && survivor_life.destroyed == 1, "destructor reentry completes reclamation");
}

void lifecycleErrorsClose() {
  Life life;
  auto scheduler = take(Scheduler::create());
  std::vector<NodeId> errors;

  errors.reserve(8);

  auto tree = take(WidgetTree::create(
    *scheduler, {},
    [](void *context, NodeId id, TreeError code) noexcept {
      check(code == TreeError::CALLBACK_FAILED, "sink reports callback failure");
      static_cast<std::vector<NodeId> *>(context)->push_back(id);
    },
    &errors));
  auto child = take(tree->emplace<Probe>(take(tree->root()), life, [] {
    throw 2;
  }));

  check(errors.size() == 1 && errors[0] == child.id() && tree->inspect(child).has_value(),
        "mount failure reports committed identity");
  (void)tree->edit(child, [&](Probe &p) {
    p.on_unmount = [] {
      throw 3;
    };
  });
  (void)tree->edit(child, [&](Probe &p) {
    check(tree->close().has_value(), "close from callback");
    check(error(tree->edit(child,
                           [](Probe &) {
                           }),
                TreeError::CLOSED),
          "close forbids new borrows");
    check(life.destroyed == 0 && p.value == 17, "close retains current widget");
  });
  check(life.unmounts == 1 && life.destroyed == 1 && errors.size() == 2, "throwing unmount does not prevent cleanup");
  check(tree->close().has_value(), "close idempotent");
}

struct FailingConstructor : Widget {
  explicit FailingConstructor(bool allocationFailure) {
    if (allocationFailure) {
      throw std::bad_alloc();
    }

    throw std::runtime_error("constructor failed");
  }
};

void checkedInsertion() {
  Life failedLife, removedLife, siblingLife, childLife, parentLife;
  Fixture fixture;
  auto failed = fixture.tree->emplaceChecked<Probe>(fixture.root, failedLife, [] {
    throw std::runtime_error("mount failed");
  });

  check(error(failed, TreeError::CALLBACK_FAILED), "checked insertion returns mount failure");
  check(take(fixture.tree->inspect(fixture.root)).children.empty() && failedLife.unmounts == 1 &&
          failedLife.destroyed == 1,
        "checked insertion removes failed attachment and balances lifecycle");

  auto removed = fixture.tree->emplaceChecked<Probe>(fixture.root, removedLife, [&] {
    const auto children = take(fixture.tree->inspect(fixture.root)).children;

    (void)fixture.tree->remove(children.back());
  });

  check(error(removed, TreeError::STALE_NODE), "checked insertion rejects mount self-removal");

  auto sibling = take(fixture.tree->emplaceChecked<Probe>(fixture.root, siblingLife));
  auto parent = take(fixture.tree->emplaceChecked<Probe>(fixture.root, parentLife));
  auto child = fixture.tree->emplaceChecked<Probe>(parent, childLife, [&] {
    (void)fixture.tree->remove(parent);

    throw std::runtime_error("removed subtree then failed");
  });

  check(error(child, TreeError::CALLBACK_FAILED) && childLife.destroyed == 1 && parentLife.destroyed == 1,
        "mount exception after ancestor removal is not lost or double-reclaimed");
  check(take(fixture.tree->inspect(fixture.root)).children == std::vector<Node<Widget>>{sibling},
        "checked failure preserves unrelated live subtrees");

  int reports{};
  Life reportedLife;
  auto reportedTree = take(WidgetTree::create(
    *fixture.scheduler, {},
    [](void *context, NodeId, TreeError code) noexcept {
      check(code == TreeError::CALLBACK_FAILED, "checked mount failure sink receives original error");
      ++*static_cast<int *>(context);
    },
    &reports));
  auto reported = reportedTree->emplaceChecked<Probe>(take(reportedTree->root()), reportedLife, [] {
    throw std::bad_alloc();
  });

  check(error(reported, TreeError::CALLBACK_FAILED) && reports == 1 && reportedLife.destroyed == 1,
        "checked mount bad_alloc remains callback failure and reaches sink exactly once");
  check(error(fixture.tree->emplaceChecked<FailingConstructor>(fixture.root, true), TreeError::NO_MEMORY) &&
          error(fixture.tree->emplaceChecked<FailingConstructor>(fixture.root, false), TreeError::CALLBACK_FAILED),
        "checked construction retains allocation and callback failure categories");
}

void checkedInsertionAfterReclamation() {
  Life siblingLife, childLife;
  Fixture fixture;
  const auto parent = take(fixture.tree->emplace<Widget>(fixture.root));
  const auto sibling = take(fixture.tree->emplace<Probe>(parent, siblingLife));

  check(fixture.tree
          ->edit(sibling,
                 [&](Probe &widget) {
                   widget.on_unmount = [&] {
                     check(fixture.tree->remove(parent).has_value(), "deferred unmount removes parent");
                   };
                 })
          .has_value(),
        "install deferred unmount callback");

  const auto inserted = fixture.tree->emplaceChecked<Probe>(parent, childLife, [&] {
    check(fixture.tree->remove(sibling).has_value(), "mount retires sibling");
  });

  check(error(inserted, TreeError::STALE_NODE), "checked insertion revalidates after deferred unmount callbacks");
  check(siblingLife.destroyed == 1 && childLife.destroyed == 1 &&
          take(fixture.tree->inspect(fixture.root)).children.empty(),
        "deferred ancestor removal retires the new child exactly once");
}

struct View : Widget {
  std::shared_ptr<int> model;

  explicit View(std::shared_ptr<int> owner) : model(std::move(owner)) {
  }
};

void sharedModel() {
  Fixture f;
  auto model = std::make_shared<int>(42);
  std::weak_ptr<int> observer = model;
  auto one = take(f.tree->emplace<View>(f.root, model));
  auto two = take(f.tree->emplace<View>(f.root, model));

  model.reset();
  (void)f.tree->remove(one);
  check(!observer.expired(), "second view retains explicit model owner");
  (void)f.tree->edit(two, [](View &view) {
    check(*view.model == 42, "remaining model readable");
  });
  (void)f.tree->remove(two);
  check(observer.expired(), "last explicit owner frees model without cycle");
}

struct Custom final : Widget {
  GraphemeText text = take(GraphemeText::from("meter"));
  CellRect bounds;
  bool fail{};

  std::expected<CellSize, TreeError> measure(CellSize available) override {
    return CellSize{available.width / 2, 1};
  }

  std::expected<void, TreeError> arrange(CellRect value) override {
    if (fail) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    bounds = value;

    return {};
  }

  std::expected<void, TreeError> paint(PaintContext &context) override {
    RenderCommand command = TextCommand{bounds.origin(), text, bounds};

    return context.submit(std::span(&command, 1));
  }
};

struct Recorder : PaintContext {
  std::vector<RenderCommand> commands;

  std::expected<void, TreeError> submit(std::span<const RenderCommand> values) override {
    for (const auto &value : values) {
      commands.push_back(value);
    }

    return {};
  }
};

void geometryFrame() {
  Fixture f;
  auto node = take(f.tree->emplace<Custom>(f.root));
  auto viewport = take(CellRect::from({}, {20, 5}));

  check(!take(f.tree->inspect(node)).geometry.valid, "unarranged geometry invalid");
  check(take(f.tree->consumeFrameRequest()) == Invalidation::LAYOUT &&
          take(f.tree->consumeFrameRequest()) == Invalidation::NONE,
        "structure coalesces one frame request");
  (void)f.tree->setVisible(node, true);
  (void)f.tree->setEnabled(node, true);
  check(take(f.tree->consumeFrameRequest()) == Invalidation::NONE, "equal setters do nothing");
  check(take(f.tree->measure(node, {20, 5})) == CellSize{10, 1}, "external measure hook");
  (void)f.tree->arrange(f.root, viewport, viewport);

  auto local = take(CellRect::from({2, 1}, {8, 2}));

  (void)f.tree->arrange(node, local, viewport);

  auto state = take(f.tree->inspect(node));

  check(state.geometry.valid && state.geometry.surface == local && state.geometry.clip == local,
        "owning surface geometry and clip");

  Recorder sink;

  check(error(f.tree->paint(node, sink), TreeError::WOULD_BLOCK), "paint requires full-frame borrow");
  check(f.tree
          ->frame([&] {
            check(f.tree
                    ->read(node,
                           [](const Custom &value) {
                             check(!value.fail, "layout owner can read custom const interface");
                           })
                    .has_value(),
                  "read works in frame");
            check(f.tree->paint(node, sink).has_value(), "paint in frame");
            check(error(f.tree->remove(node), TreeError::WOULD_BLOCK) && error(f.tree->close(), TreeError::WOULD_BLOCK),
                  "frame retains text before render");
            check(renderFullFrame(viewport.size(), sink.commands).has_value(), "borrowed text rendered within frame");
            sink.commands.clear();
          })
          .has_value(),
        "frame completes");
  (void)f.tree->invalidate(node, Invalidation::LAYOUT);
  check(!take(f.tree->inspect(node)).geometry.valid && take(f.tree->inputEligible(node)),
        "layout invalidation preserves keyboard eligibility");
  (void)f.tree->setEnabled(f.root, false);
  check(!take(f.tree->inputEligible(node)), "disabled ancestor excludes input");
  (void)f.tree->setEnabled(f.root, true);
  (void)f.tree->setVisible(f.root, false);
  check(take(f.tree->measure(node, {20, 5})) == CellSize{} && !take(f.tree->inputEligible(node)),
        "hidden ancestor excludes layout/input");
  (void)f.tree->setVisible(f.root, true);
  (void)f.tree->arrange(f.root, viewport, viewport);
  (void)f.tree->arrange(node, local, viewport);
  (void)f.tree->edit(node, [](Custom &widget) {
    widget.fail = true;
  });
  check(error(f.tree->arrange(node, local, viewport), TreeError::INVALID_GEOMETRY) &&
          !take(f.tree->inspect(node)).geometry.valid,
        "failed arrange invalidates old geometry");

  auto extreme = take(CellRect::from({std::numeric_limits<std::int32_t>::max() - 1, 0}, {1, 1}));

  (void)f.tree->arrange(f.root, extreme, extreme);
  check(error(f.tree->arrange(node, local, viewport), TreeError::INVALID_GEOMETRY), "surface overflow rejected");
}

struct FailingHooks final : Widget {
  std::function<void()> probe;
  bool throws{};

  std::expected<CellSize, TreeError> measure(CellSize) override {
    probe();

    if (throws) {
      throw 1;
    }

    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  std::expected<void, TreeError> paint(PaintContext &context) override {
    probe();

    auto area = take(CellRect::from({}, {1, 1}));
    RenderCommand command = FillCommand{area, area};

    (void)context.submit(std::span(&command, 1));

    if (throws) {
      throw 2;
    }

    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }
};

void failingHooks() {
  auto scheduler = take(Scheduler::create());
  int reports{};
  auto tree = take(WidgetTree::create(
    *scheduler, {},
    [](void *context, NodeId, TreeError) noexcept {
      ++*static_cast<int *>(context);
    },
    &reports));
  auto root = take(tree->root());
  auto node = take(tree->emplace<FailingHooks>(root));
  auto viewport = take(CellRect::from({}, {4, 2}));

  (void)tree->edit(node, [&](FailingHooks &value) {
    value.probe = [&] {
      check(error(tree->remove(node), TreeError::WOULD_BLOCK), "hook rejects removal");
      check(error(tree->edit(node,
                             [](FailingHooks &) {
                             }),
                  TreeError::WOULD_BLOCK),
            "hook rejects mutable borrow");
      check(error(tree->measure(root, {1, 1}), TreeError::WOULD_BLOCK), "hook rejects recursive extension entry");
    };
  });

  check(error(tree->measure(node, {4, 2}), TreeError::INVALID_GEOMETRY), "measure failure propagates");
  (void)tree->edit(node, [](FailingHooks &value) {
    value.throws = true;
  });
  check(error(tree->measure(node, {4, 2}), TreeError::CALLBACK_FAILED), "measure exception contained");
  (void)tree->arrange(root, viewport, viewport);
  (void)tree->arrange(node, viewport, viewport);

  Recorder transaction;

  (void)tree->frame([&] {
    check(error(tree->paint(node, transaction), TreeError::CALLBACK_FAILED), "paint exception contained");
    check(transaction.commands.size() == 1, "caller owns commands recorded before failure");
    transaction.commands.clear();
  });
  (void)tree->edit(node, [](FailingHooks &value) {
    value.throws = false;
  });
  (void)tree->frame([&] {
    check(error(tree->paint(node, transaction), TreeError::INVALID_GEOMETRY), "paint failure propagates");
    transaction.commands.clear();
  });
  check(reports == 4, "returned and thrown measure/paint errors reach sink exactly once");
  (void)tree->remove(node);

  bool called{};

  check(error(tree->read(node,
                         [&](const FailingHooks &) {
                           called = true;
                         }),
              TreeError::STALE_NODE),
        "read rejects stale");

  auto foreign = take(WidgetTree::create(*scheduler));

  check(error(foreign->read(root,
                            [&](const Widget &) {
                              called = true;
                            }),
              TreeError::WRONG_SESSION) &&
          !called,
        "read rejects foreign before external code");
}

void allocationAndSequences() {
  auto scheduler = take(Scheduler::create());
  bool created{};
  int factory_failures{};

  for (std::ptrdiff_t limit = 0; limit < 16 && !created; ++limit) {
    allocation_test::remaining = limit;

    auto tree = WidgetTree::create(*scheduler, {.max_nodes = 8});

    allocation_test::remaining = -1;

    if (tree) {
      created = true;
    } else {
      ++factory_failures;
      check(error(tree, TreeError::NO_MEMORY), "factory translates every allocation failure");
    }
  }

  check(created && factory_failures >= 5, "factory allocation sites exercised without partial tree");
  check(error(WidgetTree::create(*scheduler, {.max_nodes = std::numeric_limits<std::size_t>::max()}),
              TreeError::INVALID_ARGUMENT),
        "impossible registry capacity rejected before allocation");

  int failures_seen{};
  bool succeeded{};

  for (std::ptrdiff_t limit = 0; limit < 16 && !succeeded; ++limit) {
    Fixture f({.max_nodes = 8});

    allocation_test::remaining = limit;

    auto node = f.tree->emplace<Widget>(f.root);

    allocation_test::remaining = -1;

    if (node) {
      succeeded = true;
    } else {
      ++failures_seen;
      check(error(node, TreeError::NO_MEMORY) && take(f.tree->inspect(f.root)).children.empty(),
            "allocation failure leaves no attachment");
    }
  }

  check(succeeded && failures_seen >= 3, "allocation failures tested at multiple real allocation sites");

  Life life;
  Fixture f({.max_nodes = 32});
  auto doomed = take(f.tree->emplace<Probe>(f.root, life));
  auto sibling_one = take(f.tree->emplace<Widget>(f.root));
  auto sibling_two = take(f.tree->emplace<Widget>(f.root));
  bool inspected{};
  int snapshot_failures{};

  for (std::ptrdiff_t limit = 0; limit < 8 && !inspected; ++limit) {
    allocation_test::remaining = limit;

    auto snapshot = f.tree->inspect(f.root);

    allocation_test::remaining = -1;

    if (snapshot) {
      inspected = true;
      check(snapshot->children.size() == 3, "snapshot returns all children after allocation fault sweep");
    } else {
      ++snapshot_failures;
      check(error(snapshot, TreeError::NO_MEMORY), "snapshot proxy/storage failure returns no_memory");
    }
  }

  check(inspected && snapshot_failures >= 2, "snapshot and return-copy allocation failures exercised");
  (void)f.tree->remove(sibling_one);
  (void)f.tree->remove(sibling_two);

  auto connection = take(f.tree->connect<int>(doomed, [](Probe &, auto) {
  }));

  (void)connection.publisher.publish(std::make_shared<const int>(1));
  allocation_test::remaining = 0;

  auto removed = f.tree->remove(doomed);

  allocation_test::remaining = -1;
  check(removed.has_value() && life.destroyed == 1, "remove/cancel/reclaim allocates no memory");

  std::vector<Node<Widget>> live, stale;
  std::uint32_t seed = 0x015aceu;

  for (int step = 0; step < 600; ++step) {
    seed = seed * 1664525u + 1013904223u;

    if (live.empty() || (live.size() < 24 && (seed & 4))) {
      live.push_back(take(f.tree->emplace<Widget>(f.root)));
    } else {
      auto position = static_cast<std::size_t>(seed) % live.size();

      stale.push_back(live[position]);
      (void)f.tree->remove(live[position]);
      live.erase(live.begin() + static_cast<std::ptrdiff_t>(position));
    }

    check(take(f.tree->inspect(f.root)).children == live, "generated order/exclusive ownership preserved");

    for (auto node : live) {
      check(take(f.tree->inspect(node)).parent == f.root, "generated single parent");
    }

    if (!stale.empty()) {
      check(error(f.tree->inspect(stale.back()), TreeError::STALE_NODE), "generated stale observer never revives");
    }
  }
}
} // namespace

int main() {
  std::cerr << "scheduler_lifetime\n";
  schedulerLifetime();
  std::cerr << "timer_and_edit_exception\n";
  timerAndEditException();
  std::cerr << "identity_limits_threads\n";
  identityLimitsThreads();
  std::cerr << "reentrant_lifecycle\n";
  reentrantLifecycle();
  std::cerr << "lifecycle_errors_close\n";
  lifecycleErrorsClose();
  checkedInsertion();
  checkedInsertionAfterReclamation();
  std::cerr << "shared_model\n";
  sharedModel();
  std::cerr << "geometry_frame\n";
  geometryFrame();
  std::cerr << "allocation_and_sequences\n";
  allocationAndSequences();
  failingHooks();

  return failures == 0 ? 0 : 1;
}
