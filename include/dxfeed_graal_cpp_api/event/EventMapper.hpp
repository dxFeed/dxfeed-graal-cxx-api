// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <memory>
#include <vector>

#include "EventType.hpp"

namespace dxfcpp {

struct DXFCPP_EXPORT EventMapper {
    static std::vector<std::shared_ptr<EventType>> fromGraalList(void *graalNativeList) noexcept;

    // TODO: check that EventIt is iterator of EventType* or shared_ptr<EventType> or T* or shared_ptr<T>, where T is
    // child of EventType
    template <typename EventIt> static void *toGraalList(EventIt begin, EventIt end) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("EventMapper::toGraalList(symbols = " + elementsToString(begin, end) + ")");
        }

        auto size = calculateGraalListSize(std::distance(begin, end));

        // Zero size is needed, for example, to clear the list of symbols.
        auto *list = newGraalList(size);

        if (!list || size == 0) {
            return list;
        }

        std::ptrdiff_t elementIdx = 0;
        bool needToFree = false;

        for (auto it = begin; it != end && elementIdx < size; it++, elementIdx++) {
            if constexpr (requires { it->toGraal(); }) { // It<EventType>
                needToFree = setGraalListElement(list, elementIdx, it->toGraal()) == false;
            } else if constexpr (requires { (*it)->toGraal(); }) { // It<Ptr<EventType>>
                needToFree = setGraalListElement(list, elementIdx, (*it)->toGraal()) == false;
            }

            if (needToFree) {
                break;
            }
        }

        if (needToFree) {
            freeGraalListElements(list, elementIdx);

            return nullptr;
        }

        return list;
    }

    static void freeGraalList(void *graalList) noexcept;

  private:
    static std::ptrdiff_t calculateGraalListSize(std::ptrdiff_t initSize) noexcept;
    static void *newGraalList(std::ptrdiff_t size) noexcept;
    static bool setGraalListElement(void *graalList, std::ptrdiff_t elementIdx, void *element) noexcept;
    static bool freeGraalListElements(void *graalList, std::ptrdiff_t count) noexcept;
};

} // namespace dxfcpp