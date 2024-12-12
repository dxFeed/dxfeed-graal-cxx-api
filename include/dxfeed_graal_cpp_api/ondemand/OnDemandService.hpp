// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <memory>
#include <mutex>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXEndpoint;

/**
 * Provides on-demand historical tick data replay controls.
 * This class is used to seamlessly transition from ordinary real-time or delayed data feed to the replay of the
 * tick-by-tick history of market data behaviour without any changes to the code that consumes and process these market
 * events.
 *
 * <p>As single OnDemandService instance is conceptually associated with each DXEndpoint endpoint instance with the role
 * of @ref DXEndpoint::Role::FEED "FEED" or @ref DXEndpoint::Role::ON_DEMAND_FEED "ON_DEMAND_FEED".
 * You can retrieve OnDemandService instance that is associated with a given
 * DXEndpoint using @ref OnDemandService::getInstance(std::shared_ptr<DXEndpoint>) "getInstance(endpoint)" method.
 *
 * <p>For example, OnDemandService instance for a default DXFeed that is featured in the documentation of DXFeed class
 * and is retrieved via
 * @ref DXFeed::getInstance() "DXFeed::getInstance()" method can be acquired with the following code:
 * <pre><tt>
 * @ref DXEndpoint "auto" endpoint = DXEndpoint::getInstance();
 * @ref OnDemandService "auto" onDemand = OnDemandService::@ref OnDemandService::getInstance(std::shared_ptr<DXEndpoint>) "getInstance"(endpoint);
 * </tt></pre>
 *
 * This instance can be used for on-demand historical tick data replay only when endpoint is connected to
 * the on-demand data provider and the appropriate credentials are provided.
 * @ref OnDemandService::isReplaySupported() "isReplaySupported" returns `true` when it is so.
 *
 * <p>On-demand replay is started with @ref OnDemandService::replay() "replay" method that takes time as a parameter.
 * DXFeed is then disconnected from other data providers, pre-buffers historical tick data for replay,
 * and starts replay of the data as if it was coming from the data feeds now. All the usual APIs that are
 * part of DXFeed like @ref DXFeedSubscription "subscription" and various dxFeed models
 * can be used normally as with the ordinary real-time or delayed data feed.
 *
 * <p>Replay speed can be changed on the fly with @ref OnDemandService::setSpeed() "setSpeed" method or it can be set
 * initially when starting replay with a two-argument version of @ref OnDemandService::replay() "replay" method.
 * @ref OnDemandService::pause() "pause" method is the same as @ref OnDemandService::setSpeed() "setSpeed(0)".
 *
 * <p>@ref OnDemandService::stopAndResume() "stopAndResume" method stops data replay and resumes ordinary real-time or delayed data feed
 * that was used before @ref OnDemandService::replay() "replay" method was invoked.
 * Endpoints with a role of
 * @ref DXEndpoint::Role::ON_DEMAND_FEED "ON_DEMAND_FEED" do not have an ordinary feed (they are essentially on-demand only)
 * and thus @ref OnDemandService::stopAndResume() "stopAndResume" method works like @ref OnDemandService::stopAndClear() "stopAndClear" for them.
 *
 * <h3>State and change notifications</h3>
 *
 * On-demand historical tick data replay state can be queried with @ref OnDemandService::isReplaySupported() "isReplaySupported",
 * @ref OnDemandService::isReplay() "isReplay", @ref OnDemandService::getSpeed() "getSpeed", and @ref OnDemandService::getTime() "getTime" methods.
 *
 * <h3>Threads and locks</h3>
 *
 * This class is thread-safe and can be used concurrently from multiple threads without external synchronization.
 */
struct DXFCPP_EXPORT OnDemandService : SharedEntity {
    /// The alias to a type of shared pointer to the OnDemandService object
    using Ptr = std::shared_ptr<OnDemandService>;

    /// The alias to a type of unique pointer to the OnDemandService object
    using Unique = std::unique_ptr<OnDemandService>;

  private:
    JavaObjectHandle<OnDemandService> handle_;
    std::shared_ptr<DXEndpoint> endpoint_{};

  protected:
    OnDemandService() noexcept;

  public:
    ~OnDemandService() noexcept override;

    /**
     * Returns on-demand service for the default DXEndpoint instance with
     * @ref DXEndpoint::Role::ON_DEMAND_FEED "ON_DEMAND_FEED" role that is
     * not connected to any other real-time or delayed data feed.
     * This method is a shortcut for:
     * <pre><tt>
     * OnDemandService::@ref OnDemandService::getInstance(std::shared_ptr<DXEndpoint>) "getInstance"(@ref DXEndpoint "DXEndpoint"::@ref DXEndpoint::getInstance(DXEndpoint::Role) "getInstance"(DXEndpoint::Role::ON_DEMAND_FEED))</tt></pre>
     *
     * <p>If you need an instance of OnDemandService to seamlessly switch from other real-time or delayed
     * data feed to on-demand historical tick data replay, then use
     * {@ref OnDemandService::getInstance(std::shared_ptr<DXEndpoint>) "getInstance(endpoint)" method for a specific `endpoint` that
     * you are using.
     *
     * @return on-demand endpoint for the default DXEndpoint instance.
     */
    static std::shared_ptr<OnDemandService> getInstance();

    /**
     * Returns on-demand service for the specified DXEndpoint.
     * Each DXEndpoint is conceptually associated with a single instance of on-demand service to
     * control its historic data replay and this method returns this instance.
     * The endpoint must have a role of
     * @ref DXEndpoint::Role::FEED "FEED" or @ref DXEndpoint::Role::ON_DEMAND_FEED "ON_DEMAND_FEED".
     *
     * @param endpoint the endpoint.
     * @return the on-demand service.
     */
    static std::shared_ptr<OnDemandService> getInstance(std::shared_ptr<DXEndpoint> endpoint);

    /**
     * Returns DXEndpoint that is associated with this on-demand service.
     *
     * @return DXEndpoint that is associated with this on-demand service.
     */
    std::shared_ptr<DXEndpoint> getEndpoint() const noexcept;

    /**
     * Returns `true` when on-demand historical data replay mode is supported.
     * DXEndpoint should be @ref DXEndpoint::connect() "connected"
     * to an address with "(ondemand:<address>)" component that specifies an on-demand historical
     * data provider address.
     * When this method returns `false`, @ref OnDemandService::replay() "replay" method does nothing.
     *
     * @return `true` when on-demand historical data replay mode is supported.
     */
    bool isReplaySupported() const;

    /**
     * Returns `true` when this on-demand historical data replay service is in replay mode.
     * Replay mode is in effect after invocation of @ref OnDemandService::replay() "replay" method and
     * before invocation @ref OnDemandService::stopAndResume() "stopAndResume" or @ref OnDemandService::stopAndClear() "stopAndClear" methods.
     *
     * @return `true` when this on-demand historical data replay service is in replay mode.
     */
    bool isReplay() const;

    /**
     * Returns `true` when this on-demand historical data replay service is in clear mode.
     * Clear mode is in effect after invocation of @ref OnDemandService::stopAndClear() "stopAndClear"  method and
     * before invocation @ref OnDemandService::stopAndResume() "stopAndResume" or @ref OnDemandService::replay() "replay" methods.
     *
     * @return `true` when this on-demand historical data replay service is in clear mode.
     */
    bool isClear() const;

    /**
     * Returns current or last on-demand historical data replay time.
     * In @ref OnDemandService::isReplay() "replay mode" this is the time that is being currently replayed,
     * otherwise this is the last time that was replayed.
     * If replay was never started, then result is 0.
     *
     * @return current or last on-demand historical data replay time in millis.
     */
    std::int64_t getTime() const;

    /**
     * Returns on-demand historical data replay speed.
     * Speed is measured with respect to the real-time playback speed.
     * The result of this method is zero when this service is not in @ref OnDemandService::isReplay() "replay mode".
     * The speed is set when starting replay by @ref OnDemandService::replay() "replay(time, speed)" method
     * and with @ref OnDemandService::setSpeed() "setSpeed(speed)" method during replay.
     *
     * @return on-demand historical data replay speed.
     */
    double getSpeed() const;

    /**
     * Turns on-demand historical data replay mode from a specified time with real-time speed.
     * This is a shortcut for:
     * <pre><tt>
     *     @ref OnDemandService::replay(std::int64_t,double) "replay"(time, 1);</tt></pre>
     * This method can be used only when OnDemandService::isReplaySupported() method returns `true`,
     * that is when DXEndpoint is @ref DXEndpoint::connect() "connected"
     * to an address with "(ondemand:<address>)" component that specifies an on-demand historical
     * data provider address.
     *
     * @param time time (timestamp in millis) to start replay from.
     */
    void replay(std::int64_t time) const;

    /**
     * Turns on-demand historical data replay mode from a specified time and with a specified speed.
     * This method can be used only when OnDemandService::isReplaySupported() method returns `true`,
     * that is when DXEndpoint is @ref DXEndpoint::connect() "connected"
     * to an address with "(ondemand:<address>)" component that specifies an on-demand historical
     * data provider address.
     *
     * <p>After invocation of this method:
     * <ul>
     *     <li>OnDemandService::isReplay() returns `true`.
     *     <li>OnDemandService::isClear() returns `false`.
     *     <li>OnDemandService::getTime() returns time that is currently being replayed.
     *     <li>OnDemandService::getSpeed() returns `speed`.
     * </ul>
     *
     * @param time time (timestamp in millis) to start replay from.
     * @param speed speed to start replay with. Use 1 for real-time speed, >1 for faster than real-time speed,
     *              <1 for slower than real-time speed, and 0 for pause.
     */
    void replay(std::int64_t time, double speed) const;

    /**
     * Pauses on-demand historical data replay and keeps data snapshot.
     * This method can only be called in @ref OnDemandService::isReplay() "replay mode".
     * This is a shortcut for:
     * <pre><tt>
     *     @ref OnDemandService::setSpeed() "setSpeed"(0);</tt></pre>
     * This method atomically captures current replay time and pauses at it.
     * After invocation of this method:
     * <ul>
     *     <li>OnDemandService::isReplay() returns `true`.
     *     <li>OnDemandService::isClear() returns `false`.
     *     <li>OnDemandService::getTime() returns time where replay was paused at.
     *     <li>OnDemandService::getSpeed() returns `0`.
     * </ul>
     */
    void pause() const;

    /**
     * Stops on-demand historical data replay and resumes ordinary data feed.
     * This method has no effect when invoked not in @ref OnDemandService::isReplay() "replay mode".
     * After invocation of this method:
     * <ul>
     *     <li>OnDemandService::isReplay() returns `false`.
     *     <li>OnDemandService::isClear() returns `false`.
     *     <li>OnDemandService::getTime() returns last replayed time.
     *     <li>OnDemandService::getSpeed() returns `0`.
     * </ul>
     *
     * <p>To stop on-demand historical data replay without resuming ordinary data feed use
     * either OnDemandService::pause() to keep data snapshot or OnDemandService::stopAndClear() to clear data.
     *
     * <p>Note, that endpoints with a role of
     * @ref DXEndpoint::Role::ON_DEMAND_FEED "ON_DEMAND_FEED" do not have an ordinary feed (they are essentially on-demand only)
     * and thus `stopAndResume` method works like @ref OnDemandService::stopAndClear() "stopAndClear" for them.
     */
    void stopAndResume() const;

    /**
     * Stops incoming data and clears it without resuming data updates.
     * This method works both in on-demand historical data replay mode and
     * for ordinary data feed. All incoming data updates are suspended and current data is cleared.
     * After invocation of this method:
     * <ul>
     *     <li>OnDemandService::isReplay() returns `false`.
     *     <li>OnDemandService::isClear() returns `true`.
     *     <li>OnDemandService::getTime() returns last replayed time.
     *     <li>OnDemandService::getSpeed() returns `0`.
     * </ul>
     *
     * <p>Ordinary data feed can be resumed with OnDemandService::stopAndResume() method and on-demand historical data
     * replay can be continued with @ref OnDemandService::replay() "replay(...)" method.
     */
    void stopAndClear() const;

    /**
     * Changes on-demand historical data replay speed while continuing replay at current @ref OnDemandService::getTime() "time".
     * Speed is measured with respect to the real-time playback speed.
     * This method can only be called with non-zero speed in @ref OnDemandService::isReplay() "replay mode".
     *
     * @param speed on-demand historical data replay speed.
     */
    void setSpeed(double speed) const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()