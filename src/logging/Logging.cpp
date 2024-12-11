// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/logging/Logging.hpp>
#include <dxfeed_graal_cpp_api/system/System.hpp>

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp>
#include <dxfeed_graal_cpp_api/isolated/logging/IsolatedLogging.hpp>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

std::string Logging::levelToString(Level level) {
    switch (level) {
    case Level::ALL:
        return "ALL";
    case Level::TRACE:
        return "TRACE";
    case Level::DEBUG:
        return "DEBUG";
    case Level::INFO:
        return "INFO";
    case Level::WARN:
        return "WARN";
    case Level::ERROR:
        return "ERROR";
    case Level::OFF:
        return "OFF";
    }

    return "UNKNOWN";
}

SimpleHandler<void(Logging::Level, std::int64_t /*timestamp*/, const std::string & /*threadName*/,
                   std::int64_t /*threadId*/, const std::string & /*loggerName*/, const std::string & /*message*/)>
    Logging::handler_{};

struct Logging::Impl {
    static JavaObjectHandle<Logging::ListenerTag> listenerHandle;

    static void onLog(graal_isolatethread_t * /*thread*/, dxfg_logging_level_t level, int64_t timestamp,
                      const char *threadName, int64_t threadId, const char *loggerName, const char *message,
                      dxfg_exception_t * /*exception*/, const char * /*formattedMessage*/, void * /*userData*/) {
        handler_(static_cast<Level>(level), timestamp, threadName, threadId, loggerName, message);
    }
};

JavaObjectHandle<Logging::ListenerTag> Logging::Impl::listenerHandle{};

void Logging::init(Level level) {
    System::setProperty("log.className", "com.devexperts.logging.InterceptableLogging");
    System::setProperty("log.level", levelToString(level));
    System::setProperty("err.level", levelToString(level));
}

void Logging::init(const std::string &logFile, Level logLevel) {
    System::setProperty("log.className", "com.devexperts.logging.InterceptableLogging");
    System::setProperty("log.file", logFile);
    System::setProperty("log.level", levelToString(logLevel));
}

void Logging::init(const std::string &logFile, const std::string &errFile, Level logLevel, Level errFileLevel) {
    System::setProperty("log.className", "com.devexperts.logging.InterceptableLogging");
    System::setProperty("log.file", logFile);
    System::setProperty("log.level", levelToString(logLevel));
    System::setProperty("err.file", errFile);
    System::setProperty("err.level", levelToString(errFileLevel));
}

void Logging::setListener(std::function<void(Level, std::int64_t, const std::string &, std::int64_t,
                                             const std::string &, const std::string &)>
                              listener) {
    static auto id = static_cast<std::size_t>(-1);

    if (id != static_cast<std::size_t>(-1)) {
        handler_ -= id;
    }

    id = handler_ += std::move(listener);

    if (!Impl::listenerHandle) {
        Impl::listenerHandle =
            isolated::logging::IsolatedLogging::createListener(dxfcpp::bit_cast<void *>(&Impl::onLog), nullptr);
    }
}

void Logging::setLogLevel(Level level) {
    isolated::logging::IsolatedLogging::setLogLevel(level);
}

void Logging::setErrFileLevel(Level level) {
    isolated::logging::IsolatedLogging::setErrFileLevel(level);
}

DXFCPP_END_NAMESPACE