// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/schedule/IsolatedSession.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::schedule {

namespace IsolatedSessionFilter {

/* dxfg_session_filter_t* */ JavaObjectHandle<dxfcpp::SessionFilter> getInstance(std::uint32_t code) {
    return JavaObjectHandle<dxfcpp::SessionFilter>{runGraalFunctionAndThrowIfNullptr(
        dxfg_SessionFilter_getInstance, static_cast<dxfg_session_filter_prepare_t>(code))};
}

}; // namespace IsolatedSessionFilter

namespace IsolatedSession {

/* dxfg_day_t* */ JavaObjectHandle<dxfcpp::Day>
getDay(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getDay`. The `session` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Day>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Session_getDay, static_cast<dxfg_session_t *>(session.get()))};
}

std::int32_t getType(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getType`. The `session` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Session_getType, static_cast<dxfg_session_t *>(session.get()));
}

bool isTrading(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_isTrading`. The `session` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Session_isTrading, static_cast<dxfg_session_t *>(session.get())) ==
           1;
}

bool isEmpty(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_isEmpty`. The `session` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Session_isEmpty, static_cast<dxfg_session_t *>(session.get())) == 1;
}

std::int64_t getStartTime(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getStartTime`. The `session` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Session_getStartTime, static_cast<dxfg_session_t *>(session.get()));
}

std::int64_t getEndTime(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getEndTime`. The `session` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Session_getEndTime, static_cast<dxfg_session_t *>(session.get()));
}

bool containsTime(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session, std::int64_t time) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_containsTime`. The `session` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Session_containsTime, static_cast<dxfg_session_t *>(session.get()),
                                              time) == 1;
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getPrevSession(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
               /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getPrevSession`. The `session` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getPrevSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Session_getPrevSession, static_cast<dxfg_session_t *>(session.get()),
                                          static_cast<dxfg_session_filter_t *>(filter.get()))};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
getNextSession(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
               /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getNextSession`. The `session` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getNextSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Session_getNextSession, static_cast<dxfg_session_t *>(session.get()),
                                          static_cast<dxfg_session_filter_t *>(filter.get()))};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findPrevSession(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
                /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_findPrevSession`. The `session` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_findPrevSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Session_findPrevSession, static_cast<dxfg_session_t *>(session.get()),
                                          static_cast<dxfg_session_filter_t *>(filter.get()))};
}

/* dxfg_session_t* */ JavaObjectHandle<dxfcpp::Session>
findNextSession(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
                /* dxfg_session_filter_t* */ const JavaObjectHandle<dxfcpp::SessionFilter> &filter) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_findNextSession`. The `session` handle is invalid");
    }

    if (!filter) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_findNextSession`. The `filter` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::Session>{
        runGraalFunctionAndThrowIfNullptr(dxfg_Session_findNextSession, static_cast<dxfg_session_t *>(session.get()),
                                          static_cast<dxfg_session_filter_t *>(filter.get()))};
}

std::size_t getHashCode(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_getHashCode`. The `session` handle is invalid");
    }

    return static_cast<std::size_t>(
        runGraalFunctionAndThrowIfMinusOne(dxfg_Session_hashCode, static_cast<dxfg_session_t *>(session.get())));
}

bool equals(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session,
            /* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &otherSession) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_equals`. The `session` handle is invalid");
    }

    if (!otherSession) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_equals`. The `otherSession` handle is invalid");
    }

    if (session.get() == otherSession.get()) {
        return true;
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_Session_equals, static_cast<dxfg_session_t *>(session.get()),
                                              static_cast<dxfg_session_t *>(otherSession.get())) == 1;
}

std::string toString(/* dxfg_session_t* */ const JavaObjectHandle<dxfcpp::Session> &session) {
    if (!session) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Session_toString`. The `session` handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(
        runGraalFunctionAndThrowIfNullptr(dxfg_Session_toString, static_cast<dxfg_session_t *>(session.get())));

    auto result = dxfcpp::toString(string.get());

    return result;
}

} // namespace IsolatedSession

namespace IsolatedSessionList {

bool release(/* dxfg_session_list* */ void *sessionList) {
    if (!sessionList) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_SessionList_wrapper_release`. The `sessionList` is nullptr");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_SessionList_wrapper_release,
                                                  static_cast<dxfg_session_list *>(sessionList));
}

std::unique_ptr<void, decltype(&IsolatedSessionList::release)> toUnique(/* dxfg_session_list* */ void *sessionList) {
    return {sessionList, IsolatedSessionList::release};
}

} // namespace IsolatedSessionList

} // namespace isolated::schedule

DXFCPP_END_NAMESPACE