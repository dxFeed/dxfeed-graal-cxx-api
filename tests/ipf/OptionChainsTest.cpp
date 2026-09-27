// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Reads tests/data/ipf/option-chains.ipf and builds the option chains. The expected values are those of the Java API
// (QD 3.355) for the same file: an empty STRIKE is read as 0 (so that option is in a series), the CFI of a series has
// `X` instead of the call/put letter, an option on a future is in the chains of both its product and its underlying.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TapeSupport.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace dxfcpp;

namespace {

std::vector<std::shared_ptr<InstrumentProfile>> readProfiles() {
    return InstrumentProfileReader::create()->readFromFile(test::testDataPath("ipf/option-chains.ipf"));
}

std::vector<std::string> symbols(const std::map<double, std::shared_ptr<InstrumentProfile>> &options) {
    std::vector<std::string> result{};

    for (const auto &[strike, option] : options) {
        result.push_back(option->getSymbol());
    }

    return result;
}

struct ExpectedSeries {
    std::int32_t expiration;
    double multiplier;
    double spc;
    std::string optionType;
    std::string mmy;
    std::string expirationStyle;
    std::string settlementStyle;
    std::string cfi;
    std::string javaToString; // OptionSeries.toString() of the Java API
};

void checkSeries(const OptionSeries<InstrumentProfile> &series, const ExpectedSeries &expected) {
    CHECK(series.getExpiration() == expected.expiration);
    CHECK(series.getLastTrade() == expected.expiration);
    CHECK(series.getMultiplier() == expected.multiplier);
    CHECK(series.getSPC() == expected.spc);
    CHECK(series.getOptionType() == expected.optionType);
    CHECK(series.getMMY() == expected.mmy);
    CHECK(series.getExpirationStyle() == expected.expirationStyle);
    CHECK(series.getSettlementStyle() == expected.settlementStyle);
    CHECK(series.getCFI() == expected.cfi);
}

const ExpectedSeries TEST_NOV17_MINI{19'678, 10, 10, "STAN", "", "Regular", "Close", "OXASPS",
                                     "expiration=20231117, lastTrade=20231117, multiplier=10.0, spc=10.0, "
                                     "optionType=STAN, expirationStyle=Regular, settlementStyle=Close, cfi=OXASPS"};
const ExpectedSeries TEST_NOV17{19'678, 100, 100, "STAN", "", "Regular", "Close", "OXASPS",
                                "expiration=20231117, lastTrade=20231117, multiplier=100.0, spc=100.0, "
                                "optionType=STAN, expirationStyle=Regular, settlementStyle=Close, cfi=OXASPS"};
const ExpectedSeries TEST_NOV24_WEEKLY{19'685, 100, 100, "WEEKLY", "", "Regular", "Close", "OXASPS",
                                       "expiration=20231124, lastTrade=20231124, multiplier=100.0, spc=100.0, "
                                       "optionType=WEEKLY, expirationStyle=Regular, settlementStyle=Close, cfi=OXASPS"};
const ExpectedSeries TEST_DEC15{19'706, 100, 100, "STAN", "", "Regular", "Close", "OXASPS",
                                "expiration=20231215, lastTrade=20231215, multiplier=100.0, spc=100.0, "
                                "optionType=STAN, expirationStyle=Regular, settlementStyle=Close, cfi=OXASPS"};
const ExpectedSeries FUTURE_DEC15{19'706, 50, 1, "", "202312", "American", "Future", "OXAFPS",
                                  "expiration=20231215, lastTrade=20231215, multiplier=50.0, spc=1.0, mmy=202312, "
                                  "expirationStyle=American, settlementStyle=Future, cfi=OXAFPS"};

} // namespace

TEST_CASE("InstrumentProfileReader reads all the profiles of an IPF file") {
    const auto profiles = readProfiles();

    REQUIRE(profiles.size() == 14);

    CHECK(profiles[0]->getType() == "STOCK");
    CHECK(profiles[0]->getSymbol() == "TEST");
    CHECK(profiles[0]->getCFI() == "ESXXXX");

    CHECK(profiles[1]->getType() == "FUTURE");
    CHECK(profiles[1]->getSymbol() == "/TSTZ23");
    CHECK(profiles[1]->getProduct() == "/TST");
    CHECK(profiles[1]->getExpiration() == 19'706); // 2023-12-15

    CHECK(profiles[2]->getType() == "OPTION");
    CHECK(profiles[2]->getSymbol() == ".TEST231117C100");
    CHECK(profiles[2]->getUnderlying() == "TEST");
    CHECK(profiles[2]->getExpiration() == 19'678); // 2023-11-17
    CHECK(profiles[2]->getLastTrade() == 19'678);
    CHECK(profiles[2]->getStrike() == 100.0);
    CHECK(profiles[2]->getMultiplier() == 100.0);
    CHECK(profiles[2]->getSPC() == 100.0);
    CHECK(profiles[2]->getOptionType() == "STAN");
    CHECK(profiles[2]->getExpirationStyle() == "Regular");
    CHECK(profiles[2]->getSettlementStyle() == "Close");
    CHECK(profiles[2]->getCFI() == "OCASPS");

    CHECK(profiles[10]->getSymbol() == "./TSTZ23C4500");
    CHECK(profiles[10]->getProduct() == "/TST");
    CHECK(profiles[10]->getMMY() == "202312");

    CHECK(profiles[12]->getSymbol() == ".TESTNOSTRIKE");
    CHECK(profiles[12]->getStrike() == 0.0); // an empty numeric field is 0
}

TEST_CASE("OptionChainsBuilder builds the chains of the IPF file as the Java API does") {
    auto builder = OptionChainsBuilder<InstrumentProfile>::build(readProfiles());
    const auto &chains = builder.getChains();

    REQUIRE(chains.size() == 3);
    REQUIRE(chains.count("TEST") == 1);
    REQUIRE(chains.count("/TST") == 1);    // the product of the options on the future
    REQUIRE(chains.count("/TSTZ23") == 1); // their underlying

    SUBCASE("The chain of the stock has a series per expiration, option type, multiplier and SPC") {
        auto chain = chains.at("TEST");
        const auto series = chain.getSeries();

        REQUIRE(series.size() == 4);

        auto it = series.begin();

        checkSeries(*it, TEST_NOV17_MINI);
        CHECK(it->getStrikes() == std::vector<double>{100.0});
        CHECK(symbols(it->getCalls()) == std::vector<std::string>{".TEST7231117C100"});
        CHECK(it->getPuts().empty());

        ++it;
        checkSeries(*it, TEST_NOV17);
        CHECK(it->getStrikes() == std::vector<double>{0.0, 100.0, 110.0});
        CHECK(symbols(it->getCalls()) ==
              std::vector<std::string>{".TESTNOSTRIKE", ".TEST231117C100", ".TEST231117C110"});
        CHECK(symbols(it->getPuts()) == std::vector<std::string>{".TEST231117P100", ".TEST231117P110"});

        ++it;
        checkSeries(*it, TEST_NOV24_WEEKLY);
        CHECK(symbols(it->getCalls()) == std::vector<std::string>{".TEST231124C100"});
        CHECK(it->getPuts().empty());

        ++it;
        checkSeries(*it, TEST_DEC15);
        CHECK(it->getStrikes() == std::vector<double>{95.0, 105.0});
        CHECK(symbols(it->getCalls()) == std::vector<std::string>{".TEST231215C105"});
        CHECK(symbols(it->getPuts()) == std::vector<std::string>{".TEST231215P95"});
    }

    SUBCASE("The options on a future are in the chains of both the product and the underlying") {
        for (const std::string name : {"/TST", "/TSTZ23"}) {
            CAPTURE(name);

            auto chain = chains.at(name);
            const auto series = chain.getSeries();

            REQUIRE(series.size() == 1);
            checkSeries(*series.begin(), FUTURE_DEC15);
            CHECK(series.begin()->getStrikes() == std::vector<double>{4400.0, 4500.0});
            CHECK(symbols(series.begin()->getCalls()) == std::vector<std::string>{"./TSTZ23C4500"});
            CHECK(symbols(series.begin()->getPuts()) == std::vector<std::string>{"./TSTZ23P4400"});
        }
    }

    SUBCASE("An instrument whose CFI is not an option CFI is not in any chain") {
        for (const auto &[name, chain] : chains) {
            auto copy = chain;

            for (const auto &series : copy.getSeries()) {
                for (const auto *options : {&series.getCalls(), &series.getPuts()}) {
                    for (const auto &[strike, option] : *options) {
                        CHECK(option->getSymbol() != ".TESTNOTOPTION");
                    }
                }
            }
        }
    }
}

TEST_CASE("OptionSeries::toString() matches the Java API (EVT-17)" * doctest::may_fail()) {
    auto builder = OptionChainsBuilder<InstrumentProfile>::build(readProfiles());
    auto chain = builder.getChains().at("TEST");
    const auto series = chain.getSeries();
    std::vector<std::string> actual{};

    for (const auto &s : series) {
        actual.push_back(s.toString());
    }

    CHECK(actual == std::vector<std::string>{TEST_NOV17_MINI.javaToString, TEST_NOV17.javaToString,
                                             TEST_NOV24_WEEKLY.javaToString, TEST_DEC15.javaToString});
}
