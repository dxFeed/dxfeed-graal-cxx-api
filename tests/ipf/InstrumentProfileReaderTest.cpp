// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TapeSupport.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace dxfcpp;

namespace {

void checkOptionChainsProfiles(const std::vector<std::shared_ptr<InstrumentProfile>> &profiles) {
    REQUIRE(profiles.size() == 14);
    CHECK(profiles[0]->getType() == "STOCK");
    CHECK(profiles[0]->getSymbol() == "TEST");
}

} // namespace

TEST_CASE("InstrumentProfileReader reads a file completely and reports when it was modified") {
    const auto reader = InstrumentProfileReader::create();

    CHECK_FALSE(reader->wasComplete());

    checkOptionChainsProfiles(reader->readFromFile(test::testDataPath("ipf/option-chains.ipf")));

    CHECK(reader->wasComplete());
    CHECK(reader->getLastModified() > 0);
}

TEST_CASE("InstrumentProfileReader reads a file with a user and a password or with a token") {
    const auto reader = InstrumentProfileReader::create();
    const auto path = test::testDataPath("ipf/option-chains.ipf");

    SUBCASE("User and password") {
        checkOptionChainsProfiles(reader->readFromFile(path, "user", "password"));
    }

    SUBCASE("Token") {
        checkOptionChainsProfiles(reader->readFromFile(path, AuthToken::createBasicToken("user", "password")));
    }

    CHECK(reader->wasComplete());
}

TEST_CASE("InstrumentProfileReader reports a missing file as JavaException") {
    const auto reader = InstrumentProfileReader::create();

    CHECK_THROWS_AS(reader->readFromFile(test::testDataPath("ipf/no-such-file.ipf")), JavaException);
}

TEST_CASE("InstrumentProfileReader::resolveSourceURL() turns host:port into the URL of the profiles") {
    CHECK(InstrumentProfileReader::resolveSourceURL("localhost:7777") == "http://localhost:7777/ipf/all.ipf.gz");
    CHECK(InstrumentProfileReader::resolveSourceURL("data/profiles.ipf") == "data/profiles.ipf");
}
