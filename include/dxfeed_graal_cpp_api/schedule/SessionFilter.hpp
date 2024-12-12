// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/JavaObjectHandle.hpp"
#include "SessionType.hpp"

#include <mutex>

DXFCPP_BEGIN_NAMESPACE

enum class SessionFilterEnum : std::uint32_t {
    /** Accepts any session - useful for pure schedule navigation. */
    ANY = 0,
    /** Accepts trading sessions only - those with <code>(Session::isTrading() == true)</code>. */
    TRADING,
    /** Accepts non-trading sessions only - those with <code>(Session::isTrading() == false)</code>. */
    NON_TRADING,

    /** Accepts any session with type SessionType::NO_TRADING. */
    NO_TRADING,
    /** Accepts any session with type SessionType::PRE_MARKET. */
    PRE_MARKET,
    /** Accepts any session with type SessionType::REGULAR. */
    REGULAR,
    /** Accepts any session with type SessionType::AFTER_MARKET. */
    AFTER_MARKET,
};

/**
 * A filter for sessions used by various search methods.
 * This class provides predefined filters for certain Session attributes,
 * although users can create their own filters to suit their needs.
 * <p>
 * Please note that sessions can be either trading or non-trading, and this distinction can be
 * either based on rules (e.g. weekends) or dictated by special occasions (e.g. holidays).
 * Different filters treat this distinction differently - some accept only trading sessions,
 * some only non-trading, and some ignore type of session altogether.
 */
struct DXFCPP_EXPORT SessionFilter final {
    /** Accepts any session - useful for pure schedule navigation. */
    static const SessionFilter ANY;
    /** Accepts trading sessions only - those with <code>(Session::isTrading() == true)</code>. */
    static const SessionFilter TRADING;
    /** Accepts non-trading sessions only - those with <code>(Session::isTrading() == false)</code>. */
    static const SessionFilter NON_TRADING;

    /** Accepts any session with type SessionType::NO_TRADING. */
    static const SessionFilter NO_TRADING;
    /** Accepts any session with type SessionType::PRE_MARKET. */
    static const SessionFilter PRE_MARKET;
    /** Accepts any session with type SessionType::REGULAR. */
    static const SessionFilter REGULAR;
    /** Accepts any session with type SessionType::AFTER_MARKET. */
    static const SessionFilter AFTER_MARKET;

  protected:
    SessionFilterEnum code_{};

    std::string name_{};

    /// Required type, std::nullopt if not relevant.
    std::optional<SessionType> type_{};
    /// Required trading flag, std::nullopt if not relevant.
    std::optional<bool> trading_{};

    mutable std::mutex mtx_{};
    mutable JavaObjectHandle<SessionFilter> handle_{};

  public:
    SessionFilter() noexcept = default;
    virtual ~SessionFilter() noexcept = default;

    SessionFilter(const SessionFilter &) = delete;
    SessionFilter(SessionFilter &&) noexcept = delete;
    SessionFilter &operator=(const SessionFilter &) = delete;
    SessionFilter &operator=(const SessionFilter &&) noexcept = delete;

    /**
     * Creates filter with specified type and trading flag conditions.
     * <p>
     * Both parameters specify what value corresponding attributes should have.
     * If some parameter is std::nullopt then corresponding attribute is ignored (any value is accepted).
     *
     * @param code The enum code
     * @param name The enum name
     * @param type required type, std::nullopt if not relevant
     * @param trading required trading flag, std::nullopt if not relevant
     */
    SessionFilter(SessionFilterEnum code, std::string name, std::optional<SessionType> type, std::optional<bool> trading) noexcept;

    /**
     * Tests whether or not the specified session is an acceptable result.
     *
     * @param session the session to be tested
     * @return `true` if specified session is accepted
     */
    template <typename Session> bool accept(Session session) const noexcept {
        return (!type_ || type_.value() == session.getType()) && (!trading_ || trading_.value() == session.isTrading());
    }

    bool operator==(const SessionFilter &sessionFilter) const noexcept {
        return type_ == sessionFilter.type_ && trading_ == sessionFilter.trading_;
    }

    std::string toString() const {
        return std::string("SessionFilter(") + ((!type_) ? "null" : type_.value().toString()) + ", " +
               ((!trading_) ? "null" : dxfcpp::toString(trading_.value())) + ")";
    }
    const std::optional<SessionType> &getType() const & noexcept {
        return type_;
    }

    const std::optional<bool> &getTrading() const & noexcept {
        return trading_;
    }

    const JavaObjectHandle<SessionFilter>& getHandle() const&;
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::SessionFilter> {
    std::size_t operator()(const dxfcpp::SessionFilter &sessionFilter) const noexcept {
        std::size_t hash = 0;

        if (sessionFilter.getType()) {
            dxfcpp::hashCombine(hash, sessionFilter.getType().value());
        }

        if (sessionFilter.getTrading()) {
            dxfcpp::hashCombine(hash, sessionFilter.getTrading().value());
        }

        return hash;
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()