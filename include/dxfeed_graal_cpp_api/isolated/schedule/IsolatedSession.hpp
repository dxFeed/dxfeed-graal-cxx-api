// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../schedule/Session.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedSessionFilter {

/* dxfg_session_filter_t* */ JavaObjectHandle<dxfcpp::SessionFilter> getInstance(std::uint32_t code);

};

namespace IsolatedSession {

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDay(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session);

std::int32_t getType(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session);

bool isTrading(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session);

bool isEmpty(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session);

std::int64_t getStartTime(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session);

std::int64_t getEndTime(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session);

bool containsTime(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session, std::int64_t time);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getPrevSession(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
               /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getNextSession(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
               /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findPrevSession(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
                /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findNextSession(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
                /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter);

std::size_t getHashCode(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session);

bool equals(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
            /* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &otherSession);

std::string toString(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session);

} // namespace IsolatedSession

namespace IsolatedSessionList {

bool release(/* dxfg_session_list* */ void *sessionList);

std::unique_ptr<void, decltype(&IsolatedSessionList::release)> toUnique(/* dxfg_session_list* */ void *sessionList);

} // namespace IsolatedSessionList

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()