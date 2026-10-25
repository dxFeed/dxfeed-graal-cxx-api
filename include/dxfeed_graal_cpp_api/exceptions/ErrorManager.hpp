// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/utils/StringUtils.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <optional>
#include <source_location>
#include <string>
#include <vector>

/**
 * @page error_handling Error handling
 *
 * The library reports an error through one of two channels.
 *
 * - **A call that fails** throws an exception to its caller: JavaException when the Java code of the dxFeed Graal
 *   SDK throws, GraalException when the call cannot enter the isolate, InvalidArgumentException, RuntimeException.
 * - **An error without a caller** goes to ErrorManager. Such an error happens in the code that the library runs on
 *   its own: a callback of the dxFeed Graal SDK (events of a subscription, a state change of an endpoint or of an
 *   instrument profile connection, a change of a subscription, instrument profiles of a collector, the events of a
 *   model). An exception of that code would otherwise terminate the process, and the caller's thread could not see
 *   it anyway: the callback runs on a thread of the SDK.
 *
 * ErrorManager passes each error to the handler and keeps the last errors in a journal:
 *
 * - The default handler writes one line per error to `stderr`. ErrorManager::setHandler() replaces it, a null
 *   handler turns the output off, ErrorManager::resetHandler() restores the default one.
 * - The handler is called on the thread where the error happened, without the locks of ErrorManager held: it may
 *   read the journal and change the handler. Keep it short, it delays the thread that reported the error (for
 *   example, the delivery of the events of a subscription). An exception of the handler is ignored; an error
 *   reported from inside the handler goes to the journal only.
 * - The journal keeps the last ErrorManager::getCapacity() errors (ErrorManager::DEFAULT_CAPACITY by default):
 *   ErrorManager::getErrors(), ErrorManager::getLastError(), ErrorManager::clearErrors().
 *
 * Events of the types that the C++ API does not implement (for example, EventTypeEnum::CONFIGURATION) are skipped
 * in the event callbacks: the other events of the batch are delivered, and one error per type and batch is reported
 * with the number of the skipped events.
 *
 * ErrorManager works during the exit of the process and after dxfcpp::shutdown(): it is never destroyed and does not
 * call the SDK. The handler can be called at exit from a thread of the SDK while the static objects of the program
 * are being destroyed, as the listeners can (see dxfcpp::shutdown()).
 *
 * ```cpp
 * dxfcpp::ErrorManager::setHandler([](const dxfcpp::ErrorInfo &error) {
 *     myLogger.error(error.toString());
 * });
 * ```
 */

/**
 * \addtogroup dxfcpp_exceptions
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * An error without a caller, reported to ErrorManager. A value: copies do not change when ErrorManager gets new
 * errors.
 */
struct DXFCPP_EXPORT ErrorInfo {
    /// The number of the error, from 1, increasing in the order of the reports.
    std::uint64_t id{};

    /// The time of the report, in milliseconds since the UNIX epoch (UTC).
    std::int64_t time{};

    /// The hash of the `std::thread::id` of the thread that reported the error.
    std::uint64_t threadId{};

    /// The part of the library that reported the error, for example `DXFeedSubscription`.
    std::string component{};

    /// The source file and the line of the report, for example `DXFeedSubscription.cpp:42`.
    std::string location{};

    /// The message of the error. For a Java exception, the message of the Java exception.
    std::string message{};

    /// The class of the Java exception, or an empty string when the error is not a Java exception.
    std::string javaClassName{};

    /// The stack trace of the Java exception, or an empty string when the error is not a Java exception.
    std::string javaStackTrace{};

    /**
     * Returns a string representation of the error: the component, the location, the Java class (if any) and the
     * message, in one line.
     *
     * @return The string representation of the error.
     */
    std::string toString() const;
};

/**
 * Receives the errors that have no caller: the exceptions of the callbacks of the dxFeed Graal SDK and of the code
 * the library runs on its own. See @ref error_handling "Error handling".
 */
struct DXFCPP_EXPORT ErrorManager final {
    /// The handler of the errors. A null handler turns the output off.
    using Handler = std::function<void(const ErrorInfo &)>;

    /// The default capacity of the journal.
    static constexpr std::size_t DEFAULT_CAPACITY = 128;

    ErrorManager() = delete;

    /**
     * Reports an error: adds it to the journal and passes it to the handler.
     *
     * @param component The part of the code that reports the error.
     * @param message The message of the error.
     * @param location The source location of the report.
     */
    static void report(const StringLike &component, const StringLike &message,
                       const std::source_location &location = std::source_location::current()) noexcept;

    /**
     * Reports an exception: adds it to the journal and passes it to the handler. A JavaException gives the Java
     * class and stack trace, another `std::exception` its `what()`, any other exception the message
     * `unknown exception`.
     *
     * @param component The part of the code that reports the error.
     * @param exception The exception, usually `std::current_exception()` in a `catch` block.
     * @param location The source location of the report.
     */
    static void reportException(const StringLike &component, const std::exception_ptr &exception,
                                const std::source_location &location = std::source_location::current()) noexcept;

    /**
     * Sets the handler of the errors.
     *
     * @param handler The new handler, or a null handler to turn the output off (the journal keeps the errors).
     */
    static void setHandler(Handler handler);

    /// Restores the default handler, which writes one line per error to `stderr`.
    static void resetHandler() noexcept;

    /**
     * The default handler: writes ErrorInfo::toString() of the error as one line to `stderr`.
     *
     * @param error The error.
     */
    static void defaultHandler(const ErrorInfo &error) noexcept;

    /**
     * Sets the number of the last errors that the journal keeps. Older errors are dropped at once if there are more.
     *
     * @param capacity The capacity; 0 turns the journal off.
     */
    static void setCapacity(std::size_t capacity);

    /// @return The number of the last errors that the journal keeps.
    static std::size_t getCapacity() noexcept;

    /// @return The copies of the errors in the journal, from the oldest to the newest.
    static std::vector<ErrorInfo> getErrors();

    /// @return The copy of the newest error in the journal, or an empty optional when the journal is empty.
    static std::optional<ErrorInfo> getLastError();

    /// Removes all the errors from the journal. The numbers of the next errors continue.
    static void clearErrors() noexcept;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
