// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <memory>

#include "Entity.hpp"

namespace dxfcpp {

/// Base abstract "shared entity" class. Has some helpers for dynamic polymorphism
struct DXFCPP_EXPORT SharedEntity : public Entity, std::enable_shared_from_this<SharedEntity> {
    /// The simple type synonym for the SharedEntity type
    using Ptr = std::shared_ptr<SharedEntity>;

    /**
     * Checks that pointer to the current type could be converted to type T*
     * In other words: whether type T belongs to the type hierarchy in which the current type resides.
     * @tparam T The type to check
     * @return true if type belongs to the type hierarchy in which the current type resides.
     */
    template <typename T> bool is() const noexcept {
        try {
            auto p = dynamic_cast<const T *>(this);
            (void)(p);

            return true;
        } catch (const std::bad_cast &) {
            return false;
        }
    }

    /**
     * Returns a pointer to the current object wrapped in a smart pointer to type T
     *
     * @tparam T The type to convert to a pointer to
     * @return a smart pointer to type T
     */
    template <typename T> std::shared_ptr<T> sharedAs() noexcept {
        return std::dynamic_pointer_cast<T>(shared_from_this());
    }

    /**
     * Returns a pointer to the current object wrapped in a smart pointer to type T
     *
     * @tparam T The type to convert to a pointer to
     * @return a smart pointer to type T
     */
    template <typename T> std::shared_ptr<T> sharedAs() const noexcept {
        return std::dynamic_pointer_cast<T>(shared_from_this());
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    virtual std::string toString() const noexcept { return "SharedEntity{}"; }
};

} // namespace dxfcpp