// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <memory>
#include <mutex>
#include <unordered_set>

namespace dxfcpp {

struct DXEndpoint;

/**
 * Provides on-demand historical tick data replay controls.
 * This class is used to seamlessly transition from ordinary real-time or delayed data feed to the
 * replay of the tick-by-tick history of market data behaviour without any changes to the code
 * that consumes and process these market events.
 *
 * <p>As single {@code OnDemandService} instance
 * is conceptually associated with each {@link DXEndpoint} endpoint instance with the role
 * of {@link Role#FEED FEED} or {@link Role#ON_DEMAND_FEED ON_DEMAND_FEED}.
 * You can retrieve {@code OnDemandService} instance that is associated with a given
 * {@link DXEndpoint} using {@link #getInstance(DXEndpoint) getInstance(endpoint)} method.
 *
 * <p>For example, {@code OnDemandService} instance for a default {@link DXFeed} that is
 * featured in the documentation of {@link DXFeed} class and is retrieved via
 * {@link DXFeed#getInstance() DXFeed.getInstance()} method can be acquired with the following code:
 * <pre><tt>
 * {@link DXEndpoint} endpoint = {@link DXEndpoint}.{@link DXEndpoint#getInstance() getInstance}();
 * OnDemandService onDemand = {@link #getInstance(DXEndpoint) getInstance}(endpoint);</tt></pre>
 *
 * This instance can be used for on-demand historical tick data replay only when endpoint is connected to
 * the on-demand data provider and the appropriate credentials are provided.
 * {@link #isReplaySupported() isReplaySupported} returns {@code true} when it is so.
 *
 * <p>On-demand replay is started with {@link #replay(Date) replay} method that takes time as a parameters.
 * {@link DXFeed} is then disconnected from other data providers, pre-buffers historical tick data for replay,
 * and starts replay of the data as if it was coming from the data feeds now. All the usual APIs that are
 * part of {@link DXFeed} like {@link DXFeedSubscription subscription} and various {@link com.dxfeed.model models}
 * can be used normally as with the ordinary real-time or delayed data feed.
 *
 * <p>Replay speed can be changed on the fly with {@link #setSpeed(double) setSpeed} method or it can be set
 * initially when starting replay with a two-argument version of {@link #replay(Date, double) replay} method.
 * {@link #pause()} method is the same as {@link #setSpeed(double) setSpeed(0)}.
 *
 * <p>{@link #stopAndResume() stopAndResume} method stops data replay and resumes ordinary real-time or delayed data feed
 * that was used before {@link #replay(Date) replay} method was invoked.
 * Endpoints with a role of
 * {@link Role#ON_DEMAND_FEED ON_DEMAND_FEED} do not have an ordinary feed (they are essentially on-demand only)
 * and thus {@link #stopAndResume() stopAndResume} method works like {@link #stopAndClear() stopAndClear} for them.
 *
 * <h3>State and change notifications</h3>
 *
 * On-demand historical tick data replay state can be queried with {@link #isReplaySupported() isReplaySupported},
 * {@link #isReplay() isReplay}, {@link #getSpeed() getSpeed}, and {@link #getTime() getTime} methods.
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

    static std::shared_ptr<OnDemandService> create(void *feedHandle) noexcept;

  protected:
    OnDemandService() noexcept;

  public:
    ~OnDemandService() noexcept override;

    /**
     * Returns on-demand service for the default {@link DXEndpoint} instance with
     * {@link Role#ON_DEMAND_FEED ON_DEMAND_FEED} role that is
     * not connected to any other real-time or delayed data feed.
     * This method is a shortcut for:
     * <pre><tt>
     * OnDemandService.{@link #getInstance(DXEndpoint) getInstance}({@link DXEndpoint DXEndpoint}.{@link DXEndpoint#getInstance(DXEndpoint.Role) getInstance}({@link Role#ON_DEMAND_FEED ON_DEMAND_FEED}))</tt></pre>
     *
     * <p>If you need an instance of {@code OnDemandService} to seamlessly switch from other real-time or delayed
     * data feed to on-demand historical tick data replay, then use
     * {@link #getInstance(DXEndpoint) getInstance(endpoint)} method for a specific {@code endpoint} that
     * you are using.
     *
     * <p>This method works only when the optional "<b>dxfeed-ondemand.jar</b>" is in the class-path.
     *
     * @return on-demand endpoint for the default {@link DXEndpoint} instance.
     * @throws UnsupportedOperationException if "<b>dxfeed-ondemand.jar</b>" is not in the class-path.
     */
    static std::shared_ptr<OnDemandService> getInstance() noexcept;

    /**
     * Returns on-demand service for the specified {@link DXEndpoint}.
     * Each {@code DXEndpoint} is conceptually associated with a single instance of on-demand service to
     * control its historic data replay and this method returns this instance.
     * The endpoint must have a role of
     * {@link Role#FEED FEED} or {@link Role#ON_DEMAND_FEED ON_DEMAND_FEED}.
     *
     * <p>This method works only when the optional "<b>dxfeed-ondemand.jar</b>" is in the class-path.
     *
     * @param endpoint the endpoint.
     * @return the on-demand service.
     * @throws UnsupportedOperationException if "<b>dxfeed-ondemand.jar</b>" is not in the class-path.
     * @throws IllegalArgumentException if the specified endpoint has unsupported class or wrong role.
     */
    static std::shared_ptr<OnDemandService> getInstance(std::shared_ptr<DXEndpoint> endpoint) noexcept;

    /**
     * Returns {@link DXEndpoint} that is associated with this on-demand service.
     * @return {@link DXEndpoint} that is associated with this on-demand service.
     */
    std::shared_ptr<DXEndpoint> getEndpoint() const noexcept;

    /**
     * Returns {@code true} when on-demand historical data replay mode is supported.
     * {@link DXEndpoint} should be {@link DXEndpoint#connect(String) connected}
     * to an address with "(ondemand:&lt;address&gt;)" component that specifies an on-demand historical
     * data provider address.
     * When this method returns {@code false}, {@link #replay(Date, double) replay} method throws
     * {@link IllegalStateException}.
     * @return {@code true} when on-demand historical data replay mode is supported.
     */
    bool isReplaySupported() const noexcept;

    /**
     * Returns true when this on-demand historical data replay service is in replay mode.
     * Replay mode is in effect after invocation of {@link #replay(Date, double) replay} method and
     * before invocation {@link #stopAndResume() stopAndResume} or {@link #stopAndClear() stopAndClear} methods.
     * @return true when this on-demand historical data replay service is in replay mode.
     */
    bool isReplay() const noexcept;

    /**
     * Returns true when this on-demand historical data replay service is in clear mode.
     * Clear mode is in effect after invocation of {@link #stopAndClear()}  method and
     * before invocation {@link #stopAndResume() stopAndResume} or {@link #replay(Date, double) replay} methods.
     * @return true when this on-demand historical data replay service is in clear mode.
     */
    bool isClear() const noexcept;

    /**
     * Returns current or last on-demand historical data replay time.
     * In {@link #isReplay() replay mode} this is the time that is being currently replayed,
     * otherwise this is the last time that was replayed.
     * If replay was never started, then result is {@code new Date(0)}.
     * @return current or last on-demand historical data replay time.
     */
    std::int64_t getTime() const noexcept;

    /**
     * Returns on-demand historical data replay speed.
     * Speed is measured with respect to the real-time playback speed.
     * The result of this method is zero when this service is not in {@link #isReplay() replay mode}.
     * The speed is set when starting replay by {@link #replay(Date, double) replay(time, speed)} method
     * and with {@link #setSpeed(double) setSpeed(speed)} method during replay.
     *
     * @return on-demand historical data replay speed.
     */
    double getSpeed() const noexcept;

    /**
     * Turns on-demand historical data replay mode from a specified time with real-time speed.
     * This is a shortcut for:
     * <pre><tt>
     *     {@link #replay(Date, double) replay}(time, 1);</tt></pre>
     * This method can be used only when {@link #isReplaySupported()} method returns {@code true},
     * that is when {@link DXEndpoint} is {@link DXEndpoint#connect(String) connected}
     * to an address with "(ondemand:&lt;address&gt;)" component that specifies an on-demand historical
     * data provider address.
     *
     * @param time time to start replay from.
     * @throws IllegalStateException when {@link #isReplaySupported()} is {@code false}.
     * @throws NullPointerException if time is null.
     * @see #replay(Date, double)
     * @see #isReplaySupported()
     */
    void replay(std::int64_t time) const noexcept;

    /**
     * Turns on-demand historical data replay mode from a specified time and with a specified speed.
     * This method can be used only when {@link #isReplaySupported()} method returns {@code true},
     * that is when {@link DXEndpoint} is {@link DXEndpoint#connect(String) connected}
     * to an address with "(ondemand:&lt;address&gt;)" component that specifies an on-demand historical
     * data provider address.
     *
     * <p>After invocation of this method:
     * <ul>
     *     <li>{@link #isReplay()} returns {@code true}.
     *     <li>{@link #isClear()} returns {@code false}.
     *     <li>{@link #getTime()} returns time that is currently being replayed.
     *     <li>{@link #getSpeed()} returns {@code speed}.
     * </ul>
     *
     * @param time time to start replay from.
     * @param speed speed to start replay with. Use 1 for real-time speed, &gt;1 for faster than real-time speed,
     *              &lt;1 for slower than real-time speed, and 0 for pause.
     * @throws IllegalStateException when {@link #isReplaySupported()} is {@code false}.
     * @throws IllegalArgumentException if {@code speed < 0}.
     * @throws NullPointerException when if is null.
     * @see #isReplaySupported()
     */
    void replay(std::int64_t time, double speed) const noexcept;

    /**
     * Pauses on-demand historical data replay and keeps data snapshot.
     * This method can only be called in {@link #isReplay() replay mode}.
     * This is a shortcut for:
     * <pre><tt>
     *     {@link #setSpeed(double) setSpeed}(0);</tt></pre>
     * This method atomically captures current replay time and pauses at it.
     * After invocation of this method:
     * <ul>
     *     <li>{@link #isReplay()} returns {@code true}.
     *     <li>{@link #isClear()} returns {@code false}.
     *     <li>{@link #getTime()} returns time where replay was paused at.
     *     <li>{@link #getSpeed()} returns {@code 0}.
     * </ul>
     * @throws IllegalStateException if on-demand service was not in {@link #isReplay() replay mode}.
     */
    void pause() const noexcept;

    /**
     * Stops on-demand historical data replay and resumes ordinary data feed.
     * This method has no effect when invoked not in {@link #isReplay() replay mode}.
     * After invocation of this method:
     * <ul>
     *     <li>{@link #isReplay()} returns {@code false}.
     *     <li>{@link #isClear()} returns {@code false}.
     *     <li>{@link #getTime()} returns last replayed time.
     *     <li>{@link #getSpeed()} returns {@code 0}.
     * </ul>
     *
     * <p>To stop on-demand historical data replay without resuming ordinary data feed use
     * either {@link #pause()} to keep data snapshot or {@link #stopAndClear()} to clear data.
     *
     * <p>Note, that endpoints with a role of
     * {@link Role#ON_DEMAND_FEED ON_DEMAND_FEED} do not have an ordinary feed (they are essentially on-demand only)
     * and thus {@code stopAndResume} method works like {@link #stopAndClear() stopAndClear} for them.
     */
    void stopAndResume() const noexcept;

    /**
     * Stops incoming data and clears it without resuming data updates.
     * This method works both in on-demand historical data replay mode and
     * for ordinary data feed. All incoming data updates are suspended and current data is cleared.
     * After invocation of this method:
     * <ul>
     *     <li>{@link #isReplay()} returns {@code false}.
     *     <li>{@link #isClear()} returns {@code true}.
     *     <li>{@link #getTime()} returns last replayed time.
     *     <li>{@link #getSpeed()} returns {@code 0}.
     * </ul>
     *
     * <p>Ordinary data feed can be resumed with {@link #stopAndResume()} method and on-demand historical data
     * replay can be continued with {@link #replay(Date, double) replay(...)} method.
     */
    void stopAndClear() const noexcept;

    /**
     * Changes on-demand historical data replay speed while continuing replay at current {@link #getTime()} time}.
     * Speed is measured with respect to the real-time playback speed.
     * This method can only be called with non-zero speed in {@link #isReplay() replay mode}.
     * @param speed on-demand historical data replay speed.
     * @throws IllegalStateException if on-demand service was not in {@link #isReplay() replay mode} and {@code speed != 0}.
     * @throws IllegalArgumentException if {@code speed < 0}.
     */
    void setSpeed(double speed) const noexcept;
};

}