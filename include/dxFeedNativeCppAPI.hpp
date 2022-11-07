// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_system.h>
#include <dxfg_utils.h>
#include <graal_isolate.h>

#include <string>

#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

namespace dxfcpp {

namespace detail {
using GraalIsolateHandle = std::add_pointer_t<graal_isolate_t>;
using GraalIsolateThreadHandle = std::add_pointer_t<graal_isolatethread_t>;

class Isolate final {
    mutable std::shared_mutex mutex_{};

    GraalIsolateHandle graalIsolateHandle_;
    GraalIsolateThreadHandle graalIsolateThreadHandle_;

    Isolate(GraalIsolateHandle graalIsolateHandle, GraalIsolateThreadHandle graalIsolateThreadHandle)
        : graalIsolateHandle_{graalIsolateHandle}, graalIsolateThreadHandle_{graalIsolateThreadHandle} {}

    static std::shared_ptr<Isolate> create() {
        GraalIsolateHandle graalIsolateHandle{};
        GraalIsolateThreadHandle graalIsolateThreadHandle{};

        if (auto result = graal_create_isolate(nullptr, &graalIsolateHandle, &graalIsolateThreadHandle); result == 0) {
            return std::shared_ptr<Isolate>{new Isolate{graalIsolateHandle, graalIsolateThreadHandle}};
        }

        return nullptr;
    }

    GraalIsolateHandle getHandleImpl() const { return graalIsolateHandle_; }

    GraalIsolateThreadHandle getThreadHandleImpl() const { return graalIsolateThreadHandle_; }

  public:
    Isolate() = delete;
    Isolate(const Isolate &) = delete;
    Isolate &operator=(const Isolate &) = delete;

    static std::shared_ptr<Isolate> getInstance() {
        static std::shared_ptr<Isolate> instance = create();

        return instance;
    }

    GraalIsolateHandle getHandle() const {
        std::shared_lock lock(mutex_);

        return getHandleImpl();
    }

    GraalIsolateThreadHandle getThreadHandle() const {
        std::shared_lock lock(mutex_);

        return getThreadHandleImpl();
    }

    GraalIsolateThreadHandle attachThread() const {
        std::unique_lock lock(mutex_);

        GraalIsolateThreadHandle graalIsolateThreadHandle{};

        if (auto result = graal_attach_thread(getHandleImpl(), &graalIsolateThreadHandle); result == 0) {
            return graalIsolateThreadHandle;
        }

        // TODO: Handle the situation where an isolated thread cannot be attached
        return graalIsolateThreadHandle;
    }

    ~Isolate() {
        std::unique_lock lock(mutex_);

        graal_detach_all_threads_and_tear_down_isolate(getThreadHandleImpl());
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
        auto t = detail::Isolate::getInstance()->attachThread();

        std::string resultString{};

        if (auto result = dxfg_system_get_property(t, key.c_str())) {
            resultString = result;
            dxfg_utils_free(t, static_cast<void *>(const_cast<char *>(result)));
        }

        return resultString;
    }
};

} // namespace dxfcpp
