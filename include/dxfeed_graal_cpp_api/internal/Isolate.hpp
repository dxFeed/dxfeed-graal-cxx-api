// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../exceptions/GraalException.hpp"
#include "CEntryPointErrors.hpp"
#include "Common.hpp"
#include "utils/StringUtils.hpp"

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <variant>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

using GraalIsolateHandle = void *;
using GraalIsolateThreadHandle = void *;
using ConstGraalIsolateThreadHandle = const void *;

class Isolate final {
    struct IsolateThread final {
        GraalIsolateThreadHandle handle{};
        std::thread::id tid{};
        std::size_t idx{};

        explicit IsolateThread(GraalIsolateThreadHandle handle = nullptr) noexcept
            : handle{handle}, tid{std::this_thread::get_id()} {
            this->idx = Id<IsolateThread>::getNext().getValue();

            if constexpr (Debugger::traceIsolates) {
                Debugger::trace("IsolateThread{" + dxfcpp::toString(handle) + ", tid = " + dxfcpp::toString(tid) +
                                ", idx = " + std::to_string(idx) + "}()");
            }
        }

        IsolateThread(const IsolateThread &) = delete;
        IsolateThread(IsolateThread &&) = default;

        IsolateThread &operator=(const IsolateThread &) = delete;
        IsolateThread &operator=(IsolateThread &&) = default;

        CEntryPointErrorsEnum detach() noexcept;

        CEntryPointErrorsEnum detachAllThreadsAndTearDownIsolate() noexcept;

        ~IsolateThread() noexcept {
            if constexpr (Debugger::traceIsolates) {
                Debugger::trace(toString() + "::~()");
            }

            // detach();
        }

        std::string toString() const {
            return std::string("IsolateThread{") + dxfcpp::toString(handle) + ", tid = " + dxfcpp::toString(tid) +
                   ", idx = " + std::to_string(idx) + "}";
        }
    };

    mutable std::recursive_mutex mtx_{};
    ConstGraalIsolateThreadHandle handle_;
    static thread_local IsolateThread currentIsolateThread_;

    Isolate() noexcept;

    CEntryPointErrorsEnum attach() noexcept;

    GraalIsolateThreadHandle get() noexcept;

  public:
    Isolate(const Isolate &) = delete;
    Isolate &operator=(const Isolate &) = delete;

    static Isolate &getInstance() noexcept {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("Isolate::getInstance()");
        }

        static Isolate instance{};

        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("Isolate::getInstance() -> " + instance.toString());
        }

        return instance;
    }

    template <typename F>
    auto runIsolated(F &&f)
        -> std::variant<CEntryPointErrorsEnum, std::invoke_result_t<F &&, GraalIsolateThreadHandle>> {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::runIsolated(" + typeid(f).name() + ")");
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle);
        }

        if (auto result = attach(); result != CEntryPointErrorsEnum::NO_ERROR) {
            if constexpr (Debugger::traceIsolates) {
                Debugger::trace(toString() + "::runIsolated(" + typeid(f).name() +
                                "): result != CEntryPointErrorsEnum::NO_ERROR -> " +
                                CEntryPointErrorsEnumToStr(result));
            }

            // TODO: Store "silent" errors

            return result;
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread_.handle);
    }

    template <typename F> auto runIsolatedThrow(F &&f) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle> {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::runIsolatedThrow(" + typeid(f).name() + ")");
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle);
        }

        if (auto result = attach(); result != CEntryPointErrorsEnum::NO_ERROR) {
            if constexpr (Debugger::traceIsolates) {
                Debugger::trace(toString() + "::runIsolatedThrow(" + typeid(f).name() +
                                "): result != CEntryPointErrorsEnum::NO_ERROR -> " +
                                CEntryPointErrorsEnumToStr(result));
            }

            throw GraalException(result);
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread_.handle);
    }

    template <typename F, typename Arg, typename... Args>
    auto runIsolated(F &&f, Arg &&arg, Args &&...args)
        -> std::variant<CEntryPointErrorsEnum,
                        std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...>> {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::runIsolated(" + typeid(f).name() + ")");
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle, std::forward<Arg>(arg),
                               std::forward<Args>(args)...);
        }

        if (auto result = attach(); result != CEntryPointErrorsEnum::NO_ERROR) {
            if constexpr (Debugger::traceIsolates) {
                Debugger::trace(toString() + "::runIsolated(" + typeid(f).name() +
                                "): result != CEntryPointErrorsEnum::NO_ERROR -> " +
                                CEntryPointErrorsEnumToStr(result));
            }

            // TODO: Store "silent" errors

            return result;
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread_.handle, std::forward<Arg>(arg),
                           std::forward<Args>(args)...);
    }

    template <typename F, typename Arg, typename... Args>
    auto runIsolatedThrow(F &&f, Arg &&arg, Args &&...args)
        -> std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...> {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::runIsolatedThrow(" + typeid(f).name() + ")");
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle, std::forward<Arg>(arg),
                               std::forward<Args>(args)...);
        }

        if (auto result = attach(); result != CEntryPointErrorsEnum::NO_ERROR) {
            if constexpr (Debugger::traceIsolates) {
                Debugger::trace(toString() + "::runIsolatedThrow(" + typeid(f).name() +
                                "): result != CEntryPointErrorsEnum::NO_ERROR -> " +
                                CEntryPointErrorsEnumToStr(result));
            }

            throw GraalException(result);
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread_.handle, std::forward<Arg>(arg),
                           std::forward<Args>(args)...);
    }

    template <typename F, typename R>
#if __cpp_concepts
        requires dxfcpp::ConvertibleTo<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle>>
#endif
    auto runIsolatedOrElse(F &&f, R defaultValue) {
        return std::visit(
            [defaultValue =
                 std::move(defaultValue)]<typename T>(T &&arg) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle> {
                if constexpr (std::is_same_v<T, CEntryPointErrorsEnum>) {
                    return defaultValue;
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
            [defaultValue = std::move(defaultValue)]<typename T>(
                T &&arg) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...> {
                if constexpr (std::is_same_v<T, CEntryPointErrorsEnum>) {
                    return defaultValue;
                } else {
                    return arg;
                }
            },
            Isolate::getInstance().runIsolated(std::forward<F>(f), std::forward<Arg>(arg),
                                               std::forward<Args>(args)...));
    }

    ~Isolate() {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("~Isolate()");
        }
    }

    std::string toString() const {
        return std::string("Isolate{") + dxfcpp::toString(handle_) + ", current = " + currentIsolateThread_.toString() +
               "}";
    }
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

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()