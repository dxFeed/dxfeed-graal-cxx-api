// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <cstdint>
#include <string>

namespace dxfcpp {

/**
 * Source identifier for IndexedEvent.
 *
 * See IndexedEvent::getSource().
 */
class DXFCPP_EXPORT IndexedEventSource {
    std::uint32_t id_{};
    std::string name_{};

  public:
    /**
     * The default source with zero @ref ::id() "identifier" for all events that do not support multiple sources.
     */
    static const IndexedEventSource DEFAULT;

    /**
     * Creates the new IndexedEvent's source by id and name.
     *
     * @param id The source id
     * @param name The source name
     */
    IndexedEventSource(unsigned id, std::string name) noexcept : id_{id}, name_{std::move(name)} {}

    /**
     * Returns the source identifier. Source identifier is non-negative.
     *
     * @return The source identifier.
     */
    std::uint32_t id() const noexcept { return id_; }

    /**
     * Returns the string representation of the object.
     *
     * @return The string representation of the object.
     */
    const std::string &name() const noexcept { return name_; }

    /**
     * Returns the string representation of the object.
     *
     * @return The string representation of the object.
     */
    std::string toString() const noexcept { return name_; }
};

} // namespace dxfcpp