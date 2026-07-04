// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"
#include "./Entity.hpp"

#include <memory>
#include <string>
#include <vector>

/**
 * \addtogroup dxfcpp_entity
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/// A base abstract "shared entity" class. Has some helpers for dynamic polymorphism
struct DXFCPP_EXPORT SharedEntity : Entity, std::enable_shared_from_this<SharedEntity> {
    /// The alias to a type of shared pointer to the SharedEntity object.
    using Ptr = std::shared_ptr<SharedEntity>;

    SharedEntity();

    ~SharedEntity() noexcept override;

    /**
     * Checks that the pointer to the current type could be converted to type T*
     * In other words: whether type T belongs to the type hierarchy in which the current type resides.
     * @tparam T The type to check
     * @return true if the type belongs to the type hierarchy in which the current type resides.
     */
    template <typename T> bool is() const noexcept {
        try {
            auto p = dynamic_cast<const T *>(this);

            ignoreUnused(p);

            return true;
        } catch (const std::bad_cast &) {
            return false;
        }
    }

    /**
     * Returns a pointer to the current object wrapped in a smart pointer to type T or std::shared_ptr<T>{nullptr}
     * if the object is not managed by std::shared_ptr.
     *
     * @tparam T The type to convert to a pointer to
     * @return a smart pointer to type T or std::shared_ptr<T>{nullptr} if the object is not managed by std::shared_ptr.
     */
    template <typename T> std::shared_ptr<T> sharedAs() noexcept {
        return std::dynamic_pointer_cast<T>(weak_from_this().lock());
    }

    /**
     * Returns a pointer to the current object wrapped in a smart pointer to type T or std::shared_ptr<T>{nullptr}
     * if the object is not managed by std::shared_ptr.
     *
     * @tparam T The type to convert to a pointer to
     * @return a smart pointer to type T or std::shared_ptr<T>{nullptr} if the object is not managed by std::shared_ptr.
     */
    template <typename T> std::shared_ptr<T> sharedAs() const noexcept {
        return std::dynamic_pointer_cast<T>(weak_from_this().lock());
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    virtual std::string toString() const {
        return "SharedEntity{}";
    }
};

DXFCXX_DISABLE_GCC_WARNINGS_PUSH("-Wvirtual-move-assign")

/**
 * A helper class needed to construct smart pointers to objects and does not allow explicit construction of objects.
 * @tparam T The object type.
 */
template <typename T> struct RequireMakeShared : virtual SharedEntity {
    protected:
    struct LockExternalConstructionTag {
        explicit LockExternalConstructionTag() {
        } // NOLINT(*-use-equals-default)
    };

    public:
    /**
     * Creates a smart pointer to an object.
     *
     * @tparam Args Types or arguments.
     * @param args The arguments.
     * @return A new smart pointer to an object.
     */
    template <typename... Args> static auto createShared(Args &&...args) {
        static_assert(std::is_convertible_v<T *, RequireMakeShared *>, "Must derive publicly from RequireMakeShared");

        return std::make_shared<T>(LockExternalConstructionTag{}, std::forward<Args>(args)...);
    }
};

DXFCXX_DISABLE_GCC_WARNINGS_POP()

/**
 * Converts a shared pointer of a SharedEntity descendant of type EBase to a shared pointer of an EBase descendant.
 *
 * @tparam EBase The SharedEntity descendant type.
 * @tparam EDerived The EBase descendant type.
 * @param source The source shared pointer.
 * @return The converted shared pointer.
 */
template <Derived<SharedEntity> EBase, Derived<EBase> EDerived>
static std::shared_ptr<EDerived> convertSharedEntity(const std::shared_ptr<EBase> &source) {
    if (!source) {
        return {};
    }

    return source->template sharedAs<EDerived>();
}

/**
 * Converts a vector of shared pointers to EBase-type SharedEntity descendants into a vector of shared pointers to EBase
 * descendants.
 *
 * @tparam EBase The SharedEntity descendant type.
 * @tparam EDerived The EBase descendant type.
 * @param source The source vector of shared pointers.
 * @return The converted vector of shared pointers.
 */
template <Derived<SharedEntity> EBase, Derived<EBase> EDerived>
static std::vector<std::shared_ptr<EDerived>> convertSharedEntities(const std::vector<std::shared_ptr<EBase>> &source) {
    std::vector<std::shared_ptr<EDerived>> result{};

    result.reserve(source.size());

    for (const auto &e : source) {
        result.emplace_back(e->template sharedAs<EDerived>());
    }

    return result;
}

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()