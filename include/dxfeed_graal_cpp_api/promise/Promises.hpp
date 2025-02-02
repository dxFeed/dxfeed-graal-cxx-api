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
 * Utility methods to manipulate @ref Promise "promises".
 */
struct DXFCPP_EXPORT Promises {
  private:
    static std::shared_ptr<Promise<void>> allOfImpl(const std::vector<void *> &handles);

  public:
    /**
     * Returns a new promise that completes when all promises from the given collection complete normally or
     * exceptionally. The results of the given promises are not reflected in the returned promise, but may be obtained
     * by inspecting them individually. If no promises are provided, returns a promise completed with the value `void`.
     * When the resulting promise completes for any reason (is canceled, for example)
     * then all of the promises from the given array are canceled.
     *
     * @tparam Collection The collection type. For example: PromiseList<LastingEvent> (i.e.
     * std::vector<Promise<std::shared_ptr<LastingEvent>>>) or
     * std::vector<std::shared_ptr<Promise<std::shared_ptr<LastingEvent>>>>.
     * @param collection The collection of promises or collection of pointer-like of promises
     * @return A new promise that completes when all promises from the given array complete.
     */
    template <typename Collection> static std::shared_ptr<Promise<void>> allOf(Collection &&collection) {
        std::vector<void *> handles{};

        handles.reserve(std::size(collection));

        for (const auto &e : collection) {
            if constexpr (requires { e->impl.handle; }) { // std::shared_ptr<Promise<std::shared_ptr<Event>>
                handles.emplace_back(e->impl.handle);
            } else if constexpr (requires {
                                     e.impl.handle;
                                 }) { // Promise<std::shared_ptr<Event> (PromiseList<Event> etc)
                handles.emplace_back(e.impl.handle);
            }
        }

        handles.shrink_to_fit();

        return allOfImpl(handles);
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()