// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "./EventType.hpp"

#include <memory>
#include <vector>

/**
 * \addtogroup dxfcpp_event
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT EventMapper {
    static std::shared_ptr<EventType> fromGraal(void *graalNativeEvent);

    // Throws on an event that cannot be converted (for example, of a type that the C++ API does not implement).
    static std::vector<std::shared_ptr<EventType>> fromGraalList(void *graalNativeList);

    // For the event callbacks of the dxFeed Graal SDK: an event that cannot be converted is skipped, and one error per
    // event type with the number of the skipped events is reported to ErrorManager.
    static std::vector<std::shared_ptr<EventType>> fromGraalListSkippingErrors(void *graalNativeList);

    static void freeGraal(void *graalNativeEvent);

    template <typename EventIt> static void *toGraalList(EventIt begin, EventIt end) {
        auto size = calculateGraalListSize(std::distance(begin, end));

        // Zero size is needed, for example, to clear the list of symbols.
        auto *list = newGraalList(size);

        if (!list || size == 0) {
            return list;
        }

        std::ptrdiff_t elementIdx = 0;
        bool needToFree = false;

        try {
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
        } catch (...) {
            // For example, a string field that is not valid UTF-8: the events converted before are freed.
            freeGraalListElements(list, elementIdx);

            throw;
        }

        if (needToFree) {
            freeGraalListElements(list, elementIdx);

            return nullptr;
        }

        return list;
    }

    static void freeGraalList(void *graalList);

    template <typename EventIt>
    static std::unique_ptr<void, decltype(&freeGraalList)> toGraalListUnique(EventIt begin, EventIt end) {
        return {toGraalList(begin, end), freeGraalList};
    }

    /**
     * Frees a list made by toGraalListInArenaUnique(): the arena of the list and the events that are not in it.
     *
     * @param graalList The list or nullptr.
     */
    static void freeGraalListInArena(void *graalList) noexcept;

    /**
     * Converts the events to the native list of the dxFeed Graal SDK, as toGraalListUnique() does, but the list, its
     * array, the structures of the events and their strings take their memory from one arena: a few heap operations per
     * list instead of a few per event. An event of a class defined outside the library (a subclass of a library event
     * included) is converted with its own toGraal() on the heap, as by toGraalListUnique(). The pointer frees the list
     * with freeGraalListInArena().
     *
     * @tparam EventIt The iterator of the events (of EventType or of a pointer to it).
     * @param begin The beginning of the events.
     * @param end The end of the events.
     * @return The list.
     * @throws InvalidArgumentException if a string field or the symbol of an event is not valid UTF-8, an event pointer
     * is null, or the conversion of an event returns null; nothing is left allocated then.
     */
    template <typename EventIt>
    static std::unique_ptr<void, decltype(&freeGraalListInArena)> toGraalListInArenaUnique(EventIt begin,
                                                                                           EventIt end) {
        struct Range {
            EventIt it;
            EventIt end;
        };

        Range range{begin, end};

        // Called once per event by the library: the next event, or nullptr for a null event pointer.
        NextEvent next = [](void *state) -> const EventType * {
            auto &r = *static_cast<Range *>(state);
            const EventType *event = nullptr;

            if constexpr (requires { r.it->toGraal(); }) { // It<EventType>
                event = &*r.it;
            } else if constexpr (requires { (*r.it)->toGraal(); }) { // It<Ptr<EventType>>
                const auto &pointer = *r.it;

                // get() of a smart pointer: operator-> of a null one may assert.
                if constexpr (requires { pointer.get(); }) {
                    event = pointer.get();
                } else {
                    event = pointer;
                }
            }

            ++r.it;

            return event;
        };

        return {newGraalListInArena(calculateGraalListSize(std::distance(begin, end)), next, &range),
                freeGraalListInArena};
    }

    private:
    using NextEvent = const EventType *(*)(void *state);

    static void *newGraalListInArena(std::ptrdiff_t size, NextEvent next, void *state);

    static std::ptrdiff_t calculateGraalListSize(std::ptrdiff_t initSize) noexcept;
    static void *newGraalList(std::ptrdiff_t size);
    static bool setGraalListElement(void *graalList, std::ptrdiff_t elementIdx, void *element) noexcept;
    static bool freeGraalListElements(void *graalList, std::ptrdiff_t count);
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()