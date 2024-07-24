// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * Source identifier for IndexedEvent.
 *
 * See IndexedEvent::getSource().
 */
class DXFCPP_EXPORT IndexedEventSource {
    std::int32_t id_{};
    std::string name_{};

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
    virtual void *toGraal() const;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    /**
     * Creates an object of the current type and fills it with data from the the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     */
    static IndexedEventSource fromGraal(void *graalNative);

    IndexedEventSource() noexcept = default;
    virtual ~IndexedEventSource() noexcept = default;

    /**
     * Creates the new IndexedEvent's source by id and name.
     *
     * @param id The source id
     * @param name The source name
     */
    IndexedEventSource(std::int32_t id, std::string name) noexcept : id_{id}, name_{std::move(name)} {
    }

    /**
     * Returns the source identifier. Source identifier is non-negative.
     *
     * @return The source identifier.
     */
    std::int32_t id() const noexcept {
        return id_;
    }

    /**
     * Returns the string representation of the object.
     *
     * @return The string representation of the object.
     */
    const std::string &name() const noexcept {
        return name_;
    }

    /**
     * Returns the string representation of the object.
     *
     * @return The string representation of the object.
     */
    std::string toString() const {
        return name_;
    }

    bool operator==(const IndexedEventSource &indexedEventSource) const {
        return id_ == indexedEventSource.id_;
    }

    auto operator<(const IndexedEventSource &indexedEventSource) const {
        return id_ < indexedEventSource.id_;
    }
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::IndexedEventSource> {
    std::size_t operator()(const dxfcpp::IndexedEventSource &indexedEventSource) const noexcept {
        return static_cast<std::size_t>(indexedEventSource.id());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()