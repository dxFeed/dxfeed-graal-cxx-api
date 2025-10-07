// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <doctest/doctest.h>

#include <config/config.hpp>
#include <map>
#include <string>
#include <utility>

#if defined(_WIN32)
#  include <windows.h>
#  define setenv(name, value, overwrite) _putenv_s(name, value)
#  define unsetenv(name) _putenv_s(name, "")
#else
#  include <cstdlib>
extern "C" {
int setenv(const char*, const char*, int);
int unsetenv(const char*);
}
#endif

bool setEnvironmentVariable(const std::string& name, const std::string& value) {
  return setenv(name.c_str(), value.c_str(), 1) == 0;
}

bool unsetEnvironmentVariable(const std::string& name) {
  return unsetenv(name.c_str()) == 0;
}

using namespace org::ttldtor::config;

template <typename M1, typename M2>
bool equals(M1&& m1, M2&& m2) {
  for (auto& [key, value] : m1) {
    if (m2.find(key) == m2.end()) {
      return false;
    }

    if (m2.at(key) != value) {
      return false;
    }
  }

  for (auto& [key, value] : m2) {
    if (m1.find(key) == m1.end()) {
      return false;
    }

    if (m1.at(key) != value) {
      return false;
    }
  }

  return true;
}

TEST_CASE("Config") {
  SUBCASE("Adding keys and values.") {
    std::map<std::string, std::string> properties{{"Key1", "Value1"}, {"Key2", "Value2"}};
    auto config = Config();

    config.addSource(properties);

    CHECK(equals(properties, config.getData()));
  }

  SUBCASE("Subsequent additions of values with the same keys overwrite the old values.") {
    std::map<std::string, std::string> properties{{"Key1", "Value1"}, {"Key2", "Value2"}};
    std::map<std::string, std::string> properties2{{"Key2", "ValueX"}, {"Key3", "Value3"}};
    auto config = Config();

    config.addSource(properties);
    config.addSource(properties2);

    CHECK(
      equals(std::unordered_map<std::string, std::string>{{"Key1", "Value1"}, {"Key2", "ValueX"}, {"Key3", "Value3"}},
             config.getData()));
  }
}

TEST_CASE("IniSource") {
  SUBCASE("Parse") {
    std::string iniString = R"("
# Connection address.

# endpoint.address=my.host.com:77
endpoint.address=localhost:8080

# Turn on support for event timestamps
endpoint.eventTime=true

# User name and password

user=demo
password=demo

# Notifies on incoming data at most on every specified time interval.
# By default, notifications are performed as soon as data arrives.
# Use this property to throttle notification rate.
#
# aggregationPeriod=0.1s

# Sets the size of thread pool, equal to the number of processors if not specified
#
# threadPoolSize=5
    ")";

    const auto s = IniSource::fromString(iniString);

    CHECK(s.getData().size() == 4);
  }
}

TEST_CASE("EnvSource") {
  SUBCASE("Load") {
    setEnvironmentVariable("ORG_TTLDTOR_CONFIG_TEST_ENV_VAR", "test value");

    const auto s = EnvSource();

    CHECK(s.getData().count("ORG_TTLDTOR_CONFIG_TEST_ENV_VAR") == 1);

    const auto s2 = EnvSource("ORG_TTLDTOR_CONFIG_");

    CHECK(s2.getData().count("ORG_TTLDTOR_CONFIG_TEST_ENV_VAR") == 0);
    CHECK(s2.getData().count("TEST_ENV_VAR") == 1);

    unsetEnvironmentVariable("ORG_TTLDTOR_CONFIG_TEST_ENV_VAR");
  }
}

TEST_CASE("Full") {
  Config config{};

  config.addSource(IniSource::fromString(R"("
user=demo
password=demo
    ")"));

  for (auto& [key, value] : config) {
    std::cout << key << " = " << value << '\n';
  }

  setEnvironmentVariable("ORG_TTLDTOR_CONFIG_user", "test");
  config.addSource(EnvSource("ORG_TTLDTOR_CONFIG_"));

  for (auto& [key, value] : config) {
    std::cout << key << " = " << value << '\n';
  }

  CHECK(config.getData().size() == 2);
  CHECK(config.getData().count("user") == 1);
  CHECK(config.getData().at("user") == "test");
}