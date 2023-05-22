// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <graal_isolate.h>

#include "CEntryPointErrors.hpp"
#include "Common.hpp"

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
using GraalIsolateHandle = std::add_pointer_t<graal_isolate_t>;
using GraalIsolateThreadHandle = std::add_pointer_t<graal_isolatethread_t>;

class Isolate final {
    struct IsolateThread final {
        GraalIsolateThreadHandle handle{};
        bool isMain{};
        std::thread::id tid{};
        std::size_t idx{};

        explicit IsolateThread(GraalIsolateThreadHandle handle = nullptr, bool isMain = false) noexcept
            : handle{handle}, isMain{isMain}, tid{std::this_thread::get_id()} {
            static size_t idx = 0;

            this->idx = idx++;

            if constexpr (traceIsolates) {
                trace("IsolateThread{{{}, isMain = {}, tid = {}, idx = {}}}()", bit_cast<std::size_t>(handle), isMain,
                      tid, idx);
            }
        }

        CEntryPointErrors detach() noexcept {
            if constexpr (traceIsolates) {
                trace("{}::detach()", toString());
            }

            // OK if nothing is attached.
            if (!handle) {
                if constexpr (traceIsolates) {
                    trace("\tNot attached");
                }

                return CEntryPointErrors::NO_ERROR;
            }

            auto result = CEntryPointErrors::valueOf(graal_detach_thread(handle));

            if (result == CEntryPointErrors::NO_ERROR) {
                if constexpr (traceIsolates) {
                    trace("\tDetached");
                }

                handle = nullptr;
            }

            return result;
        }

        CEntryPointErrors detachAllThreadsAndTearDownIsolate() noexcept {
            if constexpr (traceIsolates) {
                trace("{}::detachAllThreadsAndTearDownIsolate()", toString());
            }

            if (!handle) {
                if constexpr (traceIsolates) {
                    trace("\tNot attached");
                }

                return CEntryPointErrors::NO_ERROR;
            }

            auto result = CEntryPointErrors::valueOf(graal_detach_all_threads_and_tear_down_isolate(handle));

            if (result == CEntryPointErrors::NO_ERROR) {
                if constexpr (traceIsolates) {
                    trace("\tAll threads have been detached. The isolate has been teared down.");
                }

                handle = nullptr;
            }

            return result;
        }

        ~IsolateThread() noexcept {
            if constexpr (traceIsolates) {
                trace("~{}()", toString());
            }

            if (isMain) {
                if constexpr (traceIsolates) {
                    trace("\tThis is the main thread");
                }

                return;
            }

            detach();
        }

        std::string toString() const {
            return fmt::format("IsolateThread{{{}, isMain = {}, tid = {}, idx = {}}}", bit_cast<std::size_t>(handle),
                               isMain, tid, idx);
        }
    };

    mutable std::recursive_mutex mtx_{};
    const GraalIsolateHandle handle_;
    IsolateThread mainIsolateThread_;
    static thread_local IsolateThread currentIsolateThread_;

    Isolate(GraalIsolateHandle handle, GraalIsolateThreadHandle mainIsolateThreadHandle) noexcept
        : mtx_{}, handle_{handle}, mainIsolateThread_{mainIsolateThreadHandle, true} {

        currentIsolateThread_.handle = mainIsolateThreadHandle;
        currentIsolateThread_.isMain = true;

        if constexpr (traceIsolates) {
            trace("Isolate{{{}, main = {}, current = {}}}()", bit_cast<std::size_t>(handle),
                  mainIsolateThread_.toString(), currentIsolateThread_.toString());
        }
    }

    static std::shared_ptr<Isolate> create() noexcept {
        if constexpr (traceIsolates) {
            trace("Isolate::create()");
        }

        GraalIsolateHandle graalIsolateHandle{};
        GraalIsolateThreadHandle graalIsolateThreadHandle{};

        if (CEntryPointErrors::valueOf(graal_create_isolate(nullptr, &graalIsolateHandle, &graalIsolateThreadHandle)) ==
            CEntryPointErrors::NO_ERROR) {

            auto result = std::shared_ptr<Isolate>{new Isolate{graalIsolateHandle, graalIsolateThreadHandle}};

            if constexpr (traceIsolates) {
                trace("Isolate::create() -> *{}", result->toString());
            }

            return result;
        }

        if constexpr (traceIsolates) {
            trace("\t-> nullptr");
        }

        return nullptr;
    }

    CEntryPointErrors attach() noexcept {
        if constexpr (traceIsolates) {
            trace("{}::attach()", toString());
        }

        // We will not re-attach.
        if (!currentIsolateThread_.handle) {
            if constexpr (traceIsolates) {
                trace("\tNeeds to be attached.");
            }

            GraalIsolateThreadHandle newIsolateThreadHandle{};

            if (auto result = CEntryPointErrors::valueOf(graal_attach_thread(handle_, &newIsolateThreadHandle));
                result != CEntryPointErrors::NO_ERROR) {

                if constexpr (traceIsolates) {
                    trace("\t-> {}", result.getDescription());
                }

                return result;
            }

            currentIsolateThread_.handle = newIsolateThreadHandle;
            currentIsolateThread_.isMain = mainIsolateThread_.handle == newIsolateThreadHandle;

            if constexpr (traceIsolates) {
                trace("\tAttached: {}", currentIsolateThread_.toString());
            }
        } else {
            if constexpr (traceIsolates) {
                trace("\tCached: {}", currentIsolateThread_.toString());
            }
        }

        return CEntryPointErrors::NO_ERROR;
    }

    GraalIsolateThreadHandle get() noexcept {
        if constexpr (traceIsolates) {
            trace("{}::get()", toString());
        }

        return graal_get_current_thread(handle_);
    }

  public:
    Isolate() = delete;
    Isolate(const Isolate &) = delete;
    Isolate &operator=(const Isolate &) = delete;

    static std::shared_ptr<Isolate> getInstance() noexcept {
        if constexpr (traceIsolates) {
            trace("Isolate::getInstance()");
        }

        static std::shared_ptr<Isolate> instance = create();

        if constexpr (traceIsolates) {
            trace("Isolate::getInstance() -> *{}", instance->toString());
        }

        return instance;
    }

    template <typename F>
    auto runIsolated(F &&f) -> std::variant<CEntryPointErrors, std::invoke_result_t<F &&, GraalIsolateThreadHandle>> {
        if constexpr (traceIsolates) {
            trace("{}::runIsolated({})", toString(), bit_cast<std::size_t>(&f));
        }

        // Perhaps the code is already running within the GraalVM thread (for example, we are in a listener)
        if (auto currentThreadHandle = get(); currentThreadHandle != nullptr) {
            return std::invoke(std::forward<F>(f), currentThreadHandle);
        }

        if (auto result = attach(); result != CEntryPointErrors::NO_ERROR) {
            if constexpr (traceIsolates) {
                trace("\t-> {}", result.getDescription());
            }

            return result;
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread_.handle);
    }

    template <typename F, typename R>
#if __cpp_concepts
        requires std::convertible_to<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle>>
#endif
    auto runIsolatedOrElse(F &&f, R defaultValue) {
        return std::visit(
            [defaultValue =
                 std::move(defaultValue)]<typename T>(T &&arg) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle> {
                if constexpr (std::is_same_v<T, CEntryPointErrors>) {
                    return defaultValue;
                } else {
                    return arg;
                }
            },
            Isolate::getInstance()->runIsolated(std::forward<F>(f)));
    }

    ~Isolate() {
        if constexpr (traceIsolates) {
            trace("~Isolate()");
        }
    }

    std::string toString() const {
        std::lock_guard lock(mtx_);

        return fmt::format("Isolate{{{}, main = {}, current = {}}}", bit_cast<std::size_t>(handle_),
                           mainIsolateThread_.toString(), currentIsolateThread_.toString());
    }
};

template <typename F> auto runIsolated(F &&f) { return Isolate::getInstance()->runIsolated(std::forward<F>(f)); }

template <typename F, typename R>
#if __cpp_concepts
    requires std::convertible_to<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle>>
#endif
auto runIsolatedOrElse(F &&f, R defaultValue) {
    return Isolate::getInstance()->runIsolatedOrElse(std::forward<F>(f), std::move(defaultValue));
}

} // namespace dxfcpp