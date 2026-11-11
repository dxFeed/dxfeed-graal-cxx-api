// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedSession.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedSessionFilter {

/// dxfg_v4_SessionFilter_getInstance
/* dxfg_v4_session_filter_t* */ JavaObjectHandle<SessionFilter> getInstance(std::uint32_t code) {
    return JavaObjectHandle<SessionFilter>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_SessionFilter_getInstance, static_cast<dxfg_v4_session_filter_prepare_t>(code))};
}

} // namespace IsolatedSessionFilter

namespace IsolatedSession {

/// dxfg_v4_Session_getDay
/* dxfg_v4_day_t* */ JavaObjectHandle<Day>
getDay(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_getDay`. The `session` handle is invalid");
    }

    return JavaObjectHandle<Day>{runGraalV4FunctionWithNonNullResult(dxfg_v4_Session_getDay,
                                                                     static_cast<dxfg_v4_session_t *>(session.get()))};
}

/// dxfg_v4_Session_getType
std::int32_t getType(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_getType`. The `session` handle is invalid");
    }

    return static_cast<std::int32_t>(
        runGraalV4FunctionWithResult(dxfg_v4_Session_getType, static_cast<dxfg_v4_session_t *>(session.get())));
}

/// dxfg_v4_Session_isTrading
bool isTrading(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_isTrading`. The `session` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Session_isTrading, static_cast<dxfg_v4_session_t *>(session.get())) !=
           0;
}

/// dxfg_v4_Session_isEmpty
bool isEmpty(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_isEmpty`. The `session` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Session_isEmpty, static_cast<dxfg_v4_session_t *>(session.get())) != 0;
}

/// dxfg_v4_Session_getStartTime
std::int64_t getStartTime(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_getStartTime`. The `session` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Session_getStartTime, static_cast<dxfg_v4_session_t *>(session.get()));
}

/// dxfg_v4_Session_getEndTime
std::int64_t getEndTime(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_getEndTime`. The `session` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Session_getEndTime, static_cast<dxfg_v4_session_t *>(session.get()));
}

/// dxfg_v4_Session_containsTime
bool containsTime(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session, std::int64_t time) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_containsTime`. The `session` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Session_containsTime, static_cast<dxfg_v4_session_t *>(session.get()),
                                        time) != 0;
}

/// dxfg_v4_Session_getPrevSession
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
getPrevSession(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session,
               /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_getPrevSession`. The `session` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_getPrevSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Session_getPrevSession, static_cast<dxfg_v4_session_t *>(session.get()),
        static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Session_getNextSession
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
getNextSession(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session,
               /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_getNextSession`. The `session` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_getNextSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Session_getNextSession, static_cast<dxfg_v4_session_t *>(session.get()),
        static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Session_findPrevSession
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
findPrevSession(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session,
                /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_findPrevSession`. The `session` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_findPrevSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{
        runGraalV4FunctionWithResult(dxfg_v4_Session_findPrevSession, static_cast<dxfg_v4_session_t *>(session.get()),
                                     static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Session_findNextSession
/* dxfg_v4_session_t* */ JavaObjectHandle<Session>
findNextSession(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session,
                /* dxfg_v4_session_filter_t* */ const JavaObjectHandle<SessionFilter> &filter) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_findNextSession`. The `session` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_findNextSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<Session>{
        runGraalV4FunctionWithResult(dxfg_v4_Session_findNextSession, static_cast<dxfg_v4_session_t *>(session.get()),
                                     static_cast<dxfg_v4_session_filter_t *>(filter.get()))};
}

/// dxfg_v4_Session_hashCode
std::size_t hashCode(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_hashCode`. The `session` handle is invalid");
    }

    return static_cast<std::size_t>(
        runGraalV4FunctionWithResult(dxfg_v4_Session_hashCode, static_cast<dxfg_v4_session_t *>(session.get())));
}

/// dxfg_v4_Session_equals
bool equals(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session,
            /* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &otherSession) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_equals`. The `session` handle is invalid");
    }

    if (!otherSession) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_equals`. The `otherSession` handle is invalid");
    }

    if (session.get() == otherSession.get()) {
        return true;
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Session_equals, static_cast<dxfg_v4_session_t *>(session.get()),
                                        static_cast<dxfg_v4_session_t *>(otherSession.get())) != 0;
}

/// dxfg_v4_Session_toString
std::string toString(/* dxfg_v4_session_t* */ const JavaObjectHandle<Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_Session_toString`. The `session` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Session_toString, static_cast<dxfg_v4_session_t *>(session.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

} // namespace IsolatedSession

namespace IsolatedSessionList {

/// dxfg_v4_session_list_wrapper_release: releases the list and keeps the sessions.
bool release(/* dxfg_v4_session_list* */ void *sessionList) {
    if (!sessionList) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_session_list_wrapper_release`. The `sessionList` is nullptr");
    }

    runGraalV4Function(dxfg_v4_session_list_wrapper_release, static_cast<dxfg_v4_session_list *>(sessionList));

    return true;
}

std::unique_ptr<void, decltype(&release)> toUnique(/* dxfg_v4_session_list* */ void *sessionList) {
    return {sessionList, release};
}

} // namespace IsolatedSessionList

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE