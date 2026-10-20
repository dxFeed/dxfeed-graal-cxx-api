// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

/**
 * An immutable string that refers either to a string with static storage duration (a literal) or to an owned copy.
 *
 * A literal string does not allocate and is created in constant expressions, so a constant that holds it is
 * initialized before any dynamic initialization (`constinit`). An owned copy is shared between the copies of the
 * string, so its view stays valid while any of them exists.
 */
struct ConstString final {
    private:
    std::string_view view_{};
    std::shared_ptr<const std::string> owned_{};

    explicit ConstString(std::shared_ptr<const std::string> owned) noexcept : view_{*owned}, owned_{std::move(owned)} {
    }

    public:
    /**
     * Characters with static storage duration (a string literal). A trivial type, so it is passed to the constructors
     * of the constants in constant expressions.
     */
    struct Literal final {
        std::string_view view{};

        template <std::size_t N> constexpr explicit Literal(const char (&literal)[N]) noexcept : view{literal, N - 1} {
        }
    };

    /// Creates an empty string.
    constexpr ConstString() noexcept = default;

    /**
     * Creates a string that refers to the specified characters without copying them.
     *
     * @param literal The characters with static storage duration.
     */
    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr ConstString(Literal literal) noexcept : view_{literal.view} {
    }

    /**
     * Creates a string that owns a copy of the specified characters.
     *
     * @param s The characters.
     * @return The string.
     */
    static ConstString copyOf(std::string_view s) {
        return ConstString{std::make_shared<const std::string>(s)};
    }

    ConstString(const ConstString &) = default;
    ConstString &operator=(const ConstString &) = default;

    ConstString(ConstString &&other) noexcept : view_{std::exchange(other.view_, {})}, owned_{std::move(other.owned_)} {
    }

    ConstString &operator=(ConstString &&other) noexcept {
        view_ = std::exchange(other.view_, {});
        owned_ = std::move(other.owned_);

        return *this;
    }

    ~ConstString() = default;

    /// @return The characters of the string.
    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return view_;
    }

    /// @return A copy of the string.
    [[nodiscard]] std::string str() const {
        return std::string{view_};
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator std::string_view() const noexcept {
        return view_;
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return view_.empty();
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return view_.size();
    }

    /// @return `true` if the string owns its characters.
    [[nodiscard]] bool isOwned() const noexcept {
        return owned_ != nullptr;
    }

    friend constexpr bool operator==(const ConstString &a, const ConstString &b) noexcept {
        return a.view_ == b.view_;
    }

    friend constexpr bool operator==(const ConstString &a, std::string_view b) noexcept {
        return a.view_ == b;
    }

    friend std::ostream &operator<<(std::ostream &os, const ConstString &s) {
        return os << s.view_;
    }
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::ConstString> {
    std::size_t operator()(const dxfcpp::ConstString &s) const noexcept {
        return std::hash<std::string_view>{}(s.view());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
