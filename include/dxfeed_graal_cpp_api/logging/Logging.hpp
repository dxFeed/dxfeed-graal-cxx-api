// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"
#include "dxfeed_graal_cpp_api/internal/Handler.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"

DXFCPP_BEGIN_NAMESPACE

/**
 *
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

    static std::string levelToString(Level level);

  private:
    static SimpleHandler<void(Level, std::int64_t /*timestamp*/, const std::string & /*threadName*/,
                              std::int64_t /*threadId*/, const std::string & /*loggerName*/,
                              const std::string & /*message*/)>
        handler_;

    struct Impl;

  public:
    /**
     *
     * @param level
     */
    static void init(Level level = Level::OFF);

    /**
     *
     * @param logFile
     * @param logLevel
     */
    static void init(const std::string &logFile, Level logLevel = Level::INFO);

    /**
     *
     * @param logFile
     * @param errFile
     * @param logLevel
     * @param errFileLevel
     */
    static void init(const std::string &logFile, const std::string &errFile, Level logLevel = Level::INFO,
                     Level errFileLevel = Level::WARN);

    /**
     *
     * @param listener
     */
    static void setListener(std::function<void(Level, std::int64_t /*timestamp*/, const std::string & /*threadName*/,
                                               std::int64_t /*threadId*/, const std::string & /*loggerName*/,
                                               const std::string & /*message*/)>
                                listener);

    /**
     *
     * @param level
     */
    static void setLogLevel(Level level);

    /**
     *
     * @param level
     */
    static void setErrFileLevel(Level level);
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()