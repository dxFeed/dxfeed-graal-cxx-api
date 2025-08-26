// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"
#include "Promise.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <chrono>
#include <memory>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

/**
 * @class Promises
 *
 * @brief A class that represents a promise-based implementation often used for
 * handling asynchronous operations.
 *
 * This class provides a framework for handling asynchronous tasks by
 * encapsulating the eventual success (resolution) or failure (rejection)
 * of an operation. It enables chaining of asynchronous operations using
 * methods like then, catch, and finally.
 *
 * The Promises class is designed to streamline the processing of
 * asynchronous workflows, reducing callback nesting and improving
 * readability and maintainability of asynchronous code.
 *
 * The functionality includes:
 * - Resolving and rejecting a promise.
 * - Attaching callbacks to handle resolved or rejected states.
 * - Chaining operations to execute subsequent tasks in sequence.
 *
 * The Promises class follows the native promise behavior in programming
 * languages like JavaScript to facilitate deferred computation and eventual
 * state handling.
 */
struct DXFCPP_EXPORT Promises {
    private:
    static std::shared_ptr<Promise<void>> allOfImpl(const std::vector<void *> &handles);

    public:
    /**
     * Returns a new promise that completes when all promises from the given collection complete normally or
     * exceptionally. The results of the given promises are not reflected in the returned promise, but may be obtained
     * by inspecting them individually. If no promises are provided, returns a promise completed with the value `void`.
     * When the resulting promise completes for any reason (is canceled, for example),
     * then all the promises from the given array are canceled.
     *
     * @tparam Collection The collection type. For example, PromiseList<LastingEvent> (i.e.,
     * std::vector<Promise<std::shared_ptr<LastingEvent>>>) or
     * std::vector<std::shared_ptr<Promise<std::shared_ptr<LastingEvent>>>>.
     * @param collection The collection of promises or collection of pointer-likes of promises
     * @return A new promise that completes when all promises from the given array complete.
     */
    template <typename Collection> static std::shared_ptr<Promise<void>> allOf(const Collection &collection) {
        using ElemRef = decltype(*begin(collection));
        using Elem = std::remove_cvref_t<ElemRef>;

        // Determine whether elements are pointer-like or value-like exactly once.
        constexpr bool hasPtrHandle = requires(const Elem &x) { x->impl.handle; };
        constexpr bool hasValHandle = requires(const Elem &x) { x.impl.handle; };

        static_assert(hasPtrHandle || hasValHandle,
                      "Promises::allOf: Collection element must be Promise-like with impl.handle accessible via "
                      "either e->impl.handle (pointer-like) or e.impl.handle (value-like).");

        std::vector<void *> handles{};
        handles.reserve(std::size(collection));

        if constexpr (hasPtrHandle) {
            for (const auto &e : collection) {
                handles.emplace_back(e->impl.handle);
            }
        } else { // hasValHandle
            for (const auto &e : collection) {
                handles.emplace_back(e.impl.handle);
            }
        }

        return allOfImpl(handles);
    }

    /**
     * Returns a new promise that completes when all promises from the given collection complete normally or
     * exceptionally. The results of the given promises are not reflected in the returned promise, but may be obtained
     * by inspecting them individually. If no promises are provided, returns a promise completed with the value `void`.
     * When the resulting promise completes for any reason (is canceled, for example),
     * then all the promises from the given array are canceled.
     *
     * @tparam Collection The collection type. For example, PromiseList<LastingEvent> (i.e.,
     * std::vector<Promise<std::shared_ptr<LastingEvent>>>) or
     * std::vector<std::shared_ptr<Promise<std::shared_ptr<LastingEvent>>>>.
     * @param collection The smart pointer to the collection of promises or collection of pointer-likes of promises
     * @return A new promise that completes when all promises from the given array complete.
     */
    template <typename Collection> static std::shared_ptr<Promise<void>> allOf(std::shared_ptr<Collection> collection) {
        return allOf(*collection);
    }

    /**
     * Returns a new promise that completes when all promises from the given collection complete normally or
     * exceptionally. The results of the given promises are not reflected in the returned promise, but may be obtained
     * by inspecting them individually. If no promises are provided, returns a promise completed with the value `void`.
     * When the resulting promise completes for any reason (is canceled, for example),
     * then all the promises from the given array are canceled.
     *
     * @tparam Collection The collection type. For example, PromiseList<LastingEvent> (i.e.
     * std::vector<Promise<std::shared_ptr<LastingEvent>>>) or
     * std::vector<std::shared_ptr<Promise<std::shared_ptr<LastingEvent>>>>.
     * @param collection The smart pointer to the collection of promises or collection of pointer-likes of promises
     * @return A new promise that completes when all promises from the given array complete.
     */
    template <typename Collection> static std::shared_ptr<Promise<void>> allOf(std::unique_ptr<Collection> collection) {
        return allOf(*collection);
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()