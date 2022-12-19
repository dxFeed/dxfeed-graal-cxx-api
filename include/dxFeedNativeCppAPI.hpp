// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_endpoint.h>
#include <dxfg_system.h>
#include <graal_isolate.h>

#include "internal/CEntryPointErrors.hpp"

#include <string>

#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace dxfcpp {

namespace detail {
using GraalIsolateHandle = std::add_pointer_t<graal_isolate_t>;
using GraalIsolateThreadHandle = std::add_pointer_t<graal_isolatethread_t>;

class Isolate final {
    class IsolateThread final {
        mutable std::mutex mutex_{};
        GraalIsolateThreadHandle graalIsolateThreadHandle_{};

      public:
        explicit IsolateThread(GraalIsolateThreadHandle handle = nullptr) noexcept
            : graalIsolateThreadHandle_{handle} {}

        CEntryPointErrors attach(const Isolate &isolate) {
            std::lock_guard<std::mutex> guard{mutex_};

            // We will not re-attach.
            if (!graalIsolateThreadHandle_) {
                return CEntryPointErrors::valueOf(
                    graal_attach_thread(isolate.getHandleImpl(), &graalIsolateThreadHandle_));
            }

            return CEntryPointErrors::NO_ERROR;
        }

        CEntryPointErrors detach() {
            std::lock_guard<std::mutex> guard{mutex_};

            // OK if nothing is attached.
            if (!graalIsolateThreadHandle_) {
                return CEntryPointErrors::NO_ERROR;
            }

            auto result = CEntryPointErrors::valueOf(graal_detach_thread(graalIsolateThreadHandle_));

            if (result == CEntryPointErrors::NO_ERROR) {
                graalIsolateThreadHandle_ = nullptr;
            }

            return result;
        }

        CEntryPointErrors detachAllThreadsAndTearDownIsolate() {
            if (!graalIsolateThreadHandle_) {
                return CEntryPointErrors::NO_ERROR;
            }

            auto result =
                CEntryPointErrors::valueOf(graal_detach_all_threads_and_tear_down_isolate(graalIsolateThreadHandle_));

            if (result == CEntryPointErrors::NO_ERROR) {
                graalIsolateThreadHandle_ = nullptr;
            }

            return result;
        }

        template <typename F> auto runIsolated(F &&f) {
            std::unique_lock lock(mutex_);

            return std::invoke(std::forward<F>(f), graalIsolateThreadHandle_);
        }

        ~IsolateThread() { detach(); }
    };

    mutable std::shared_mutex mutex_{};

    GraalIsolateHandle graalIsolateHandle_;
    IsolateThread mainIsolateThread_;
    static thread_local IsolateThread currentIsolateThread_;

    Isolate(GraalIsolateHandle graalIsolateHandle, GraalIsolateThreadHandle graalIsolateThreadHandle)
        : graalIsolateHandle_{graalIsolateHandle}, mainIsolateThread_{graalIsolateThreadHandle} {}

    static std::shared_ptr<Isolate> create() {
        GraalIsolateHandle graalIsolateHandle{};
        GraalIsolateThreadHandle graalIsolateThreadHandle{};

        if (CEntryPointErrors::valueOf(graal_create_isolate(nullptr, &graalIsolateHandle, &graalIsolateThreadHandle)) ==
            CEntryPointErrors::NO_ERROR) {
            return std::shared_ptr<Isolate>{new Isolate{graalIsolateHandle, graalIsolateThreadHandle}};
        }

        return nullptr;
    }
    //
    //    GraalIsolateHandle getHandleImpl() const { return graalIsolateHandle_; }
    //
    //    GraalIsolateThreadHandle getThreadHandleImpl() const { return graalIsolateThreadHandle_; }

  public:
    Isolate() = delete;
    Isolate(const Isolate &) = delete;
    Isolate &operator=(const Isolate &) = delete;

    static std::shared_ptr<Isolate> getInstance() {
        static std::shared_ptr<Isolate> instance = create();

        return instance;
    }

    //    GraalIsolateHandle getHandle() const {
    //        std::shared_lock lock(mutex_);
    //
    //        return getHandleImpl();
    //    }
    //
    //    GraalIsolateThreadHandle getThreadHandle() const {
    //        std::shared_lock lock(mutex_);
    //
    //        return getThreadHandleImpl();
    //    }

    //    GraalIsolateThreadHandle attachThread() const {
    //        std::unique_lock lock(mutex_);
    //
    //        // TODO: detach on
    //        GraalIsolateThreadHandle graalIsolateThreadHandle{};
    //
    //        currentIsolateThread_.attach(*this)
    //
    //        if (auto result = graal_attach_thread(getHandleImpl(), &graalIsolateThreadHandle); result == 0) {
    //            return graalIsolateThreadHandle;
    //        }
    //
    //        // TODO: Handle the situation where an isolated thread cannot be attached
    //        return graalIsolateThreadHandle;
    //    }

    template <typename F>
    auto runIsolated(F &&f)
        -> std::variant<CEntryPointErrors, decltype(currentIsolateThread_.runIsolated(std::forward<F>(f)))> {
        std::unique_lock lock(mutex_);

        auto result = currentIsolateThread_.attach(*this);

        if (result == CEntryPointErrors::NO_ERROR) {
            return currentIsolateThread_.runIsolated(std::forward<F>(f));
        }

        return result;
    }

    ~Isolate() {
        std::unique_lock lock(mutex_);

        mainIsolateThread_.detachAllThreadsAndTearDownIsolate();
    }
};

const auto I = Isolate::getInstance();
} // namespace detail

/**
 * A class that allows to set JVM system properties and get the values of JVM system properties.
 */
struct System {
    /**
     * Sets the JVM system property indicated by the specified key.
     *
     * @param key The name of the system property (UTF-8 string).
     * @param value The value of the system property (UTF-8 string).
     * @return true if the setting of the JVM system property succeeded.
     */
    static inline bool setProperty(const std::string &key, const std::string &value) {
        auto t = detail::Isolate::getInstance()->attachThread();

        return dxfg_system_set_property(t, key.c_str(), value.c_str()) == DXFC_EC_SUCCESS;
    }

    /**
     * Gets the system property indicated by the specified key.
     *
     * @param key The name of the system property (UTF-8 string).
     * @return The value of a JVM system property (UTF-8 string), or an empty string.
     */
    static inline std::string getProperty(const std::string &key) {
        return std::visit(
            [](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, detail::CEntryPointErrors>) {
                    return std::string{};
                } else {
                    return arg;
                }
            },
            detail::Isolate::getInstance()->runIsolated([key](detail::GraalIsolateThreadHandle threadHandle) {
                std::string resultString{};

                if (auto result = dxfg_system_get_property(threadHandle, key.c_str()); result != nullptr) {
                    resultString = result;
                    dxfg_system_release_property(threadHandle, result);
                }

                return resultString;
            }));
    }
};

struct DXEndpoint {
    //    class Builder {
    //        dxfg_endpoint_builder_t handle_;
    //        std::mutex mutex_;
    //
    //        explicit Builder(dxfg_endpoint_builder_t handle) : handle_{handle}, mutex_{} {}
    //
    //      public:
    //        ~Builder() {
    //            std::lock_guard<std::mutex> guard{mutex_};
    //
    //            auto t = detail::Isolate::getInstance()->attachThread();
    //
    //            dxfg_endpoint_builder_release(t, &handle_);
    //        }
    //
    //        static std::shared_ptr<Builder> create() {
    //            dxfg_endpoint_builder_t builderHandle;
    //            auto t = detail::Isolate::getInstance()->attachThread();
    //
    //            if (!dxfg_endpoint_builder_create(t, &builderHandle)) {
    //                return nullptr;
    //            }
    //
    //            return std::shared_ptr<Builder>(new Builder{builderHandle});
    //        }
    //    };
};

} // namespace dxfcpp
