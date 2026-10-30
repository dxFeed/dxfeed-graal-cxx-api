// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../exceptions/GraalException.hpp"
#include "./CEntryPointErrors.hpp"
#include "./Common.hpp"
#include "./Id.hpp"
#include "./utils/StringUtils.hpp"

#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

using GraalIsolateHandle = void *;
using GraalIsolateThreadHandle = void *;
using ConstGraalIsolateThreadHandle = const void *;

/// It's a wrapper around the Graal VM Isolate. It allows calling published Graal (Java) methods in isolation.
class DXFCPP_EXPORT Isolate final {
    /// It's a wrapper around the Graal VM Isolate's thread.
    struct DXFCPP_EXPORT IsolateThread final {
        GraalIsolateThreadHandle handle{};
        std::thread::id tid{};
        std::size_t idx{};
        bool detachOnDestruction{};

        explicit IsolateThread(GraalIsolateThreadHandle threadHandle = nullptr) noexcept;

        IsolateThread(const IsolateThread &) = delete;
        IsolateThread(IsolateThread &&) noexcept;

        IsolateThread &operator=(const IsolateThread &) = delete;
        IsolateThread &operator=(IsolateThread &&) noexcept;

        CEntryPointErrorsEnum detach() noexcept;


        ~IsolateThread() noexcept;

        std::string toString() const;
    };

    mutable std::recursive_mutex mtx_{};
    ConstGraalIsolateThreadHandle handle_{};
    static IsolateThread &currentIsolateThread() noexcept;

    Isolate() noexcept;

    CEntryPointErrorsEnum attach() const noexcept;

    GraalIsolateThreadHandle get() const noexcept;

    static void init(Isolate &isolate) noexcept;

    public:
    Isolate(const Isolate &) = delete;
    Isolate &operator=(const Isolate &) = delete;

    /**
     * Returns the isolate, creating it on the first call. The isolate is never torn down (the tear-down waits for
     * every Java thread, including the threads of open endpoints and live instrument profile connections): it lives
     * until the process ends, and the OS ends its threads.
     */
    static Isolate &getInstance() noexcept;

    /**
     * Returns `true` if the Graal isolate was created: the first call into Graal (Java) has happened. It does not
     * create the isolate.
     *
     * @return `true` if the Graal isolate was created or is being created.
     */
    static bool isCreated() noexcept;

    /**
     * Stores the arguments that the isolate is created with (see System::setIsolateArguments()). It does not create
     * the isolate; it is safe to call from a static initializer and concurrently with the creation.
     *
     * @param arguments The new arguments; an empty vector clears them.
     * @return `false` if the creation of the isolate has started (nothing changes), `true` otherwise.
     * @throws InvalidArgumentException if an argument contains a NUL character (nothing changes).
     */
    static bool setArguments(std::vector<std::string> arguments);

    /**
     * Returns `true` when the library is shutting down: dxfcpp::shutdown() was called, or the process is exiting.
     * The Graal callbacks of the library return at once then, without calling the listeners.
     */
    static bool isShuttingDown() noexcept;

    /**
     * A Graal callback of the library in progress. A callback creates it first and returns at once if it converts to
     * `false` (the library is shutting down). The exit handler and dxfcpp::shutdown() wait for the callbacks in
     * progress on the other threads.
     */
    class DXFCPP_EXPORT CallbackScope {
        bool active_;

      public:
        CallbackScope() noexcept;
        ~CallbackScope() noexcept;

        CallbackScope(const CallbackScope &) = delete;
        CallbackScope &operator=(const CallbackScope &) = delete;

        /// Returns `false` when the library is shutting down: the callback must return at once.
        explicit operator bool() const noexcept;
    };

    /**
     * @return `true` if a Graal callback of the library is in progress on the current thread (see CallbackScope).
     */
    static bool isInCallback() noexcept;

    /**
     * Returns `true` when the library is shut down: dxfcpp::shutdown() has completed, or the process is exiting.
     * The isolate is not entered anymore: runIsolated() and runIsolatedOrElse() return
     * CEntryPointErrorsEnum::SHUT_DOWN (the fallback value), runIsolatedThrow() throws it as a GraalException.
     */
    static bool isShutDown() noexcept;

    /// See dxfcpp::shutdown().
    static void shutdown();

    template <typename F>
    auto runIsolated(F &&f)
        -> std::variant<CEntryPointErrorsEnum, std::invoke_result_t<F &&, GraalIsolateThreadHandle>> {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::runIsolated(" + typeid(f).name() + ")");
        }

        if (isShutDown()) {
            return CEntryPointErrorsEnum::SHUT_DOWN;
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle);
        }

        if (auto result = attach(); result != CEntryPointErrorsEnum::NO_ERROR) {
            if constexpr (Debugger::traceIsolates) {
                // ReSharper disable once CppDFAUnreachableCode
                Debugger::trace(toString() + "::runIsolated(" + typeid(f).name() +
                                "): result != CEntryPointErrorsEnum::NO_ERROR -> " +
                                CEntryPointErrorsEnumToStr(result));
            }

            // TODO: Store "silent" errors

            return result;
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread().handle);
    }

    template <typename F> auto runIsolatedThrow(F &&f) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle> {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::runIsolatedThrow(" + typeid(f).name() + ")");
        }

        if (isShutDown()) {
            throw GraalException(CEntryPointErrorsEnum::SHUT_DOWN);
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle);
        }

        if (const auto result = attach(); result != CEntryPointErrorsEnum::NO_ERROR) {
            if constexpr (Debugger::traceIsolates) {
                // ReSharper disable once CppDFAUnreachableCode
                Debugger::trace(toString() + "::runIsolatedThrow(" + typeid(f).name() +
                                "): result != CEntryPointErrorsEnum::NO_ERROR -> " +
                                CEntryPointErrorsEnumToStr(result));
            }

            throw GraalException(result);
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread().handle);
    }

    template <typename F, typename Arg, typename... Args>
    auto runIsolated(F &&f, Arg &&arg, Args &&...args)
        -> std::variant<CEntryPointErrorsEnum,
                        std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...>> {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::runIsolated(" + typeid(f).name() + ")");
        }

        if (isShutDown()) {
            return CEntryPointErrorsEnum::SHUT_DOWN;
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle, std::forward<Arg>(arg),
                               std::forward<Args>(args)...);
        }

        if (auto result = attach(); result != CEntryPointErrorsEnum::NO_ERROR) {
            if constexpr (Debugger::traceIsolates) {
                // ReSharper disable once CppDFAUnreachableCode
                Debugger::trace(toString() + "::runIsolated(" + typeid(f).name() +
                                "): result != CEntryPointErrorsEnum::NO_ERROR -> " +
                                CEntryPointErrorsEnumToStr(result));
            }

            // TODO: Store "silent" errors

            return result;
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread().handle, std::forward<Arg>(arg),
                           std::forward<Args>(args)...);
    }

    template <typename F, typename Arg, typename... Args>
    auto runIsolatedThrow(F &&f, Arg &&arg, Args &&...args)
        -> std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...> {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::runIsolatedThrow(" + typeid(f).name() + ")");
        }

        if (isShutDown()) {
            throw GraalException(CEntryPointErrorsEnum::SHUT_DOWN);
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle, std::forward<Arg>(arg),
                               std::forward<Args>(args)...);
        }

        if (const auto result = attach(); result != CEntryPointErrorsEnum::NO_ERROR) {
            if constexpr (Debugger::traceIsolates) {
                // ReSharper disable once CppDFAUnreachableCode
                Debugger::trace(toString() + "::runIsolatedThrow(" + typeid(f).name() +
                                "): result != CEntryPointErrorsEnum::NO_ERROR -> " +
                                CEntryPointErrorsEnumToStr(result));
            }

            throw GraalException(result);
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread().handle, std::forward<Arg>(arg),
                           std::forward<Args>(args)...);
    }

    template <typename F, typename R>
#if __cpp_concepts
        requires dxfcpp::ConvertibleTo<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle>>
#endif
    auto runIsolatedOrElse(F &&f, R defaultValue) {
        return std::visit(
            [dv =
                 std::move(defaultValue)]<typename T>(T &&arg) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle> {
                if constexpr (std::is_same_v<T, CEntryPointErrorsEnum>) {
                    return dv;
                } else {
                    return arg;
                }
            },
            Isolate::getInstance().runIsolated(std::forward<F>(f)));
    }

    template <typename F, typename R, typename Arg, typename... Args>
#if __cpp_concepts
        requires dxfcpp::ConvertibleTo<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...>>
#endif
    auto runIsolatedOrElse(F &&f, R defaultValue, Arg &&arg, Args &&...args) {
        return std::visit(
            [dv = std::move(defaultValue)]<typename T>(
                T &&result) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...> {
                if constexpr (std::is_same_v<T, CEntryPointErrorsEnum>) {
                    return dv;
                } else {
                    return result;
                }
            },
            Isolate::getInstance().runIsolated(std::forward<F>(f), std::forward<Arg>(arg),
                                               std::forward<Args>(args)...));
    }

    std::string toString() const;
};

template <typename F> auto runIsolated(F &&f) {
    return Isolate::getInstance().runIsolated(std::forward<F>(f));
}

template <typename F> auto runIsolatedThrow(F &&f) {
    return Isolate::getInstance().runIsolatedThrow(std::forward<F>(f));
}

template <typename F, typename Arg, typename... Args> auto runIsolatedThrow(F &&f, Arg &&arg, Args &&...args) {
    return Isolate::getInstance().runIsolatedThrow(std::forward<F>(f), std::forward<Arg>(arg),
                                                   std::forward<Args>(args)...);
}

template <typename F, typename R>
#if __cpp_concepts
    requires dxfcpp::ConvertibleTo<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle>>
#endif
auto runIsolatedOrElse(F &&f, R defaultValue) {
    return Isolate::getInstance().runIsolatedOrElse(std::forward<F>(f), std::move(defaultValue));
}

template <typename F, typename R, typename Arg, typename... Args>
#if __cpp_concepts
    requires dxfcpp::ConvertibleTo<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...>>
#endif
auto runIsolatedOrElse(F &&f, R defaultValue, Arg &&arg, Args &&...args) {
    return Isolate::getInstance().runIsolatedOrElse(std::forward<F>(f), std::move(defaultValue), std::forward<Arg>(arg),
                                                    std::forward<Args>(args)...);
}

DXFCPP_EXPORT void gc();

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
