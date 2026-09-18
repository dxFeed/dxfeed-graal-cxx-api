// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "./utils/StringUtils.hpp"
#include "./utils/debug/Debug.hpp"

#include "../isolated/internal/IsolatedObject.hpp"

#include <atomic>
#include <memory>
#include <string>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT JavaObject {
    static std::string toString(void *handle);

    static std::size_t hashCode(void *handle) noexcept;

    static bool equals(void *objectHandle1, void *objectHandle2);
};

class JavaObjectHandleState;

/**
 * Tracks native Java object handles that must be released before the Graal isolate is torn down.
 *
 * The registry is initialized before the isolate and therefore remains available for the complete isolate lifetime,
 * including destruction of handles owned by function-local and namespace-scope static objects.
 */
class DXFCPP_EXPORT JavaObjectHandleRegistry final {
    friend class JavaObjectHandleState;

    static void registerHandle(JavaObjectHandleState &state) noexcept;
    static void resetHandle(JavaObjectHandleState &state, void *handle) noexcept;
    static void moveHandle(JavaObjectHandleState &from, JavaObjectHandleState &to) noexcept;
    static void linkHandle(JavaObjectHandleState &state) noexcept;
    static void unlinkHandle(JavaObjectHandleState &state) noexcept;

    public:
    /**
     * Returns the process-wide handle registry.
     *
     * @return The handle registry instance.
     */
    static JavaObjectHandleRegistry &getInstance() noexcept;

    /**
     * Releases all currently registered handles while the Graal isolate is still available.
     *
     * Handle owners remain valid and observe a null handle after this call.
     */
    void releaseAll() noexcept;
};

/**
 * Type-erased ownership state shared by Java object and Java object-list handle wrappers.
 *
 * A non-null state registers itself for isolate shutdown and unregisters itself when reset or destroyed.
 */
class DXFCPP_EXPORT JavaObjectHandleState final {
    friend class JavaObjectHandleRegistry;

    using Deleter = void (*)(void *) noexcept;

    std::atomic<void *> handle_{};
    Deleter deleter_{};
    JavaObjectHandleState *previous_{};
    JavaObjectHandleState *next_{};
    bool registered_{};

    public:
    /**
     * Creates an ownership state for a native handle.
     *
     * @param handle The native handle, or `nullptr` for an empty state.
     * @param deleter The function used to release a non-null handle.
     */
    JavaObjectHandleState(void *handle, Deleter deleter) noexcept;

    JavaObjectHandleState(const JavaObjectHandleState &) = delete;
    JavaObjectHandleState(JavaObjectHandleState &&) = delete;
    JavaObjectHandleState &operator=(const JavaObjectHandleState &) = delete;
    JavaObjectHandleState &operator=(JavaObjectHandleState &&) = delete;

    /** Releases the owned handle, if any, and unregisters the state. */
    ~JavaObjectHandleState() noexcept;

    /**
     * Transfers ownership from another state and releases the handle previously owned by this state.
     *
     * @param other The state from which ownership is transferred.
     */
    void moveFrom(JavaObjectHandleState &other) noexcept;

    /**
     * Replaces the owned handle, releasing the previous one.
     *
     * @param handle The new native handle, or `nullptr` to make the state empty.
     */
    void reset(void *handle = nullptr) noexcept;

    /**
     * Returns the currently owned native handle.
     *
     * @return The native handle, or `nullptr` when the state is empty.
     */
    [[nodiscard]] void *get() const noexcept {
        return handle_.load(std::memory_order_acquire);
    }
};

template <typename T> struct JavaObjectHandle final {
#if DXFCPP_DEBUG == 1
    static std::string getDebugName() {
        return typeid(JavaObjectHandle<T>).name();
    }
#endif

    using Type = T;

    static DXFCPP_EXPORT void deleter(void *handle) noexcept {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ")");
        }

        if (!handle) {
            return;
        }

        const auto result = isolated::internal::IsolatedObject::release(handle);
        ignoreUnused(result);

        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ") -> " +
                            dxfcpp::toString(result));
        }
    }

    using Impl = JavaObjectHandleState;

    explicit JavaObjectHandle(void *handle = nullptr) noexcept
        : impl_{handle, &deleter} {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "(handle = " + dxfcpp::toString(handle) + ")");
        }
    }

    JavaObjectHandle(const JavaObjectHandle &) = delete;

    JavaObjectHandle(JavaObjectHandle &&other) noexcept : impl_{nullptr, &deleter} {
        impl_.moveFrom(other.impl_);
    }

    JavaObjectHandle &operator=(const JavaObjectHandle &) = delete;

    JavaObjectHandle &operator=(JavaObjectHandle &&other) noexcept {
        impl_.moveFrom(other.impl_);

        return *this;
    }

    ~JavaObjectHandle() noexcept = default;

    [[nodiscard]] std::string toString() const {
        if (impl_.get()) {
            return dxfcpp::toString(impl_.get());
        }

        return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept {
        return impl_.get();
    }

    explicit operator bool() const noexcept {
        return impl_.get() != nullptr;
    }

    private:
    Impl impl_;
};

template <typename T> struct JavaObjectHandleList final {
#if DXFCPP_DEBUG == 1
    static auto getDebugName() {
        return std::string("JavaObjectHandleList<") + typeid(T).name() + ">";
    }
#endif

    using Type = T;

    static DXFCPP_EXPORT void deleter(void *handle) noexcept {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ")");
        }

        if (!handle) {
            return;
        }

        const auto result = isolated::internal::IsolatedObject::List::release(handle);
        ignoreUnused(result);

        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ") -> " +
                            dxfcpp::toString(result));
        }
    }

    using Impl = JavaObjectHandleState;

    explicit JavaObjectHandleList(void *handle = nullptr) noexcept
        : impl_{handle, &deleter} {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "(handle = " + dxfcpp::toString(handle) + ")");
        }
    }

    JavaObjectHandleList(const JavaObjectHandleList &) = delete;

    JavaObjectHandleList(JavaObjectHandleList &&other) noexcept : impl_{nullptr, &deleter} {
        impl_.moveFrom(other.impl_);
    }

    JavaObjectHandleList &operator=(const JavaObjectHandleList &) = delete;

    JavaObjectHandleList &operator=(JavaObjectHandleList &&other) noexcept {
        impl_.moveFrom(other.impl_);

        return *this;
    }

    ~JavaObjectHandleList() noexcept = default;

    [[nodiscard]] std::string toString() const {
        if (impl_.get()) {
            return dxfcpp::toString(impl_.get());
        }

        return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept {
        return impl_.get();
    }

    explicit operator bool() const noexcept {
        return impl_.get() != nullptr;
    }

    private:
    Impl impl_;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
