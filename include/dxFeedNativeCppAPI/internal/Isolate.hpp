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

namespace dxfcpp {

namespace detail {
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

            if constexpr (isDebugIsolates) {
                debug("IsolateThread{{{}, isMain = {}, tid = {}, idx = {}}}()", bit_cast<std::size_t>(handle), isMain,
                      tid, idx);
            }
        }

        CEntryPointErrors detach() {
            if constexpr (isDebugIsolates) {
                debug("{}::detach()", toString());
            }

            // OK if nothing is attached.
            if (!handle) {
                if constexpr (isDebugIsolates) {
                    debug("\tNot attached");
                }

                return CEntryPointErrors::NO_ERROR;
            }

            auto result = CEntryPointErrors::valueOf(graal_detach_thread(handle));

            if (result == CEntryPointErrors::NO_ERROR) {
                if constexpr (isDebugIsolates) {
                    debug("\tDetached");
                }

                handle = nullptr;
            }

            return result;
        }

        CEntryPointErrors detachAllThreadsAndTearDownIsolate() {
            if constexpr (isDebugIsolates) {
                debug("{}::detachAllThreadsAndTearDownIsolate()", toString());
            }

            if (!handle) {
                if constexpr (isDebugIsolates) {
                    debug("\tNot attached");
                }

                return CEntryPointErrors::NO_ERROR;
            }

            auto result = CEntryPointErrors::valueOf(graal_detach_all_threads_and_tear_down_isolate(handle));

            if (result == CEntryPointErrors::NO_ERROR) {
                if constexpr (isDebugIsolates) {
                    debug("\tAll threads have been detached. The isolate has been teared down.");
                }

                handle = nullptr;
            }

            return result;
        }

        ~IsolateThread() {
            if constexpr (isDebugIsolates) {
                debug("~{}()", toString());
            }

            if (isMain) {
                if constexpr (isDebugIsolates) {
                    debug("\tThis is the main thread");
                }

                return;
            }

            detach();
        }

        std::string toString() const {
            return vformat("IsolateThread{{{}, isMain = {}, tid = {}, idx = {}}}", bit_cast<std::size_t>(handle),
                               isMain, tid, idx);
        }
    };

    mutable std::recursive_mutex mutex_{};

    GraalIsolateHandle handle_;
    IsolateThread mainIsolateThread_;
    static thread_local IsolateThread currentIsolateThread_;

    Isolate(GraalIsolateHandle handle, GraalIsolateThreadHandle mainIsolateThreadHandle)
        : handle_{handle}, mainIsolateThread_{mainIsolateThreadHandle, true} {

        currentIsolateThread_.handle = mainIsolateThreadHandle;
        currentIsolateThread_.isMain = true;

        if constexpr (isDebugIsolates) {
            debug("Isolate{{{}, main = {}, current = {}}}()", bit_cast<std::size_t>(handle),
                       mainIsolateThread_.toString(), currentIsolateThread_.toString());
        }
    }

    static std::shared_ptr<Isolate> create() {
        if constexpr (isDebugIsolates) {
            debug("Isolate::create()");
        }

        GraalIsolateHandle graalIsolateHandle{};
        GraalIsolateThreadHandle graalIsolateThreadHandle{};

        if (CEntryPointErrors::valueOf(graal_create_isolate(nullptr, &graalIsolateHandle, &graalIsolateThreadHandle)) ==
            CEntryPointErrors::NO_ERROR) {

            auto result = std::shared_ptr<Isolate>{new Isolate{graalIsolateHandle, graalIsolateThreadHandle}};

            if constexpr (isDebugIsolates) {
                debug("Isolate::create() -> *{}", result->toString());
            }

            return result;
        }

        if constexpr (isDebugIsolates) {
            debug("\t-> nullptr");
        }

        return nullptr;
    }

    CEntryPointErrors attach() {
        if constexpr (isDebugIsolates) {
            debug("{}::attach()", toString());
        }

        // We will not re-attach.
        if (!currentIsolateThread_.handle) {
            if constexpr (isDebugIsolates) {
                debug("\tNeeds to be attached.");
            }

            GraalIsolateThreadHandle newIsolateThreadHandle{};

            if (auto result = CEntryPointErrors::valueOf(graal_attach_thread(handle_, &newIsolateThreadHandle));
                result != CEntryPointErrors::NO_ERROR) {

                if constexpr (isDebugIsolates) {
                    debug("\t-> {}", result.getDescription());
                }

                return result;
            }

            currentIsolateThread_.handle = newIsolateThreadHandle;
            currentIsolateThread_.isMain = mainIsolateThread_.handle == newIsolateThreadHandle;

            if constexpr (isDebugIsolates) {
                debug("\tAttached: {}", currentIsolateThread_.toString());
            }
        } else {
            if constexpr (isDebugIsolates) {
                debug("\tCached: {}", currentIsolateThread_.toString());
            }
        }

        return CEntryPointErrors::NO_ERROR;
    }

  public:
    Isolate() = delete;
    Isolate(const Isolate &) = delete;
    Isolate &operator=(const Isolate &) = delete;

    static std::shared_ptr<Isolate> getInstance() {
        if constexpr (isDebugIsolates) {
            debug("Isolate::getInstance()");
        }

        static std::shared_ptr<Isolate> instance = create();

        if constexpr (isDebugIsolates) {
            debug("Isolate::getInstance() -> *{}", instance->toString());
        }

        return instance;
    }

    template <typename F>
    auto runIsolated(F &&f) -> std::variant<CEntryPointErrors, std::invoke_result_t<F &&, GraalIsolateThreadHandle>> {
        std::lock_guard lock(mutex_);
        if constexpr (isDebugIsolates) {
            debug("{}::runIsolated({})", toString(), bit_cast<std::size_t>(&f));
        }

        if (auto result = attach(); result != CEntryPointErrors::NO_ERROR) {
            if constexpr (isDebugIsolates) {
                debug("\t-> {}", result.getDescription());
            }

            return result;
        }

        return std::invoke(std::forward<F>(f), currentIsolateThread_.handle);
    }

    template <typename F, typename R>
        requires std::convertible_to<R, std::invoke_result_t<F &&, GraalIsolateThreadHandle>>
    auto runIsolatedOrElse(F &&f, R defaultValue) {
        return std::visit(
            [defaultValue =
                 std::move(defaultValue)]<typename T>(T &&arg) -> std::invoke_result_t<F &&, GraalIsolateThreadHandle> {
                if constexpr (std::is_same_v<T, detail::CEntryPointErrors>) {
                    return defaultValue;
                } else {
                    return arg;
                }
            },
            detail::Isolate::getInstance()->runIsolated(std::forward<F>(f)));
    }

    ~Isolate() {
        std::lock_guard lock(mutex_);

        if constexpr (isDebugIsolates) {
            debug("~{}()", toString());
        }

        mainIsolateThread_.detachAllThreadsAndTearDownIsolate();
    }

    std::string toString() const {
        std::lock_guard lock(mutex_);

        return vformat("Isolate{{{}, main = {}, current = {}}}", bit_cast<std::size_t>(handle_),
                           mainIsolateThread_.toString(), currentIsolateThread_.toString());
    }
};
} // namespace detail

} // namespace dxfcpp