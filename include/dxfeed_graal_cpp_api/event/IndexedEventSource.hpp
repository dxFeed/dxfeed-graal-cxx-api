// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"
#include "../internal/utils/ConstString.hpp"
#include "../internal/utils/StringUtils.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

/**
 * \addtogroup dxfcpp_event
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct EventSourceWrapper;

/**
 * Source identifier for IndexedEvent.
 *
 * See IndexedEvent::getSource().
 */
class DXFCPP_EXPORT IndexedEventSource {
    friend struct EventSourceWrapper;

    std::int32_t id_{};
    ConstString name_{};
    // Set by OrderSource: toGraal() writes the type of an order source then. The flag replaces the virtual functions,
    // so the class is not polymorphic and its constants do not change when they are destroyed at exit. A copy to this
    // class does not copy the flag, as a copy of an order source to this class did not keep its virtual functions.
    bool orderSource_{};

    protected:
    /**
     * Creates a constant source in constant expressions.
     *
     * @param id The identifier.
     * @param name The name with static storage duration.
     * @param orderSource `true` for an OrderSource.
     */
    constexpr IndexedEventSource(std::int32_t id, ConstString::Literal name, bool orderSource = false) noexcept
        : id_{id}, name_{name}, orderSource_{orderSource} {
    }

    /// Creates a source of an OrderSource by id and name.
    IndexedEventSource(std::int32_t id, const StringLike &name, bool orderSource) noexcept;

    /// Creates a copy of the source for an OrderSource.
    IndexedEventSource(const IndexedEventSource &other, bool orderSource) noexcept;

    /// @return The name without a copy.
    [[nodiscard]] constexpr std::string_view nameView() const noexcept {
        return name_.view();
    }

    public:
    /**
     * The default source with zero @ref ::id() "identifier" for all events that do not support multiple sources.
     */
    static const IndexedEventSource DEFAULT;

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    std::unique_ptr<void, decltype(&IndexedEventSource::freeGraal)> toGraalUnique() const noexcept;

    /**
     * Creates an object of the current type and fills it with data from the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of the current type.
     * @throws InvalidArgumentException
     */
    static IndexedEventSource fromGraal(void *graalNative);

    IndexedEventSource() noexcept;

    /**
     * Creates the new IndexedEvent's source by id and name.
     *
     * @param id The source id
     * @param name The source name
     */
    IndexedEventSource(std::int32_t id, const StringLike &name) noexcept;

    /**
     * Creates a copy of the source. A copy of an OrderSource is a source of the IndexedEventSource type.
     *
     * @param other The source to copy.
     */
    IndexedEventSource(const IndexedEventSource &other) noexcept;

    /**
     * Copies the identifier and the name of the source; the type of this source is kept.
     *
     * @param other The source to copy.
     * @return This source.
     */
    IndexedEventSource &operator=(const IndexedEventSource &other) noexcept;

    /**
     * Returns the source identifier. Source identifier is non-negative.
     *
     * @return The source identifier.
     */
    std::int32_t id() const noexcept;

    /**
     * Returns the string representation of the object.
     *
     * @return The string representation of the object.
     */
    std::string name() const;

    /**
     * Returns the string representation of the object.
     *
     * @return The string representation of the object.
     */
    std::string toString() const;

    bool operator==(const IndexedEventSource &indexedEventSource) const;

    bool operator<(const IndexedEventSource &indexedEventSource) const;
};

DXFCPP_END_NAMESPACE

/// @}

template <> struct std::hash<dxfcpp::IndexedEventSource> {
    std::size_t operator()(const dxfcpp::IndexedEventSource &indexedEventSource) const noexcept {
        return static_cast<std::size_t>(indexedEventSource.id());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()