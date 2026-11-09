// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include <cstddef>
#include <set>
#include <utility>

using namespace dxfcpp;

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
