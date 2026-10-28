// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "../support/TestSupport.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <latch>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

using namespace dxfcpp;
using namespace std::chrono_literals;
using dxfcpp::test::ErrorManagerGuard;

namespace {

using IntHandler = SimpleHandler<void(int)>;

} // namespace

// MDAPI-429: the listener ids are unique within a handler (the counter is a member, not a static of the template: it
// would have a copy in every module on Windows).
TEST_CASE("SimpleHandler gives unique ids within the handler and calls the listeners") {
    IntHandler handler{};
    int sum = 0;

    const auto first = handler.add([&](int value) {
        sum += value;
    });
    const auto second = handler.addLowPriority([&](int value) {
        sum += 10 * value;
    });

    CHECK(first != second);
    handler.handle(1);
    CHECK(sum == 11);

    handler.remove(first);
    handler.handle(1);
    CHECK(sum == 21);
}

// A new id must differ from the ids of the listeners that the handler holds after a move.
TEST_CASE("SimpleHandler does not reuse the ids of the moved listeners") {
    IntHandler source{};
    std::set<std::size_t> sourceIds{source.add([](int) {
                                    }),
                                    source.add([](int) {
                                    })};

    SUBCASE("move construction") {
        IntHandler moved{std::move(source)};

        CHECK(sourceIds
                  .insert(moved.add([](int) {
                  }))
                  .second);
    }

    SUBCASE("move assignment") {
        IntHandler target{};
        std::set<std::size_t> targetIds{target.add([](int) {
        })};

        target = std::move(source); // the handlers swap their listeners

        CHECK(sourceIds
                  .insert(target.add([](int) {
                  }))
                  .second);
        CHECK(targetIds
                  .insert(source.add([](int) {
                  }))
                  .second);
    }
}

// MDAPI-441: the listeners are called on the notifying thread, as in the Java API.
TEST_CASE("SimpleHandler calls the listeners on the thread of handle()") {
    IntHandler handler{};
    std::thread::id listenerThread{};

    handler.add([&](int) {
        listenerThread = std::this_thread::get_id();
    });
    handler.handle(1);

    CHECK(listenerThread == std::this_thread::get_id());
}

TEST_CASE("SimpleHandler calls the main listeners before the low priority ones, in the order of addition") {
    IntHandler handler{};
    std::vector<int> calls{};

    handler.addLowPriority([&](int) {
        calls.push_back(3);
    });
    handler.add([&](int) {
        calls.push_back(1);
    });
    handler.add([&](int) {
        calls.push_back(2);
    });
    handler.handle(0);

    CHECK(calls == std::vector{1, 2, 3});
}

TEST_CASE("A listener of SimpleHandler can remove itself") {
    IntHandler handler{};
    std::size_t selfId{};
    int selfCalls = 0;
    int otherCalls = 0;

    selfId = handler.add([&](int) {
        selfCalls++;
        handler.remove(selfId);
    });
    handler.add([&](int) {
        otherCalls++;
    });

    handler.handle(1);
    handler.handle(2);

    CHECK(selfCalls == 1);
    CHECK(otherCalls == 2);
}

// The snapshot of the batch still holds the removed listener: it is skipped, not called.
TEST_CASE("A listener of SimpleHandler can remove a listener that follows it in the batch") {
    IntHandler handler{};
    std::size_t nextId{};
    int nextCalls = 0;

    handler.add([&](int) {
        handler.remove(nextId);
    });
    nextId = handler.add([&](int) {
        nextCalls++;
    });

    handler.handle(1);
    handler.handle(2);

    CHECK(nextCalls == 0);
}

TEST_CASE("A listener of SimpleHandler can add a listener: the new one is called from the next notification") {
    IntHandler handler{};
    int addedCalls = 0;
    bool added = false;

    handler.add([&](int) {
        if (!added) {
            added = true;
            handler.add([&](int) {
                addedCalls++;
            });
        }
    });

    handler.handle(1);
    CHECK(addedCalls == 0);

    handler.handle(2);
    CHECK(addedCalls == 1);
}

TEST_CASE("A listener of SimpleHandler can notify the same handler") {
    IntHandler handler{};
    std::vector<int> values{};

    handler.add([&](int value) {
        values.push_back(value);

        if (value == 1) {
            handler.handle(2);
        }
    });

    handler.handle(1);

    CHECK(values == std::vector{1, 2});
}

TEST_CASE("An exception of a listener of SimpleHandler goes to ErrorManager, the other listeners are called") {
    ErrorManagerGuard guard{};
    IntHandler handler{"TestComponent"};
    int otherCalls = 0;

    handler.add([](int) {
        throw std::runtime_error("listener failed");
    });
    handler.add([&](int) {
        otherCalls++;
    });

    CHECK_NOTHROW(handler.handle(1));
    CHECK(otherCalls == 1);

    const auto error = ErrorManager::getLastError();

    REQUIRE(error.has_value());
    CHECK(error->component == "TestComponent");
    CHECK(error->message == "listener failed");
}

TEST_CASE("SimpleHandler::remove() on another thread returns after the call of the listener in progress ends") {
    IntHandler handler{};
    std::promise<void> entered{};
    std::promise<void> release{};
    auto releaseFuture = release.get_future().share();
    std::atomic<bool> callEnded{false};
    std::atomic<bool> removeReturned{false};
    std::atomic<bool> endedBeforeRemoveReturned{false};

    const auto id = handler.add([&](int) {
        entered.set_value();
        releaseFuture.wait();
        callEnded = true;
    });

    std::thread notifier{[&] {
        handler.handle(1);
    }};

    if (entered.get_future().wait_for(5s) != std::future_status::ready) {
        release.set_value();
        notifier.join();
        FAIL("the listener was not called");
    }

    std::thread remover{[&] {
        handler.remove(id);
        endedBeforeRemoveReturned = callEnded.load();
        removeReturned = true;
    }};

    std::this_thread::sleep_for(100ms);
    CHECK_FALSE(removeReturned.load());

    release.set_value();
    remover.join();
    notifier.join();

    CHECK(removeReturned.load());
    CHECK(endedBeforeRemoveReturned.load());
}

// Inside a Graal callback, remove() does not wait: the call in progress may be waiting for this callback to return.
TEST_CASE("SimpleHandler::remove() inside a callback of the library does not wait for the call in progress") {
    IntHandler handler{};
    std::promise<void> entered{};
    std::promise<void> release{};
    auto releaseFuture = release.get_future().share();

    const auto id = handler.add([&](int) {
        entered.set_value();
        releaseFuture.wait();
    });

    std::thread notifier{[&] {
        handler.handle(1);
    }};

    if (entered.get_future().wait_for(5s) != std::future_status::ready) {
        release.set_value();
        notifier.join();
        FAIL("the listener was not called");
    }

    CHECK_FALSE(ListenerCallScope::isActive());

    auto removed = std::async(std::launch::async, [&] {
        const Isolate::CallbackScope callbackScope{};
        const bool active = ListenerCallScope::isActive();

        handler.remove(id);

        return active;
    });
    const auto removeStatus = removed.wait_for(5s);

    release.set_value();
    notifier.join();

    REQUIRE(removeStatus == std::future_status::ready);
    CHECK(removed.get());
}

TEST_CASE("SimpleHandler::removeAll() on another thread returns after the calls in progress end") {
    IntHandler handler{};
    std::promise<void> entered{};
    std::promise<void> release{};
    auto releaseFuture = release.get_future().share();
    std::atomic<bool> callEnded{false};
    std::atomic<bool> removeReturned{false};
    std::atomic<bool> endedBeforeRemoveReturned{false};
    std::atomic<int> calls{0};

    handler.add([&](int) {
        calls++;
        entered.set_value();
        releaseFuture.wait();
        callEnded = true;
    });
    handler.addLowPriority([&](int) {
        calls++;
    });

    std::thread notifier{[&] {
        handler.handle(1);
    }};

    if (entered.get_future().wait_for(5s) != std::future_status::ready) {
        release.set_value();
        notifier.join();
        FAIL("the listener was not called");
    }

    std::thread remover{[&] {
        handler.removeAll();
        endedBeforeRemoveReturned = callEnded.load();
        removeReturned = true;
    }};

    std::this_thread::sleep_for(100ms);
    CHECK_FALSE(removeReturned.load());

    release.set_value();
    remover.join();
    notifier.join();

    CHECK(removeReturned.load());
    CHECK(endedBeforeRemoveReturned.load());

    // The low priority listener was removed before the notification reached it.
    CHECK(calls.load() == 1);

    handler.handle(2);
    CHECK(calls.load() == 1);
}

TEST_CASE("SimpleHandler::removeAll() from a listener does not wait and the listeners are not called anymore") {
    IntHandler handler{};
    int calls = 0;

    handler.add([&](int) {
        calls++;
        handler.removeAll();
    });
    handler.add([&](int) {
        calls++;
    });

    handler.handle(1);
    handler.handle(2);

    CHECK(calls == 1);

    // The handler can be used again.
    handler.add([&](int) {
        calls++;
    });
    handler.handle(3);

    CHECK(calls == 2);
}

// Each listener removes the other while the other is called on another thread: a wait in remove() would deadlock.
TEST_CASE("Two listeners of SimpleHandler can remove each other on two threads") {
    IntHandler handler{};
    std::size_t firstId{};
    std::size_t secondId{};
    std::latch bothCalled{2};

    firstId = handler.add([&](int value) {
        if (value == 1) {
            bothCalled.arrive_and_wait();
            handler.remove(secondId);
        }
    });
    secondId = handler.add([&](int value) {
        if (value == 2) {
            bothCalled.arrive_and_wait();
            handler.remove(firstId);
        }
    });

    auto first = std::async(std::launch::async, [&] {
        handler.handle(1);
    });
    auto second = std::async(std::launch::async, [&] {
        handler.handle(2);
    });

    REQUIRE(first.wait_for(5s) == std::future_status::ready);
    REQUIRE(second.wait_for(5s) == std::future_status::ready);
}

TEST_CASE("A listener of SimpleHandler removed on another thread is not called after remove() returns") {
    IntHandler handler{};
    std::atomic<bool> removed{false};
    std::atomic<bool> calledAfterRemove{false};
    std::atomic<bool> stop{false};

    const auto id = handler.add([&](int) {
        if (removed.load()) {
            calledAfterRemove = true;
        }
    });

    std::thread notifier{[&] {
        while (!stop.load()) {
            handler.handle(1);
        }
    }};

    std::this_thread::sleep_for(10ms);
    handler.remove(id);
    removed = true;
    std::this_thread::sleep_for(10ms);
    stop = true;
    notifier.join();

    CHECK_FALSE(calledAfterRemove.load());
}

// For ThreadSanitizer: add(), remove() and handle() on several threads at once.
TEST_CASE("SimpleHandler supports add, remove and handle on several threads") {
    IntHandler handler{};
    std::atomic<std::size_t> calls{0};
    std::atomic<std::size_t> notifications{0};
    std::atomic<bool> stop{false};
    std::vector<std::thread> threads{};

    handler.add([&](int) {
        calls++;
    });

    for (int i = 0; i < 2; i++) {
        threads.emplace_back([&] {
            while (!stop.load()) {
                const auto id = handler.add([](int) {
                });

                handler.remove(id);
            }
        });
        threads.emplace_back([&] {
            while (!stop.load()) {
                handler.handle(1);
                notifications++;
            }
        });
    }

    std::this_thread::sleep_for(500ms);
    stop = true;

    for (auto &thread : threads) {
        thread.join();
    }

    CHECK(calls.load() == notifications.load());
}
