// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "./Common.hpp"

#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

/**
 * A base of the enums that emulate Java enums.
 *
 * The elements are constant-initialized (`constinit`), so they and valueOf() can be used in the dynamic initialization
 * of global variables.
 *
 * @tparam Child The enum type: declares `static const std::span<const std::reference_wrapper<const Child>> ALL`,
 * all elements of the enum.
 * @tparam Code The type of the element code.
 */
template <typename Child, typename Code> struct Enum {
    using ParentType = Enum<Child, Code>;
    using ChildType = Child;
    using CodeType = Code;

    private:
    const CodeType code_;
    const std::string_view name_;

    protected:
    template <Integral OtherCodeType> static constexpr CodeType convertToInnerCodeType(OtherCodeType code) noexcept {
        return static_cast<Code>(static_cast<std::make_unsigned_t<Code>>(code));
    }

    /**
     * Creates an element of the enum.
     *
     * @param code The element code.
     * @param name The element name with static storage duration (a string literal).
     */
    template <Integral OtherCodeType>
    constexpr Enum(OtherCodeType code, std::string_view name) noexcept
        : code_{convertToInnerCodeType(code)}, name_{name} {
    }

    public:
    /**
     * Returns code that is used in flag bits.
     *
     * @return code.
     */
    [[nodiscard]] constexpr Code getCode() const noexcept {
        return code_;
    }

    /**
     * Returns the enum element name
     *
     * @return name
     */
    [[nodiscard]] std::string getName() const {
        return std::string{name_};
    }

    /**
     * Returns a string representation of an enum element.
     *
     * @return a string representation
     */
    [[nodiscard]] std::string toString() const {
        return std::string{name_};
    }

    /**
     * Compares two elements of an enum.
     *
     * @tparam E Type convertible to enum `Enum<Child, Code>`
     * @param e1 The first element of the enum to be compared
     * @param e2 The second element of the enum to be compared
     *
     * @return `true` if the elements are the same
     */
    template <dxfcpp::ConvertibleTo<Enum<Child, Code>> E> friend bool operator==(const E &e1, const E &e2) {
        return e1.getCode() == e2.getCode();
    }

    /**
     * Puts the string representation of an enum element to the output stream.
     *
     * @tparam OStream A type of the output stream
     * @tparam E Type convertible to enum `Enum<Child, Code>`
     * @param os The output stream
     * @param e The element of the enum
     *
     * @return The output stream
     */
    template <typename OStream, dxfcpp::ConvertibleTo<Enum<Child, Code>> E>
    friend OStream &operator<<(OStream &os, const E &e) {
        return os << e.toString();
    }

    /**
     * Returns enum element by integer code bit pattern.
     *
     * @param code integer code.
     * @return enum element; the element with code `0` when no element has the specified code.
     * @throws std::out_of_range if no element has the specified code or code `0`.
     */
    template <Integral OtherCodeType> static const Child &valueOf(OtherCodeType code) {
        if (const auto *found = find(convertToInnerCodeType(code))) {
            return *found;
        }

        if (const auto *found = find(CodeType{})) {
            return *found;
        }

        throw std::out_of_range("Enum::valueOf: unknown code");
    }

    private:
    static const Child *find(CodeType code) noexcept {
        for (const auto &element : Child::ALL) {
            if (element.get().getCode() == code) {
                return &element.get();
            }
        }

        return nullptr;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
