// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "Common.hpp"

namespace dxfcpp {

template <typename Child, typename Code> struct Enum {
    using ParentType = Enum<Child, Code>;
    using ChildType = Child;
    using CodeType = Code;

  private:
    friend Child;

    CodeType code_;
    std::string name_;

  protected:
    template <Integral OtherCodeType>
    Enum(OtherCodeType code, std::string name) noexcept
        : code_{static_cast<Code>(static_cast<std::make_unsigned_t<Code>>(code))}, name_{std::move(name)} {}

    static const std::unordered_map<CodeType, std::reference_wrapper<const Child>> ALL;

  public:
    /**
     * Returns code that is used in flag bits.
     *
     * @return code.
     */
    [[nodiscard]] Code getCode() const { return code_; }

    /**
     * Returns the enum element name
     *
     * @return name
     */
    [[nodiscard]] const std::string &getName() const & { return name_; }

    /**
     * Returns a string representation of an enum element.
     *
     * @return a string representation
     */
    [[nodiscard]] const std::string &toString() const &noexcept { return name_; }


    /**
     * Compares two elements of an enum.
     *
     * @tparam E Type convertible to enum `Enum<Child, Code>`
     * @param e1 The first element of the enum to be compared
     * @param e2 The second element of the enum to be compared
     *
     * @return `true` if the elements are the same
     */
    template <std::convertible_to<Enum<Child, Code>> E> friend bool operator==(const E &e1, const E &e2) {
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
    template <typename OStream, std::convertible_to<Enum<Child, Code>> E> friend OStream &operator<<(OStream &&os, const E &e) {
        return os << e.toString();
    }

    /**
     * Returns enum element by integer code bit pattern.
     *
     * @param code integer code.
     * @return enum element.
     */
    template <Integral OtherCodeType> static const Child &valueOf(OtherCodeType code) {
        if (auto found = ALL.find(static_cast<CodeType>(code)); found != ALL.end()) {
            return found->second;
        }

        return Child::getDefault();
    }
};
} // namespace dxfcpp