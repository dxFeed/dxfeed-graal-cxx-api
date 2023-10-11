// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include "CEntryPointErrors.hpp"
#include "Common.hpp"
#include "utils/StringUtils.hpp"

#include <concepts>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

namespace dxfcpp {
using GraalIsolateHandle = void *;
using GraalIsolateThreadHandle = void *;
using ConstGraalIsolateThreadHandle = const void *;

class Isolate final {
    struct IsolateThread final {
        GraalIsolateThreadHandle handle{};
        bool isMain{};
        std::thread::id tid{};
        std::size_t idx{};

        explicit IsolateThread(GraalIsolateThreadHandle handle = nullptr, bool isMain = false) noexcept
            : handle{handle}, isMain{isMain}, tid{std::this_thread::get_id()} {
            this->idx = Id<IsolateThread>::getNext().getValue();

            if constexpr (Debugger::traceIsolates) {
                Debugger::trace("IsolateThread{" + dxfcpp::toString(dxfcpp::bit_cast<void *>(handle)) +
                                ", isMain = " + dxfcpp::toString(isMain) + ", tid = " + dxfcpp::toString(tid) +
                                ", idx = " + std::to_string(idx) + "}()");
            }
        }

        CEntryPointErrorsEnum detach() noexcept;

        CEntryPointErrorsEnum detachAllThreadsAndTearDownIsolate() noexcept;

        ~IsolateThread() noexcept {
            if constexpr (Debugger::traceIsolates) {
                Debugger::trace(toString() + "::~()");
            }

            if (isMain) {
                if constexpr (Debugger::traceIsolates) {
                    Debugger::trace(toString() + "::~(): isMain => This is the main thread");
                }

                return;
            }

            detach();
        }

        std::string toString() const {
            return std::string("IsolateThread{") + dxfcpp::toString(dxfcpp::bit_cast<void *>(handle)) +
                   ", isMain = " + dxfcpp::toString(isMain) + ", tid = " + dxfcpp::toString(tid) +
                   ", idx = " + std::to_string(idx) + "}";
        }
    };

    mutable std::recursive_mutex mtx_{};
    ConstGraalIsolateThreadHandle handle_;
    IsolateThread mainIsolateThread_;
    static thread_local IsolateThread currentIsolateThread_;

    Isolate(GraalIsolateHandle handle, GraalIsolateThreadHandle mainIsolateThreadHandle) noexcept
        : mtx_{}, handle_{handle}, mainIsolateThread_{mainIsolateThreadHandle, true} {

        currentIsolateThread_.handle = mainIsolateThreadHandle;
        currentIsolateThread_.isMain = true;

        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("Isolate{" + dxfcpp::toString(dxfcpp::bit_cast<void *>(handle)) + ", main = " +
                            mainIsolateThread_.toString() + ", current = " + currentIsolateThread_.toString() + "}()");
        }
    }

    static std::shared_ptr<Isolate> create() noexcept;

    CEntryPointErrorsEnum attach() noexcept;

    GraalIsolateThreadHandle get() noexcept;

  public:
    Isolate() = delete;
    Isolate(const Isolate &) = delete;
    Isolate &operator=(const Isolate &) = delete;

    static std::shared_ptr<Isolate> getInstance() noexcept {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("Isolate::getInstance()");
        }

        static std::shared_ptr<Isolate> instance = create();

        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("Isolate::getInstance() -> *" + instance->toString());
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
                                CEntryPointErrors::valueOf(result).getDescription());
            }

            return result;
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
                                CEntryPointErrors::valueOf(result).getDescription());
            }

            return result;
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
            Isolate::getInstance()->runIsolated(std::forward<F>(f)));
    }

    template <typename F, typename R, typename Arg, typename... Args>
#if __cpp_concepts
        requires dxfcpp::ConvertibleTo<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...>>
#endif
    auto runIsolatedOrElse(F &&f, R defaultValue, Arg &&arg, Args &&...args) {
        return std::visit(
            [defaultValue =
                 std::move(defaultValue)]<typename T>(T &&arg) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...> {
                if constexpr (std::is_same_v<T, CEntryPointErrorsEnum>) {
                    return defaultValue;
                } else {
                    return arg;
                }
            },
            Isolate::getInstance()->runIsolated(std::forward<F>(f), std::forward<Arg>(arg),
                                                std::forward<Args>(args)...));
    }

    ~Isolate() {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("~Isolate()");
        }
    }

    std::string toString() const noexcept {
        return std::string("Isolate{") + dxfcpp::toString(dxfcpp::bit_cast<void *>(handle_)) +
               ", main = " + mainIsolateThread_.toString() + ", current = " + currentIsolateThread_.toString() + "}";
    }
};

template <typename F> auto runIsolated(F &&f) {
    return Isolate::getInstance()->runIsolated(std::forward<F>(f));
}

template <typename F, typename R>
#if __cpp_concepts
    requires dxfcpp::ConvertibleTo<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle>>
#endif
auto runIsolatedOrElse(F &&f, R defaultValue) {
    return Isolate::getInstance()->runIsolatedOrElse(std::forward<F>(f), std::move(defaultValue));
}

template <typename F, typename R, typename Arg, typename... Args>
#if __cpp_concepts
    requires dxfcpp::ConvertibleTo<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle, Arg &&, Args &&...>>
#endif
auto runIsolatedOrElse(F &&f, R defaultValue, Arg &&arg, Args &&...args) {
    return Isolate::getInstance()->runIsolatedOrElse(std::forward<F>(f), std::move(defaultValue),
                                                     std::forward<Arg>(arg), std::forward<Args>(args)...);
}

} // namespace dxfcpp