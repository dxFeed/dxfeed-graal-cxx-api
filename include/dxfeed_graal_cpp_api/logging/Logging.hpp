// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"

DXFCPP_BEGIN_NAMESPACE

/**
 * A class of utility methods for managing the logging mechanism.
 */
struct DXFCPP_EXPORT Logging final {
    struct ListenerTag {};
    /**
     * Defines a set of standard logging levels that can be used to control logging output.
     */
    enum class Level {
        /// Indicates that all messages should be logged.
        ALL = 0,

        /// Indicates a highly detailed tracing message.
        TRACE,

        /// Is a message level providing tracing debug information.
        DEBUG,

        /// Is a message level for informational messages.
        INFO,

        /// Is a message level indicating a potential problem.
        WARN,

        /// Is a message level indicating a serious failure.
        ERROR,

        /// Is a special level that can be used to turn off logging.
        OFF,
    };

    /**
     * Converts enum Level values to their string representation.
     *
     * @param level The enum Level value.
     * @return The string representation.
     */
    static std::string levelToString(Level level);

    private:
    static SimpleHandler<void(Level, std::int64_t /*timestamp*/, const std::string & /*threadName*/,
                              std::int64_t /*threadId*/, const std::string & /*loggerName*/,
                              const std::string & /*message*/)>
        handler_;

    struct Impl;

    public:
    /**
     * Initializes logging and sets the logging level.
     *
     * This is equivalent to the following code:
     *
     * ```cpp
     * System::setProperty("log.className", "com.devexperts.logging.InterceptableLogging");
     * System::setProperty("log.level", levelToString(level));
     * System::setProperty("err.level", levelToString(level));
     * ```
     * @param level The logging level.
     */
    static void init(Level level = Level::OFF);

    /**
     * Initializes logging, sets the path to the logging file and the logging level.
     *
     * This is equivalent to the following code:
     *
     * ```cpp
     * System::setProperty("log.className", "com.devexperts.logging.InterceptableLogging");
     * System::setProperty("log.file", logFile);
     * System::setProperty("log.level", levelToString(logLevel));
     * ```
     *
     * @param logFile The logging file.
     * @param logLevel The logging level.
     */
    static void init(const StringLike &logFile, Level logLevel = Level::INFO);

    /**
     * Initializes logging, sets the path to the file for logging, to the file for outputting errors and warnings, and
     * sets the logging level for both files.
     *
     * This is equivalent to the following code:
     *
     * ```cpp
     * System::setProperty("log.className", "com.devexperts.logging.InterceptableLogging");
     * System::setProperty("log.file", logFile);
     * System::setProperty("log.level", levelToString(logLevel));
     * System::setProperty("err.file", errFile);
     * System::setProperty("err.level", levelToString(errFileLevel));
     * ```
     *
     * @param logFile The logging file.
     * @param errFile The err file.
     * @param logLevel The logging level.
     * @param errFileLevel The err file logging level.
     */
    static void init(const StringLike &logFile, const StringLike &errFile, Level logLevel = Level::INFO,
                     Level errFileLevel = Level::WARN);
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()