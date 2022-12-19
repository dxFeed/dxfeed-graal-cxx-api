// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_endpoint.h>
#include <dxfg_system.h>
#include <graal_isolate.h>

#include <string>

#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>

namespace dxfcpp {

namespace detail {
using GraalIsolateHandle = std::add_pointer_t<graal_isolate_t>;
using GraalIsolateThreadHandle = std::add_pointer_t<graal_isolatethread_t>;

/**
 * Possible error codes returned by internal GraalVM functions
 *
 *  [Graal:CEntryPointErrors](https://github.com/oracle/graal/blob/96a1a66347bd4e5e00ae4e8e79812ebaf8cd5e33/substratevm/src/com.oracle.svm.core/src/com/oracle/svm/core/c/function/CEntryPointErrors.java#L43)
 */
class CEntryPointErrors {
    std::int32_t code_{};
    std::string description_{};

    CEntryPointErrors(std::int32_t code, std::string description) noexcept
        : code_{code}, description_{std::move(description)} {}

  public:
    /// 0. No error occurred.
    static const CEntryPointErrors NO_ERROR;

    /// 1. An unspecified error occurred.
    static const CEntryPointErrors UNSPECIFIED;

    /// 2. An argument was NULL (nullptr).
    static const CEntryPointErrors NULL_ARGUMENT;

    /// 4. The specified thread is not attached to the isolate.
    static const CEntryPointErrors UNATTACHED_THREAD;

    /// 5. The specified isolate is unknown.
    static const CEntryPointErrors UNINITIALIZED_ISOLATE;

    /// 6. Locating the image file failed.
    static const CEntryPointErrors LOCATE_IMAGE_FAILED;

    /// 7. Opening the located image file failed.
    static const CEntryPointErrors OPEN_IMAGE_FAILED;

    /// 8. Mapping the heap from the image file into memory failed.
    static const CEntryPointErrors MAP_HEAP_FAILED;

    /// 801. Reserving address space for the new isolate failed.
    static const CEntryPointErrors RESERVE_ADDRESS_SPACE_FAILED;

    /// 802. The image heap does not fit in the available address space.
    static const CEntryPointErrors INSUFFICIENT_ADDRESS_SPACE;

    /// 9. Setting the protection of the heap memory failed.
    static const CEntryPointErrors PROTECT_HEAP_FAILED;

    /// 10. The version of the specified isolate parameters is unsupported.
    static const CEntryPointErrors UNSUPPORTED_ISOLATE_PARAMETERS_VERSION;

    /// 11. Initialization of threading in the isolate failed.
    static const CEntryPointErrors THREADING_INITIALIZATION_FAILED;

    /// 12. Some exception is not caught.
    static const CEntryPointErrors UNCAUGHT_EXCEPTION;

    /// 13. Initialization the isolate failed.
    static const CEntryPointErrors ISOLATE_INITIALIZATION_FAILED;

    /// 14. Opening the located auxiliary image file failed.
    static const CEntryPointErrors OPEN_AUX_IMAGE_FAILED;

    /// 15. Reading the opened auxiliary image file failed.
    static const CEntryPointErrors READ_AUX_IMAGE_META_FAILED;

    /// 16. Mapping the auxiliary image file into memory failed.
    static const CEntryPointErrors MAP_AUX_IMAGE_FAILED;

    /// 17. Insufficient memory for the auxiliary image.
    static const CEntryPointErrors INSUFFICIENT_AUX_IMAGE_MEMORY;

    /// 18. Auxiliary images are not supported on this platform or edition.
    static const CEntryPointErrors AUX_IMAGE_UNSUPPORTED;

    /// 19. Releasing the isolate's address space failed.
    static const CEntryPointErrors FREE_ADDRESS_SPACE_FAILED;

    /// 20. Releasing the isolate's image heap memory failed.
    static const CEntryPointErrors FREE_IMAGE_HEAP_FAILED;

    /// 21. The auxiliary image was built from a different primary image.
    static const CEntryPointErrors AUX_IMAGE_PRIMARY_IMAGE_MISMATCH;

    /// 22. The isolate arguments could not be parsed.
    static const CEntryPointErrors ARGUMENT_PARSING_FAILED;

    /// 23. Current target does not support the following CPU features that are required by the image.
    static const CEntryPointErrors CPU_FEATURE_CHECK_FAILED;

    static const std::unordered_map<std::int32_t, CEntryPointErrors> ALL;

    static const CEntryPointErrors &valueOf(std::int32_t code) {
        if (auto found = ALL.find(code); found != ALL.end()) {
            return found->second;
        }

        return UNSPECIFIED;
    }

    /// Returns the code
    [[nodiscard]] int32_t getCode() const { return code_; }

    /// Returns the description
    [[nodiscard]] const std::string &getDescription() const { return description_; }
};

class Isolate final {
    struct IsolateThread final {
        GraalIsolateThreadHandle graalIsolateThreadHandle_;

        CEntryPointErrors attach(const Isolate &isolate) {
            return CEntryPointErrors::valueOf(graal_attach_thread(isolate.getHandleImpl(), &graalIsolateThreadHandle_));
        }
    };

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

        // TODO: detach on
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
            dxfg_system_release_property(t, result);
        }

        return resultString;
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
