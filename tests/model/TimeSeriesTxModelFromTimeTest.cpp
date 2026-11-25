// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

using namespace std::literals;
using namespace dxfcpp;
using dxfcpp::test::ErrorManagerGuard;

namespace {

constexpr auto SYMBOL = "TEST-SYMBOL";
constexpr std::int64_t HOUR = 60 * 60 * 1000;

std::shared_ptr<TimeSeriesTxModel<TimeAndSale>::Builder> builder() {
    return TimeSeriesTxModel<TimeAndSale>::newBuilder()->withSymbol(SYMBOL);
}

std::vector<ErrorInfo> warnings() {
    std::vector<ErrorInfo> result{};

    for (const auto &error : ErrorManager::getErrors()) {
        if (error.level == ErrorInfo::Level::WARN) {
            result.push_back(error);
        }
    }

    return result;
}

void checkOneFromTimeWarning() {
    const auto errors = ErrorManager::getErrors();

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].level == ErrorInfo::Level::WARN);
    CHECK(errors[0].component == "TimeSeriesTxModel");
    CHECK(errors[0].message.find("fromTime") != std::string::npos);
    CHECK(errors[0].message.find("not in the past") != std::string::npos);
}

} // namespace

TEST_CASE("TimeSeriesTxModel::setFromTime warns about a time in the future and still sets it") {
    ErrorManagerGuard guard{};
    const auto model = builder()->build();
    REQUIRE(ErrorManager::getErrors().empty()); // the default fromTime means "not subscribed"

    const auto fromTime = dxfcpp::now() + HOUR;
    model->setFromTime(fromTime);

    checkOneFromTimeWarning();
    CHECK(model->getFromTime() == fromTime);

    model->setFromTime(fromTime); // the same time again changes nothing: no second warning
    CHECK(ErrorManager::getErrors().size() == 1);

    model->close();
}

TEST_CASE("TimeSeriesTxModel::setFromTime with std::chrono::milliseconds warns about a time in the future") {
    ErrorManagerGuard guard{};
    const auto model = builder()->build();

    const auto fromTime = std::chrono::milliseconds(dxfcpp::now()) + 1h;
    model->setFromTime(fromTime);

    checkOneFromTimeWarning();
    CHECK(model->getFromTime() == fromTime.count());

    model->close();
}

TEST_CASE("TimeSeriesTxModel::Builder::withFromTime warns about a time in the future") {
    ErrorManagerGuard guard{};

    SUBCASE("milliseconds as a number") {
        const auto fromTime = dxfcpp::now() + HOUR;
        const auto model = builder()->withFromTime(fromTime)->build();

        checkOneFromTimeWarning();
        CHECK(model->getFromTime() == fromTime);
        model->close();
    }

    SUBCASE("std::chrono::milliseconds") {
        const auto fromTime = std::chrono::milliseconds(dxfcpp::now()) + 1h;
        const auto model = builder()->withFromTime(fromTime)->build();

        checkOneFromTimeWarning();
        CHECK(model->getFromTime() == fromTime.count());
        model->close();
    }
}

TEST_CASE("TimeSeriesTxModel: no fromTime warning for a time in the past, 0 or the maximum") {
    ErrorManagerGuard guard{};

    const std::vector<std::int64_t> times{static_cast<std::int64_t>(dxfcpp::now() - HOUR), 0, std::numeric_limits<std::int64_t>::max()};

    for (const auto fromTime : times) {
        CAPTURE(fromTime);

        const auto model = builder()->withFromTime(fromTime)->build();
        model->setFromTime(fromTime);
        model->setFromTime(std::chrono::milliseconds(fromTime));

        CHECK(warnings().empty());
        model->close();
    }

    CHECK(ErrorManager::getErrors().empty());
}
