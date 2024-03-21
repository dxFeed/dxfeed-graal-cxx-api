// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Isolate.hpp"

#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

DXFCPP_BEGIN_NAMESPACE
/**
 * A class that allows to set JVM system properties and get the values of JVM system properties.
 */
struct DXFCPP_EXPORT System {
    /**
     * Sets the JVM system property indicated by the specified key.
     *
     * @param key The name of the system property (UTF-8 string).
     * @param value The value of the system property (UTF-8 string).
     * @return true if the setting of the JVM system property succeeded.
     */
    static bool setProperty(const std::string &key, const std::string &value);

    /**
     * Sets the JVM system properties to the Properties argument.
     * @tparam Properties The properties' type (std::map, std::unordered_map etc).
     * @param properties The new system properties.
     * @return
     */
    template <typename Properties> static void setProperties(Properties &&properties) {
        for (auto &&[k, v] : properties) {
            setProperty(k, v);
        }
    }

    /**
     * Gets the system property indicated by the specified key.
     *
     * @param key The name of the system property (UTF-8 string).
     * @return The value of a JVM system property (UTF-8 string), or an empty string.
     */
    static std::string getProperty(const std::string &key);
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()