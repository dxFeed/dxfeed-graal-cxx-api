// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/exceptions/ErrorManager.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/JavaException.hpp"

#include <fmt/format.h>

#include <chrono>
#include <cstdio>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace {

enum class HandlerMode { DEFAULT, CUSTOM, DISABLED };

struct State {
    std::mutex mutex{};
    std::uint64_t lastId{};
    std::size_t capacity{ErrorManager::DEFAULT_CAPACITY};
    std::deque<ErrorInfo> errors{};
    HandlerMode handlerMode{HandlerMode::DEFAULT};
    std::shared_ptr<const ErrorManager::Handler> handler{};
};

// Immortal: errors are reported from the threads of the SDK during the exit of the process, after the static objects
// are destroyed.
State &state() {
    static auto *instance = new State{};

    return *instance;
}

// Set while the handler runs on this thread: an error reported from the handler goes to the journal only.
thread_local bool inHandler = false;

struct InHandlerScope {
    InHandlerScope() noexcept {
        inHandler = true;
    }

    ~InHandlerScope() noexcept {
        inHandler = false;
    }

    InHandlerScope(const InHandlerScope &) = delete;
    InHandlerScope &operator=(const InHandlerScope &) = delete;
};

ErrorInfo createInfo(const StringLike &component, const std::source_location &location) {
    ErrorInfo info{};

    info.time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
    info.threadId = static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    info.component = std::string(std::string_view(component));

    const std::string_view file = location.file_name() == nullptr ? "" : location.file_name();
    const auto slash = file.find_last_of("/\\");

    info.location = fmt::format("{}:{}", slash == std::string_view::npos ? file : file.substr(slash + 1),
                                location.line());

    return info;
}

void dispatch(ErrorInfo info) {
    auto &s = state();
    HandlerMode handlerMode{};
    std::shared_ptr<const ErrorManager::Handler> handler{};

    {
        std::lock_guard lock{s.mutex};

        info.id = ++s.lastId;

        if (s.capacity > 0) {
            s.errors.push_back(info);

            while (s.errors.size() > s.capacity) {
                s.errors.pop_front();
            }
        }

        handlerMode = s.handlerMode;
        handler = s.handler;
    }

    if (handlerMode == HandlerMode::DISABLED || inHandler) {
        return;
    }

    InHandlerScope scope{};

    try {
        if (handlerMode == HandlerMode::DEFAULT) {
            ErrorManager::defaultHandler(info);
        } else if (handler != nullptr && *handler) {
            (*handler)(info);
        }
    } catch (...) {
        // The handler is the last stop of an error: its own exception has nowhere to go.
    }
}

void replaceHandler(HandlerMode handlerMode, std::shared_ptr<const ErrorManager::Handler> handler) {
    auto &s = state();

    {
        std::lock_guard lock{s.mutex};

        s.handlerMode = handlerMode;
        std::swap(s.handler, handler);
    }

    // The old handler is destroyed here, out of the lock: its captures may report errors.
}

} // namespace

std::string ErrorInfo::toString() const {
    auto result = fmt::format("#{} {} ({}): {}{}", id, component, location,
                              javaClassName.empty() ? "" : javaClassName + ": ", message);

    for (auto &c : result) {
        if (c == '\n' || c == '\r') {
            c = ' ';
        }
    }

    return result;
}

void ErrorManager::report(const StringLike &component, const StringLike &message,
                          const std::source_location &location) noexcept {
    try {
        auto info = createInfo(component, location);

        info.message = std::string(std::string_view(message));
        dispatch(std::move(info));
    } catch (...) {
        // Reporting must not fail the code that reports: the error is lost.
    }
}

void ErrorManager::reportException(const StringLike &component, const std::exception_ptr &exception,
                                   const std::source_location &location) noexcept {
    try {
        auto info = createInfo(component, location);

        if (exception == nullptr) {
            info.message = "unknown exception";
        } else {
            try {
                std::rethrow_exception(exception);
            } catch (const JavaException &e) {
                info.message = e.getJavaMessage();
                info.javaClassName = e.getJavaClassName();
                info.javaStackTrace = e.getJavaStackTrace();
            } catch (const std::exception &e) {
                info.message = e.what();
            } catch (...) {
                info.message = "unknown exception";
            }
        }

        dispatch(std::move(info));
    } catch (...) {
        // Reporting must not fail the code that reports: the error is lost.
    }
}

void ErrorManager::setHandler(Handler handler) {
    if (!handler) {
        replaceHandler(HandlerMode::DISABLED, nullptr);

        return;
    }

    replaceHandler(HandlerMode::CUSTOM, std::make_shared<const Handler>(std::move(handler)));
}

void ErrorManager::resetHandler() noexcept {
    try {
        replaceHandler(HandlerMode::DEFAULT, nullptr);
    } catch (...) {
        // Only the lock can fail here; the handler stays as it was.
    }
}

void ErrorManager::defaultHandler(const ErrorInfo &error) noexcept {
    try {
        const auto line = fmt::format("[dxfcpp] {}\n", error.toString());

        std::fputs(line.c_str(), stderr);
        std::fflush(stderr);
    } catch (...) {
        // Nowhere to write.
    }
}

void ErrorManager::setCapacity(std::size_t capacity) {
    auto &s = state();
    std::deque<ErrorInfo> dropped{};

    {
        std::lock_guard lock{s.mutex};

        s.capacity = capacity;

        while (s.errors.size() > capacity) {
            dropped.push_back(std::move(s.errors.front()));
            s.errors.pop_front();
        }
    }
}

std::size_t ErrorManager::getCapacity() noexcept {
    try {
        auto &s = state();
        std::lock_guard lock{s.mutex};

        return s.capacity;
    } catch (...) {
        return 0;
    }
}

std::vector<ErrorInfo> ErrorManager::getErrors() {
    auto &s = state();
    std::lock_guard lock{s.mutex};

    return {s.errors.begin(), s.errors.end()};
}

std::optional<ErrorInfo> ErrorManager::getLastError() {
    auto &s = state();
    std::lock_guard lock{s.mutex};

    if (s.errors.empty()) {
        return std::nullopt;
    }

    return s.errors.back();
}

void ErrorManager::clearErrors() noexcept {
    try {
        auto &s = state();
        std::deque<ErrorInfo> cleared{};

        {
            std::lock_guard lock{s.mutex};

            std::swap(s.errors, cleared);
        }
    } catch (...) {
        // Only the lock can fail here; the journal stays as it was.
    }
}

DXFCPP_END_NAMESPACE
