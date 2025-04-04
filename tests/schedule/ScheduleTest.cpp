// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()

#include <doctest.h>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

decltype(ranges::views::filter([](const auto &s) {
    return !s.empty();
})) filterNonEmpty{};

decltype(ranges::views::transform([](auto &&s) {
    return s | ranges::to<std::string>();
})) transformToString{};

decltype(ranges::views::transform([](const std::string &s) {
    return trimStr(s);
})) trim{};

auto splitAndTrim = [](auto &&string, char sep = ',') noexcept {
    return string | ranges::views::split(sep) | filterNonEmpty | transformToString | trim;
};

std::shared_ptr<Schedule> gmt(const std::string &extra) {
    return Schedule::getInstance("(tz=GMT;" + extra + ")");
}

void check(const std::string &extra, const std::string &times, const std::string &types) {
    auto sessions = gmt(extra)->getDayById(42)->getSessions();
    auto timeArray = splitAndTrim(times) | ranges::to<std::vector<std::string>>();
    auto typeArray = splitAndTrim(types) | ranges::to<std::vector<std::string>>();

    REQUIRE(timeArray.size() == sessions.size());
    REQUIRE(typeArray.size() == sessions.size());

    for (std::size_t i = 0; i < sessions.size(); i++) {
        auto s = sessions[i];
        auto time = std::stoll(timeArray[i]);

        time = time / 10000 * time_util::HOUR + time / 100 % 100 * time_util::MINUTE + time % 100 * time_util::SECOND;

        REQUIRE(time == s->getStartTime() % time_util::DAY);
        REQUIRE(typeArray[i] == s->getType().getName());
    }
}

TEST_CASE("Test Parsing") {
    REQUIRE(0 == gmt("0=")->getDayById(42)->getStartTime() % time_util::DAY);
    REQUIRE(23LL * time_util::HOUR == gmt("de=2300;0=")->getDayById(42)->getStartTime() % time_util::DAY);
    REQUIRE(0 ==
            gmt("0=01000200")->getDayById(42)->getNextDay(DayFilter::NON_TRADING)->getStartTime() % time_util::DAY);
    REQUIRE(1 == gmt("0=01000200")->getDayById(42)->getNextDay(DayFilter::NON_TRADING)->getSessions().size());
    REQUIRE(time_util::HOUR ==
            gmt("0=01000200")->getNearestSessionByTime(now(), SessionFilter::TRADING)->getStartTime() % time_util::DAY);

    check("0=", "000000", "NO_TRADING");
    check("0=01000200", "000000,010000,020000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=0100/0200", "000000,010000,020000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=+-0100-+0200", "000000,010000,020000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=+-0100/-+0200", "000000,010000,020000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r01000200", "000000,010000,020000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r0100/0200", "000000,010000,020000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r+-0100-+0200", "000000,010000,020000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r+-0100/-+0200", "000000,010000,020000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=010002000300040005000600", "000000,010000,020000,030000,040000,050000,060000",
          "NO_TRADING,REGULAR,NO_TRADING,REGULAR,NO_TRADING,REGULAR,NO_TRADING");
    check("0=0100/02000300/04000500/0600", "000000,010000,020000,030000,040000,050000,060000",
          "NO_TRADING,REGULAR,NO_TRADING,REGULAR,NO_TRADING,REGULAR,NO_TRADING");
    check("0=+-0100-+0200+-0300-+0400+-0500-+0600", "000000,010000,020000,030000,040000,050000,060000",
          "NO_TRADING,REGULAR,NO_TRADING,REGULAR,NO_TRADING,REGULAR,NO_TRADING");
    check("0=+-0100/-+0200+-0300/-+0400+-0500/-+0600", "000000,010000,020000,030000,040000,050000,060000",
          "NO_TRADING,REGULAR,NO_TRADING,REGULAR,NO_TRADING,REGULAR,NO_TRADING");
    check("0=p01000200r03000400a05000600", "000000,010000,020000,030000,040000,050000,060000",
          "NO_TRADING,PRE_MARKET,NO_TRADING,REGULAR,NO_TRADING,AFTER_MARKET,NO_TRADING");
    check("0=p0100/0200r0300/0400a0500/0600", "000000,010000,020000,030000,040000,050000,060000",
          "NO_TRADING,PRE_MARKET,NO_TRADING,REGULAR,NO_TRADING,AFTER_MARKET,NO_TRADING");
    check("0=p+-0100-+0200r+-0300-+0400a+-0500-+0600", "000000,010000,020000,030000,040000,050000,060000",
          "NO_TRADING,PRE_MARKET,NO_TRADING,REGULAR,NO_TRADING,AFTER_MARKET,NO_TRADING");
    check("0=p+-0100/-+0200r+-0300/-+0400a+-0500/-+0600", "000000,010000,020000,030000,040000,050000,060000",
          "NO_TRADING,PRE_MARKET,NO_TRADING,REGULAR,NO_TRADING,AFTER_MARKET,NO_TRADING");

    check("0=010203020406", "000000,010203,020406", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=010203/020406", "000000,010203,020406", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=+-010203-+020406", "000000,010203,020406", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=+-010203/-+020406", "000000,010203,020406", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r010203020406", "000000,010203,020406", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r010203/020406", "000000,010203,020406", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r+-010203-+020406", "000000,010203,020406", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r+-010203/-+020406", "000000,010203,020406", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=p010203020406r030507040608a050709060810", "000000,010203,020406,030507,040608,050709,060810",
          "NO_TRADING,PRE_MARKET,NO_TRADING,REGULAR,NO_TRADING,AFTER_MARKET,NO_TRADING");
    check("0=p010203/020406r030507/040608a050709/060810", "000000,010203,020406,030507,040608,050709,060810",
          "NO_TRADING,PRE_MARKET,NO_TRADING,REGULAR,NO_TRADING,AFTER_MARKET,NO_TRADING");
    check("0=p+-010203-+020406r+-030507-+040608a+-050709-+060810", "000000,010203,020406,030507,040608,050709,060810",
          "NO_TRADING,PRE_MARKET,NO_TRADING,REGULAR,NO_TRADING,AFTER_MARKET,NO_TRADING");
    check("0=p+-010203/-+020406r+-030507/-+040608a+-050709/-+060810",
          "000000,010203,020406,030507,040608,050709,060810",
          "NO_TRADING,PRE_MARKET,NO_TRADING,REGULAR,NO_TRADING,AFTER_MARKET,NO_TRADING");

    check("0=rt0300r05000600", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=rt+-0300r-+0500-+0600", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r05000600rt0300", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=r+-0500/0600rt+-0300", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=05000600rt0300", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=0500/0600rt-+0300", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=rt030405r05000600", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=05000600rt030405", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
    check("0=0500/0600rt-+030405", "000000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");

    check("0=d0100230005000600rt0300", "010000,050000,060000", "NO_TRADING,REGULAR,NO_TRADING");
}

TEST_CASE("Test hd sets union with default syntax") {
    auto def = "hd=US,CA;0=08001800;td=1234567";

    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210606)->isHoliday(), "US and CA not holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210118)->isHoliday(), "US Holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210524)->isHoliday(), "CA Holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210215)->isHoliday(), "US and CA Holiday");
}

TEST_CASE("Test hd sets and days union with default syntax") {
    auto def = "hd=US,CA,20211010;0=08001800;td=1234567";

    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210606)->isHoliday(), "US and CA not holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210118)->isHoliday(), "US Holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210524)->isHoliday(), "CA Holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210215)->isHoliday(), "US and CA Holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20211010)->isHoliday(), "Custom Holiday");
}

TEST_CASE("Test hd sets difference") {
    auto def = "hd=US,-CA;0=08001800;td=1234567";

    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210606)->isHoliday(), "US and CA not holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210118)->isHoliday(), "US Only Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210524)->isHoliday(), "CA Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210215)->isHoliday(), "US and CA Holiday");
}

TEST_CASE("Test hd sets and days difference") {
    auto def = "hd=US,-CA,-20210531;0=08001800;td=1234567";

    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210606)->isHoliday(), "US and CA not holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210118)->isHoliday(), "US Only Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210524)->isHoliday(), "CA Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210215)->isHoliday(), "US and CA Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210531)->isHoliday(), "Custom removed holiday");
}

TEST_CASE("Test hd sets intersection") {
    auto def = "hd=US,*CA;0=08001800;td=1234567";

    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210606)->isHoliday(), "US and CA not holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210118)->isHoliday(), "US Only Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210524)->isHoliday(), "CA Holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210215)->isHoliday(), "US and CA Holiday");
}

TEST_CASE("Test hd sets and days intersect") {
    auto def = "hd=20210101,*US,*CA;0=08001800;td=1234567";

    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210606)->isHoliday(), "US and CA not holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210118)->isHoliday(), "US Only Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210524)->isHoliday(), "CA Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210215)->isHoliday(), "US and CA Holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210101)->isHoliday(), "Only one remain holiday");
}

TEST_CASE("Test hd sets and days intersect with wrong syntax") {
    auto def = "hd=US,CA,*20210101;0=08001800;td=1234567";

    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210606)->isHoliday(), "US and CA not holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210118)->isHoliday(), "US Only Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210524)->isHoliday(), "CA Holiday");
    REQUIRE_FALSE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210215)->isHoliday(), "US and CA Holiday");
    REQUIRE_MESSAGE(gmt(def)->getDayByYearMonthDay(20210101)->isHoliday(), "Only one remain holiday");
}