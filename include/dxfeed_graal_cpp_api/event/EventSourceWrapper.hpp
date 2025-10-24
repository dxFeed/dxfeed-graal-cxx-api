// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../internal/Common.hpp"
#include "IndexedEventSource.hpp"
#include "market/OrderSource.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>

DXFCPP_BEGIN_NAMESPACE

/**
 * A concept describing a event source that can be wrapped.
 *
 * @tparam T Probable event source type
 */
template <typename T>
concept ConvertibleToEventSourceWrapper =
    std::is_same_v<std::decay_t<T>, OrderSource> || std::is_same_v<std::decay_t<T>, IndexedEventSource>;

/**
 * A concept that defines a collection of wrapped or wrapping event sources.
 *
 * @tparam Collection The collection type
 */
template <typename Collection>
concept ConvertibleToEventSourceWrapperCollection =
    requires(Collection c) {
        std::begin(c);
        std::end(c);
    } &&
    (
        requires(Collection c) {
            { *std::begin(c) } -> dxfcpp::ConvertibleTo<EventSourceWrapper>;
        } ||
        requires(Collection c) {
            { *std::begin(c) } -> ConvertibleToEventSourceWrapper;
        });

/**
 * A helper wrapper class needed to pass heterogeneous event sources using a container and convert them to internal
 * Graal representation.
 */
struct DXFCPP_EXPORT EventSourceWrapper final {
    using DataType = std::variant<IndexedEventSource, OrderSource>;

    class DXFCPP_EXPORT ListUtils final {
        static std::ptrdiff_t calculateGraalListSize(std::ptrdiff_t initSize) noexcept;
        static void *newGraalList(std::ptrdiff_t size);
        static bool setGraalListElement(void *graalList, std::ptrdiff_t elementIdx, void *element) noexcept;
        static bool freeGraalListElements(void *graalList, std::ptrdiff_t count);

        public:
        template <typename EventSourceIt> static void *toGraalList(EventSourceIt begin, EventSourceIt end) {
            auto size = calculateGraalListSize(std::distance(begin, end));
            // Zero size is needed, for example, to clear the list of event sources.
            auto *list = newGraalList(size);

            if (!list || size == 0) {
                return list;
            }

            std::ptrdiff_t elementIdx = 0;
            bool needToFree = false;

            for (auto it = begin; it != end && elementIdx < size; it++, elementIdx++) {
                if constexpr (requires { it->toGraal(); }) {
                    needToFree = setGraalListElement(list, elementIdx, it->toGraal()) == false;
                } else if constexpr (std::is_convertible_v<decltype(*it), EventSourceWrapper> ||
                                     dxfcpp::ConvertibleToEventSourceWrapper<decltype(*it)>) {
                    needToFree = setGraalListElement(list, elementIdx, EventSourceWrapper(*it).toGraal()) == false;
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

        template <ConvertibleToEventSourceWrapperCollection EventSourcesCollection>
        static void *toGraalList(const EventSourcesCollection &collection) {
            return ListUtils::toGraalList(std::begin(collection), std::end(collection));
        }

        static void *toGraalList(std::initializer_list<EventSourceWrapper> collection) {
            return ListUtils::toGraalList(collection.begin(), collection.end());
        }

        static void freeGraalList(void *graalList);

        template <typename EventSourceIt>
        static std::unique_ptr<void, decltype(&freeGraalList)> toGraalListUnique(EventSourceIt begin,
                                                                                 EventSourceIt end) {
            return {toGraalList(begin, end), freeGraalList};
        }

        template <ConvertibleToEventSourceWrapperCollection EventSourcesCollection>
        static std::unique_ptr<void, decltype(&freeGraalList)>
        toGraalListUnique(const EventSourcesCollection &collection) {
            return {toGraalList(collection), freeGraalList};
        }

        static std::unique_ptr<void, decltype(&freeGraalList)>
        toGraalListUnique(std::initializer_list<EventSourceWrapper> collection) {
            return {toGraalList(collection), freeGraalList};
        }

        static std::vector<EventSourceWrapper> fromGraalList(void *graalList);
    };

    private:
    DataType data_;

    public:
    EventSourceWrapper(const EventSourceWrapper &) noexcept = default;
    EventSourceWrapper(EventSourceWrapper &&) noexcept = default;
    EventSourceWrapper &operator=(const EventSourceWrapper &) noexcept = default;
    EventSourceWrapper &operator=(EventSourceWrapper &&) noexcept = default;
    EventSourceWrapper() noexcept = default;
    ~EventSourceWrapper() noexcept = default;

    /**
     * Constructs a wrapper from IndexedEventSource.
     * @param data The IndexedEventSource.
     */
    EventSourceWrapper(const IndexedEventSource &data) noexcept { // NOLINT(*-explicit-constructor)
        data_ = data;
    }

    /**
     * Constructs a wrapper from OrderSource.
     * @param data The OrderSource.
     */
    EventSourceWrapper(const OrderSource &data) noexcept { // NOLINT(*-explicit-constructor)
        data_ = data;
    }

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative) {
        IndexedEventSource::freeGraal(graalNative);
    }

    /**
     * Creates an object of the current type and fills it with data from the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     * @throws RuntimeException if event source type is unknown
     */
    static EventSourceWrapper fromGraal(void *graalNative);

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    [[nodiscard]] void *toGraal() const noexcept {
        return std::visit(
            [](const auto &eventSource) {
                return eventSource.toGraal();
            },
            data_);
    }

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The smart unique pointer to the filled dxFeed Graal SDK structure
     */
    std::unique_ptr<void, decltype(&EventSourceWrapper::freeGraal)>
    toGraalUnique() const noexcept { // NOLINT(*-use-nodiscard)
        return {toGraal(), freeGraal};
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const {
        return "EventSourceWrapper{" +
               std::visit(
                   [](const auto &eventSource) {
                       return toStringAny(eventSource);
                   },
                   data_) +
               "}";
    }

    /**
     * Returns a string representation of the underlying object.
     *
     * @return a string representation of the underlying object.
     */
    std::string toStringUnderlying() const {
        return std::visit(
            [](const auto &eventSource) {
                return toStringAny(eventSource);
            },
            data_);
    }

    /**
     * @return `true` if current EventSourceWrapper holds a IndexedEventSource.
     */
    bool isIndexedEventSource() const noexcept {
        return std::holds_alternative<IndexedEventSource>(data_);
    }

    /**
     * @return `true` if current EventSourceWrapper holds a OrderSource.
     */
    bool isOrderSource() const noexcept {
        return std::holds_alternative<OrderSource>(data_);
    }

    /**
     * @return IndexedEventSource (optional) or std::nullopt if current EventSourceWrapper doesn't hold
     * IndexedEventSource.
     */
    std::optional<IndexedEventSource> asIndexedEventSource() const noexcept {
        return isIndexedEventSource() ? std::make_optional<IndexedEventSource>(std::get<IndexedEventSource>(data_))
                                      : std::nullopt;
    }

    /**
     * @return OrderSource (optional) or std::nullopt if current EventSourceWrapper doesn't hold
     * OrderSource.
     */
    std::optional<OrderSource> asOrderSource() const noexcept {
        return isOrderSource() ? std::make_optional<OrderSource>(std::get<OrderSource>(data_)) : std::nullopt;
    }

    /**
     * @return The internal data.
     */
    const DataType &getData() const noexcept {
        return data_;
    }

    bool operator==(const EventSourceWrapper &eventSourceWrapper) const {
        return getData() == eventSourceWrapper.getData();
    }

    auto operator<(const EventSourceWrapper &eventSourceWrapper) const {
        return getData() < eventSourceWrapper.getData();
    }

    using GraalPtr = std::unique_ptr<void, decltype(&EventSourceWrapper::freeGraal)>;
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::EventSourceWrapper> {
    std::size_t operator()(const dxfcpp::EventSourceWrapper &eventSourceWrapper) const noexcept {
        return std::hash<dxfcpp::EventSourceWrapper::DataType>{}(eventSourceWrapper.getData());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()