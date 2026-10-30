// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"

#include <string>
#include <vector>

/**
 * @page isolate_arguments Isolate arguments
 *
 * The library runs the Java code of the dxFeed Graal SDK in a Graal isolate, which it creates by the first call that
 * needs Java. The isolate takes arguments as a native executable takes its command line: the sizes of the heap, the
 * system properties that must be set before the first Java code runs (System::setProperty() works later), the options
 * of the runtime. There are two ways to give them, and both must be used before the isolate is created:
 *
 * - System::setIsolateArguments() in the code of the application;
 * - the `DXFCPP_ISOLATE_ARGUMENTS` environment variable, which needs no change of the code (also for the C API).
 *
 * **Order.** The arguments of the variable go after the arguments of the code. When an option is repeated, the last
 * value wins for `-D` and `-XX:`, so the variable overrides the code. When the isolate is created, the library sets the
 * properties of the `DXFEED_*` environment variables and of the `dxfeed.system.properties` file as system properties:
 * they override a `-D` argument with the same name.
 *
 * **Static initializers.** A static initializer of the application that uses the library creates the isolate before
 * `main()`: a call of System::setIsolateArguments() from `main()` then returns `false`. Such uses are, for example,
 * `TimePeriod::ZERO.getTime()`, `CFI::EMPTY()->getCode()`, DXEndpoint::create() and System::getProperty(). The setter
 * itself may be called from a static initializer (it does not create the isolate), but the order of the static
 * initializers of different files is not defined, so in such an application use the variable.
 *
 * **The variable** is split into arguments as `JDK_JAVA_OPTIONS` of the JDK is:
 *
 * - the whitespace characters (space, `\t`, `\n`, `\v`, `\f`, `\r`) separate the arguments, whatever the locale;
 * - a part in `'...'` or in `"..."` keeps its whitespace characters, and the quotes are removed: `-Dmy.prop="a b"`
 *   gives `-Dmy.prop=a b`. The quote of the other kind inside the part is an ordinary character;
 * - there are no escape sequences: the backslash is an ordinary character, so `-Dmy.dir=C:\data` is passed as is;
 * - an unset, empty or whitespace-only variable gives no arguments;
 * - an unmatched quote, and an argument that starts with `@` (an argument file), are errors: the creation of the
 *   isolate fails (see below), and the message names the variable and the reason, not the value.
 *
 * The same value in the shells:
 *
 * ```sh
 * # bash, zsh
 * export DXFCPP_ISOLATE_ARGUMENTS='-Dmy.prop="a b" -Xmx512m'
 * ```
 *
 * ```powershell
 * # PowerShell
 * $env:DXFCPP_ISOLATE_ARGUMENTS = '-Dmy.prop="a b" -Xmx512m'
 * ```
 *
 * ```bat
 * rem cmd.exe: the quotes stay in the value
 * set DXFCPP_ISOLATE_ARGUMENTS=-Dmy.prop="a b" -Xmx512m
 * ```
 *
 * On Windows the variable is read in the ANSI code page of the system. A non-ASCII value reaches the isolate intact
 * only when the system code page is UTF-8; otherwise the characters outside the code page may be replaced. Use ASCII
 * in the variable, or System::setIsolateArguments(): it passes the UTF-8 strings as they are.
 *
 * **Useful arguments** (checked with the dxFeed Graal SDK 3.11.0, which ignores an argument that does not start with
 * `-`):
 *
 * - `-Xmx<size>`, `-Xms<size>`, `-Xmn<size>`: the maximum, the initial and the young generation size of the heap, for
 *   example `-Xmx512m`, `-Xmx2g`;
 * - `-D<name>=<value>`: a system property that is set before the first Java code runs;
 * - `-XX:-EnableSignalHandling`: Linux and macOS, the isolate does not install its handlers of `SIGPIPE` and
 *   `SIGXFSZ`.
 *
 * **Signals (Linux and macOS).** By default, when the action of `SIGPIPE` or `SIGXFSZ` is the default one at the
 * creation of the isolate, the SDK installs a handler that does nothing, so a write to a closed socket by a connector
 * of an endpoint fails with `EPIPE` instead of killing the process. A handler or `SIG_IGN` that the application set
 * before the first call is kept. With `-XX:-EnableSignalHandling` the SDK installs no handlers: set `SIG_IGN` for
 * `SIGPIPE` yourself before the first call, otherwise a write to a closed socket kills the process.
 *
 * **Errors.** An argument that the SDK does not accept (for example `-Xmx64q` or an unknown `-XX:` option) makes it
 * print the reason to the standard error output, and the creation of the isolate fails with the code 22
 * (`ARGUMENT_PARSING_FAILED`). The library does not try again. The calls that need Java throw GraalException with the
 * code and the reason, the C API functions return `DXFC_EC_ERROR`: see @ref error_handling "Error handling". The values
 * of the arguments are never in the messages.
 */

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

    /**
     * Sets the arguments that the Graal isolate is created with, for example `-Dmy.prop=value` or `-Xmx512m`. Call it
     * before the first call of the library that needs Java; it does not create the isolate itself. A repeated call
     * replaces the arguments, an empty vector clears them. The `DXFCPP_ISOLATE_ARGUMENTS` environment variable adds
     * arguments after these. The arguments, the variable and the errors: @ref isolate_arguments "Isolate arguments".
     *
     * @param arguments The isolate arguments (UTF-8 strings).
     * @return `true` if the arguments are stored; `false` if the creation of the isolate has already started, then
     * nothing changes.
     * @throws InvalidArgumentException if an argument contains a NUL character; the stored arguments do not change.
     */
    static bool setIsolateArguments(std::vector<std::string> arguments);
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