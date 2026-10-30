// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"

#include <string>

/**
 * \defgroup dxfcpp_system "System" Module
 * \ingroup dxfcpp_modules
 * @{
 */

DXFCPP_BEGIN_NAMESPACE
/**
 * A class that allows to set JVM system properties and get the values of JVM system properties.
 */
struct DXFCPP_EXPORT System {
    /**
     * Sets the JVM system property indicated by the specified key. A key or a value that is not valid UTF-8 is cut to its
     * longest valid prefix.
     *
     * @param key The name of the system property (UTF-8 string).
     * @param value The value of the system property (UTF-8 string).
     * @return true if the setting of the JVM system property succeeded.
     */
    static bool setProperty(const StringLike &key, const StringLike &value);

    /**
     * Sets the JVM system properties to the Properties argument.
     * @tparam Properties The properties' type (std::map, std::unordered_map etc).
     * @param properties The new system properties.
     */
    template <typename Properties> static void setProperties(Properties &&properties) {
        for (auto &&[k, v] : properties) {
            setProperty(k, v);
        }
    }

    /**
     * Gets the system property indicated by the specified key. A key that is not valid UTF-8 is cut to its longest valid
     * prefix.
     *
     * @param key The name of the system property (UTF-8 string).
     * @return The value of a JVM system property (UTF-8 string), or an empty string.
     */
    static std::string getProperty(const StringLike &key);
};

/**
 * Stops the library before the process exits: closes all the endpoints (including the default ones of
 * DXEndpoint::getInstance(), DXFeed::getInstance() and DXPublisher::getInstance()) and the instrument profile
 * connections, waits for the termination of the endpoints, and releases the Java objects. Calling it is optional: the
 * process exits without it too.
 *
 * The listeners are not called after shutdown() starts; the listener calls in progress complete before it returns.
 * Afterwards the library does not call Java anymore, and the functions that need Java fail: most throw GraalException;
 * the methods of the objects created before shutdown() (their Java objects are released) throw
 * InvalidArgumentException or do nothing; the functions that report a failure by the result, such as
 * System::getProperty(), return it. The objects of the library can still be destroyed. Call it when the process is
 * about to end: the threads that used the library are not detached from it afterwards.
 *
 * A concurrent call waits for the first one; a second call does nothing, as does a call after the library's exit
 * handler has run. Do not call it from a listener: it waits for the listener calls in progress.
 *
 * Note that at exit the library does not close the endpoints and the connections: the process ends their threads.
 * The library's exit handler (registered when the library is first used) stops the listener calls and waits up to
 * 2 seconds for the calls in progress on other threads, so the listeners can use the static objects created before
 * the library was first used. The static objects created later are destroyed before the handler runs: if the
 * listeners use them, close the endpoints or call shutdown() before that. With the library as a DLL built with MinGW
 * or with the static C runtime, the handler runs after the static objects of the executable are destroyed.
 */
DXFCPP_EXPORT void shutdown();

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()