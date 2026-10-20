// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/internal/utils/ConstString.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>

using namespace dxfcpp;
using namespace std::literals;

namespace {

constinit const ConstString CONSTANT = ConstString{ConstString::Literal{"CONSTANT"}};
// Not EMPTY{}: GCC 14 rejects the value-initialization of a non-literal type in a constinit initializer.
constinit const ConstString EMPTY{ConstString::Literal{""}};

} // namespace

TEST_CASE("A literal ConstString is constant-initialized and does not own its characters") {
    REQUIRE(CONSTANT.view() == "CONSTANT"sv);
    REQUIRE(CONSTANT.str() == "CONSTANT"s);
    REQUIRE(!CONSTANT.isOwned());
    REQUIRE(EMPTY.empty());
    REQUIRE(EMPTY.size() == 0);
}

TEST_CASE("An owned ConstString outlives its source and is shared by its copies") {
    auto source = std::string("RUNTIME");
    auto owned = ConstString::copyOf(source);

    source.assign("CHANGED");
    REQUIRE(owned.isOwned());
    REQUIRE(owned.view() == "RUNTIME"sv);

    std::string_view view{};

    {
        auto copy = owned;

        REQUIRE(copy.view().data() == owned.view().data());

        owned = ConstString{};
        view = copy.view();

        auto moved = std::move(copy);

        REQUIRE(copy.empty());
        REQUIRE(moved.view().data() == view.data());

        owned = moved;
    }

    REQUIRE(owned.view().data() == view.data());
    REQUIRE(owned.view() == "RUNTIME"sv);
}

TEST_CASE("Literal and owned ConstStrings with the same characters are equal and have the same hash") {
    auto literal = ConstString{ConstString::Literal{"NTV"}};
    auto owned = ConstString::copyOf("NTV"s);

    REQUIRE(literal == owned);
    REQUIRE(owned == "NTV"sv);
    REQUIRE(literal != ConstString::copyOf("NTW"sv));
    REQUIRE(std::hash<ConstString>{}(literal) == std::hash<ConstString>{}(owned));
    REQUIRE(std::hash<ConstString>{}(literal) == std::hash<std::string_view>{}("NTV"sv));
}
