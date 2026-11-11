// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

TEST_CASE("System properties can be set, as well as get their values") {
    REQUIRE(dxfc_system_set_property("PropertyName", "123") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "123");
}

TEST_CASE("System properties can be set, as well as get their values (Unicode)") {
    REQUIRE(dxfc_system_set_property("PropertyName", "Привет") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(1024);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "Привет");
}

TEST_CASE("If a system property does not fit in the buffer, then it must be truncated.") {
    REQUIRE(dxfc_system_set_property("PropertyName", "123123123123") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "1231");
}

TEST_CASE("If a system property does not fit in the buffer, then it must be truncated (Unicode).") {
    REQUIRE(dxfc_system_set_property("PropertyName", "Привет") == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "Пр");
}

TEST_CASE("If the system property does not exist, then an empty string should be returned.") {
    std::vector<char> buffer(5);

    auto result = dxfc_system_get_property("UnknownPropertyName", buffer.data(), buffer.size());

    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()).empty());
}

TEST_CASE("Buffer pointer is NULL and buffer size is 0 - OK") {
    REQUIRE(dxfc_system_get_property("PropertyName", nullptr, 100) == DXFC_EC_SUCCESS);
    REQUIRE(dxfc_system_get_property("PropertyName", nullptr, 0) == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);
    CHECK(dxfc_system_get_property("PropertyName", buffer.data(), 0) == DXFC_EC_SUCCESS);
}

TEST_CASE("System properties can be set, as well as get their values. Multi-thread") {
    dxfc_error_code_t errorCode = DXFC_EC_ERROR;
    // TODO: move to jthread on clang
    std::thread t([&errorCode] {
        errorCode = dxfc_system_set_property("PropertyName", "123");
    });
    t.join();
    REQUIRE(errorCode == DXFC_EC_SUCCESS);

    std::vector<char> buffer(5);
    auto result = dxfc_system_get_property("PropertyName", buffer.data(), buffer.size());
    REQUIRE(result == DXFC_EC_SUCCESS);
    CHECK(std::string(buffer.data()) == "123");
}

TEST_CASE("System properties can be set, as well as get their values") {
    REQUIRE(dxfcpp::System::setProperty("PropertyName", "123") == true);
    CHECK(dxfcpp::System::getProperty("PropertyName") == "123");
}

TEST_CASE("System properties can be set, as well as get their values (Unicode)") {
    REQUIRE(dxfcpp::System::setProperty("PropertyName", "Привет") == true);
    CHECK(dxfcpp::System::getProperty("PropertyName") == "Привет");
}

TEST_CASE("If the system property does not exist, then an empty string should be returned.") {
    CHECK(dxfcpp::System::getProperty("UnknownPropertyName").empty());
}

TEST_CASE("System properties round-trip through the SDK for every kind of string argument") {
    // "Privet" in Cyrillic, written as UTF-8 bytes.
    const std::string utf8 = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";

    SUBCASE("const char *") {
        const char *value = "const-char-pointer";

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.ptr", value));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.ptr") == "const-char-pointer");
    }

    SUBCASE("std::string") {
        const std::string value = "std-string";

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.str", value));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.str") == value);
    }

    SUBCASE("std::string_view") {
        const std::string_view value{"string-view-with-tail", 11};

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.view", value));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.view") == "string-view");
    }

    SUBCASE("a literal") {
        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.literal", "literal"));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.literal") == "literal");
    }

    SUBCASE("a char buffer") {
        char buffer[16] = "buffer";

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.buffer", buffer));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.buffer") == "buffer");
    }

    SUBCASE("non-ASCII UTF-8") {
        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.utf8", utf8));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.utf8") == utf8);
    }

    SUBCASE("a null const char * value is an empty value") {
        const char *value = nullptr;

        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.null", "not empty"));
        REQUIRE(dxfcpp::System::setProperty("dxfcpp.test.sdkstring.null", value));
        CHECK(dxfcpp::System::getProperty("dxfcpp.test.sdkstring.null").empty());
    }
}

TEST_CASE("System properties can be set, as well as get their values. Multi-thread") {
    bool ok = false;
    // TODO: move to jthread on clang
    std::thread t([&ok] {
        ok = dxfcpp::System::setProperty("PropertyName", "123");
    });
    t.join();
    REQUIRE(ok == true);

    CHECK(dxfcpp::System::getProperty("PropertyName") == "123");
}

TEST_CASE("A key or a value with invalid UTF-8 throws and does not change the property") {
    using dxfcpp::InvalidArgumentException;
    using dxfcpp::System;

    SUBCASE("A value") {
        REQUIRE(System::setProperty("InvalidUtf8Value", "before"));
        CHECK_THROWS_WITH_AS(System::setProperty("InvalidUtf8Value", "abc\xe2\x28\xa1"),
                             "System::setProperty(value): invalid UTF-8 at byte 3: 0xE2 0x28",
                             InvalidArgumentException);
        CHECK(System::getProperty("InvalidUtf8Value") == "before");
    }

    SUBCASE("A key") {
        CHECK_THROWS_WITH_AS(System::setProperty("InvalidUtf8Key\xff", "value"),
                             "System::setProperty(key): invalid UTF-8 at byte 14: 0xFF", InvalidArgumentException);
        CHECK(System::getProperty("InvalidUtf8Key").empty());
        CHECK_THROWS_WITH_AS(System::getProperty("\xed\xa0\x80"),
                             "System::getProperty(key): invalid UTF-8 at byte 0: 0xED 0xA0 0x80",
                             InvalidArgumentException);
    }

    SUBCASE("The password of the endpoint is named without its bytes") {
        CHECK_THROWS_WITH_AS(System::setProperty(dxfcpp::DXEndpoint::DXFEED_PASSWORD_PROPERTY, "p\xc3\x28"),
                             "System::setProperty(value): invalid UTF-8 at byte 1", InvalidArgumentException);
    }

    SUBCASE("The C API returns an error") {
        CHECK(dxfc_system_set_property("InvalidUtf8C", "\xc0\xaf") == DXFC_EC_ERROR);
    }

    SUBCASE("A valid multibyte value") {
        REQUIRE(System::setProperty("ValidUtf8", "Значение \xF0\x9F\x98\x80"));
        CHECK(System::getProperty("ValidUtf8") == "Значение \xF0\x9F\x98\x80");
    }
}
