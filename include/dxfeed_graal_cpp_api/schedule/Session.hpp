// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

struct Day;
struct Schedule;
struct SessionType;

/**
 * <b>Session</b> represents a continuous period of time during which apply same rules regarding trading activity.
 * For example, <code>regular trading session</code> is a period of time consisting of one day of business activities
 * in a financial market, from the opening bell to the closing bell, when regular trading occurs.
 * <p>
 * Sessions can be either <b>trading</b> or <b>non-trading</b>, with different sets of rules and reasons to exist.
 * Sessions do not overlap with each other - rather they form consecutive chain of adjacent periods of time that
 * cover entire time scale. The point on a border line is considered to belong to following session that starts there.
 * Each session completely fits inside a certain day. Day may contain sessions with zero duration - e.g. indices
 * that post value once a day. Such sessions can be of any appropriate type, trading or non-trading.
 */
struct DXFCPP_EXPORT Session {
    friend struct Schedule;
    friend struct Day;

    /// The alias to a type of shared pointer to the Session object
    using Ptr = std::shared_ptr<Session>;

    /// The alias to a type of unique pointer to the Session object
    using Unique = std::unique_ptr<Session>;

  private:
    JavaObjectHandle<Session> handle_;

    explicit Session(JavaObjectHandle<Session> &&handle) noexcept;

    /**
     * Checks the handle, attempts to allocate memory for the pointer and return Session::Ptr
     *
     * @param handle The graal Session's handle
     * @return The smart pointer for the Session object.
     */
    static Session::Ptr create(JavaObjectHandle<Session> &&handle);

  public:
    /**
     * Returns day to which this session belongs.
     *
     * @return The day to which this session belongs.
     */
    std::shared_ptr<Day> getDay() const;

    /**
     * Returns type of this session.
     *
     * @return The type of this session.
     */
    const SessionType &getType() const &;

    /**
     * @return <code>true</code> if trading activity is allowed within this session.
     * This method is equivalent to expression @ref SessionType::isTrading() "getType().isTrading()".
     * <p>
     * Some sessions may have zero duration - e.g. indices that post value once a day.
     * Such sessions can be of any appropriate type, trading or non-trading.
     */
    bool isTrading() const;

    /**
     * @return <code>true</code> if this session has zero duration.
     * Empty sessions can be used for indices that post value once a day or for convenience.
     * Such sessions can be of any appropriate type, trading or non-trading.
     */
    bool isEmpty() const;

    /**
     * @return start time of this session (inclusive).
     * For normal sessions the start time is less than the end time, for empty sessions they are equal.
     */
    std::int64_t getStartTime() const;

    /**
     * @return end time of this session (exclusive).
     * For normal sessions the end time is greater than the start time, for empty sessions they are equal.
     */
    std::int64_t getEndTime() const;

    /**
     * @return <code>true</code> if specified time belongs to this session.
     */
    bool containsTime(std::int64_t time) const;

    /**
     * Returns previous session accepted by specified filter.
     * This method may cross the day boundary and return appropriate session from
     * previous days - up to a year back in time. If no such session was found
     * within one year this method will throw JavaException "NoSuchElementException".
     * <p>
     * To find previous trading session of any type use this code:
     * <pre>session = session->getPrevSession(SessionFilter::TRADING);</pre>
     * To find previous regular trading session use this code:
     * <pre>session = session->getPrevSession(SessionFilter::REGULAR);</pre>
     *
     * @param filter The filter to test sessions
     * @return The nearest previous session that is accepted by the filter.
     * @throw JavaException "NoSuchElementException" if no such session was found within one year.
     */
    Session::Ptr getPrevSession(const SessionFilter &filter) const;

    /**
     * Returns following session accepted by specified filter.
     * This method may cross the day boundary and return appropriate session from
     * following days - up to a year in the future. If no such session was found
     * within one year this method will throw JavaException "NoSuchElementException".
     * <p>
     * To find following trading session of any type use this code:
     * <pre>session = session->getNextSession(SessionFilter::TRADING);</pre>
     * To find following regular trading session use this code:
     * <pre>session = session->getNextSession(SessionFilter::REGULAR);</pre>
     *
     * @param filter The filter to test sessions
     * @return The nearest following session that is accepted by the filter.
     * @throw JavaException "NoSuchElementException" if no such session was found within one year.
     */
    Session::Ptr getNextSession(const SessionFilter &filter) const;

    /**
     * Returns previous session accepted by specified filter.
     * This method may cross the day boundary and return appropriate session from
     * previous days - up to a year back in time. If no such session was found
     * within one year this method will return `Session::Ptr{nullptr}` (std::shared_ptr<Session>{nullptr}).
     * <p>
     * To find previous trading session of any type use this code:
     * <pre>session = session->findPrevSession(SessionFilter::TRADING);</pre>
     * To find previous regular trading session use this code:
     * <pre>session = session->findPrevSession(SessionFilter::REGULAR);</pre>
     *
     * @param filter The filter to test sessions
     * @return nearest previous session that is accepted by the filter or `Session::Ptr{nullptr}` (std::shared_ptr<Session>{nullptr})
     */
    Session::Ptr findPrevSession(const SessionFilter &filter) const;

    /**
     * Returns following session accepted by specified filter.
     * This method may cross the day boundary and return appropriate session from
     * following days - up to a year in the future. If no such session was found
     * within one year this method will return `Session::Ptr{nullptr}` (std::shared_ptr<Session>{nullptr}).
     * <p>
     * To find following trading session of any type use this code:
     * <pre>session = session->findNextSession(SessionFilter::TRADING);</pre>
     * To find following regular trading session use this code:
     * <pre>session = session->findNextSession(SessionFilter::REGULAR);</pre>
     *
     * @param filter The filter to test sessions
     * @return nearest following session that is accepted by the filter or `Session::Ptr{nullptr}` (std::shared_ptr<Session>{nullptr})
     */
    Session::Ptr findNextSession(const SessionFilter &filter) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const Session &other) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const Session::Ptr &other) const {
        return *this == *other;
    }

    /**
     * @return A hash code value for this object.
     */
    std::size_t getHashCode() const;

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const;
};

DXFCPP_END_NAMESPACE

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::Session> {
    std::size_t operator()(const dxfcpp::Session &session) const {
        return session.getHashCode();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()