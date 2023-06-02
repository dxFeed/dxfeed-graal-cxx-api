// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <graal_isolate.h>

#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

CEntryPointErrors Isolate::IsolateThread::detach() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace(toString() + "::detach()");
    }

    // OK if nothing is attached.
    if (!handle) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::detach(): !handle => Not attached");
        }

        return CEntryPointErrors::NO_ERROR;
    }

    auto result = CEntryPointErrors::valueOf(graal_detach_thread(dxfcpp::bit_cast<graal_isolatethread_t *>(handle)));

    if (result == CEntryPointErrors::NO_ERROR) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::detach(): result == CEntryPointErrors::NO_ERROR => Detached");
        }

        handle = nullptr;
    }

    return result;
}

CEntryPointErrors Isolate::IsolateThread::detachAllThreadsAndTearDownIsolate() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace(toString() + "::detachAllThreadsAndTearDownIsolate()");
    }

    if (!handle) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::detachAllThreadsAndTearDownIsolate(): !handle => Not attached");
        }

        return CEntryPointErrors::NO_ERROR;
    }

    auto result = CEntryPointErrors::valueOf(
        graal_detach_all_threads_and_tear_down_isolate(dxfcpp::bit_cast<graal_isolatethread_t *>(handle)));

    if (result == CEntryPointErrors::NO_ERROR) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::detachAllThreadsAndTearDownIsolate(): CEntryPointErrors::NO_ERROR => All "
                                         "threads have been detached. The isolate has been teared down.");
        }

        handle = nullptr;
    }

    return result;
}

std::shared_ptr<Isolate> Isolate::create() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace("Isolate::create()");
    }

    graal_isolate_t* graalIsolateHandle{};
    graal_isolatethread_t* graalIsolateThreadHandle{};

    if (CEntryPointErrors::valueOf(graal_create_isolate(nullptr, &graalIsolateHandle, &graalIsolateThreadHandle)) ==
        CEntryPointErrors::NO_ERROR) {

        auto result = std::shared_ptr<Isolate>{new (std::nothrow) Isolate{graalIsolateHandle, graalIsolateThreadHandle}};

        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("Isolate::create() -> *" + result->toString());
        }

        return result;
    }

    if constexpr (Debugger::traceIsolates) {
        Debugger::trace("Isolate::create() -> nullptr");
    }

    return nullptr;
}

CEntryPointErrors Isolate::attach() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace(toString() + "::attach()");
    }

    // We will not re-attach.
    if (!currentIsolateThread_.handle) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::attach(): !currentIsolateThread_.handle => Needs to be attached.");
        }

        graal_isolatethread_t* newIsolateThreadHandle{};

        if (auto result = CEntryPointErrors::valueOf(graal_attach_thread(dxfcpp::bit_cast<graal_isolate_t*>(handle_), &newIsolateThreadHandle));
            result != CEntryPointErrors::NO_ERROR) {

            if constexpr (Debugger::traceIsolates) {
                Debugger::trace(toString() + "::attach(): result != CEntryPointErrors::NO_ERROR [" +
                                std::to_string(result.getCode()) + "] " + result.getDescription());
            }

            return result;
        }

        currentIsolateThread_.handle = newIsolateThreadHandle;
        currentIsolateThread_.isMain = mainIsolateThread_.handle == newIsolateThreadHandle;

        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::attach(): Attached: " + currentIsolateThread_.toString());
        }
    } else {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::attach(): Cached: " + currentIsolateThread_.toString());
        }
    }

    return CEntryPointErrors::NO_ERROR;
}

GraalIsolateThreadHandle Isolate::get() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace(toString() + "::get()");
    }

    return dxfcpp::bit_cast<void*>(graal_get_current_thread(dxfcpp::bit_cast<graal_isolate_t*>(handle_)));
}

} // namespace dxfcpp