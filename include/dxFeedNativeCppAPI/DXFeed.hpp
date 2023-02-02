// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_endpoint.h>
#include <dxfg_feed.h>

#include "internal/CEntryPointErrors.hpp"
#include "internal/Common.hpp"
#include "internal/Handler.hpp"
#include "internal/Isolate.hpp"

#include <memory>
#include <mutex>

namespace dxfcpp {

struct DXEndpoint;

/**
 * Main entry class for dxFeed API (<b>read it first</b>).
 */
struct DXFeed : std::enable_shared_from_this<DXFeed> {
    friend struct DXEndpoint;

  private:
    mutable std::recursive_mutex mtx_{};
    dxfg_feed_t *handle_{};

    static std::shared_ptr<DXFeed> create(dxfg_feed_t *feedHandle) {
        std::shared_ptr<DXFeed> feed{new (std::nothrow) DXFeed{}};

        feed->handle_ = feedHandle;

        return feed;
    }

    void releaseHandleImpl() {
        if (!handle_) {
            return;
        }

        detail::Isolate::getInstance()->runIsolatedOrElse(
            [this](auto threadHandle) {
                if (handle_) {
                    return dxfg_JavaObjectHandler_release(threadHandle,
                                                          detail::bit_cast<dxfg_java_object_handler *>(handle_)) == 0;
                }

                return true;
            },
            false);

        handle_ = nullptr;
    }

  protected:
    DXFeed() : mtx_(), handle_() {
        if constexpr (detail::isDebug) {
            detail::debug("DXFeed()");
        }
    }

  public:
    virtual ~DXFeed() {
        if constexpr (detail::isDebug) {
            detail::debug("DXFeed{{{}}}::~DXFeed()", detail::bit_cast<std::size_t>(handle_));
        }

        releaseHandleImpl();
    }

    /**
     * Returns a default application-wide singleton instance of feed. Most applications use only a single
     * data-source and should rely on this method to get one. This is a shortcut to
     * @ref DXEndpoint "DXEndpoint"::@ref DXEndpoint::getInstance() "getInstance()"->@ref DXEndpoint::getFeed()
     * "getFeed()".
     *
     * @return The DXFeed instance
     */
    static std::shared_ptr<DXFeed> getInstance();
};

} // namespace dxfcpp