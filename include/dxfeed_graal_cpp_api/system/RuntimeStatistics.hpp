// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string>

/**
 * \addtogroup dxfcpp_system
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * The runtime statistics of the Graal isolate of the library and of its process: the heap, the garbage collections,
 * the threads, the CPU time of the process and the uptime (see System::getRuntimeStatistics()).
 *
 * Every value is `-1` when the platform or the garbage collector does not give it ("not given"), otherwise it is `0`
 * or more. The heap pool and GC values follow the Serial GC, the GC of the Graal Native SDK builds; a build with
 * another GC (G1) has no such pools and collectors, and these values are `-1` there. `allocatedBytes` is `-1` with
 * GraalVM 25.5. A default-constructed object has all the values `-1`.
 *
 * Each value is read on its own, so the statistics are not one atomic snapshot: a heap value and a GC count may come
 * from moments around one collection. The counters are cumulative since the isolate started: compute the change
 * between two calls. `processCpuTimeNs` and `availableProcessors` belong to the whole process, the other values to the
 * isolate.
 */
struct DXFCPP_EXPORT RuntimeStatistics {
    /// The bytes of the heap of the isolate in use, garbage not yet collected included.
    std::int64_t heapUsedBytes = -1;

    /// The bytes of the heap of the isolate taken from the system, `heapUsedBytes` or more.
    std::int64_t heapCommittedBytes = -1;

    /// The bytes the heap of the isolate may grow to (`-Xmx`, by default a part of the physical memory).
    std::int64_t heapMaxBytes = -1;

    /// The incremental (young generation) collections of the isolate since it started; `-1` on G1.
    std::int64_t youngGcCount = -1;

    /// The total time of the incremental collections, in milliseconds; `-1` on G1.
    std::int64_t youngGcTimeMs = -1;

    /// The collections of the whole heap of the isolate since it started; `-1` on G1.
    std::int64_t fullGcCount = -1;

    /// The total time of the collections of the whole heap, in milliseconds; `-1` on G1.
    std::int64_t fullGcTimeMs = -1;

    /// The live Java threads of the isolate: its own threads and the attached native threads.
    std::int64_t threadCount = -1;

    /// The peak of `threadCount` since the isolate started.
    std::int64_t peakThreadCount = -1;

    /// The CPU time of the whole process, user plus system, in nanoseconds: every isolate of the process reports the
    /// same value. The step is the one of the system (on Windows 15.625 ms).
    std::int64_t processCpuTimeNs = -1;

    /// The time since the isolate started, in milliseconds.
    std::int64_t uptimeMs = -1;

    /// The bytes allocated by all threads of the isolate since it started; `-1` with GraalVM 25.5.
    std::int64_t allocatedBytes = -1;

    /// The bytes in use in the eden space of the young generation; `-1` on G1.
    std::int64_t edenUsedBytes = -1;

    /// The bytes in use in the survivor space of the young generation; `-1` on G1.
    std::int64_t survivorUsedBytes = -1;

    /// The bytes in use in the old generation: its growth across collections shows objects that stay alive; `-1` on
    /// G1.
    std::int64_t oldUsedBytes = -1;

    /// The live daemon threads of the isolate; the attached native threads count as daemon threads.
    std::int64_t daemonThreadCount = -1;

    /// The Java threads started or attached in the isolate since it started.
    std::int64_t totalStartedThreadCount = -1;

    /// The processors available to the process, container limits considered: divide a CPU load by it per core.
    std::int64_t availableProcessors = -1;

    /**
     * Returns a string representation of the statistics in one line: every value with its name, `-1` as is.
     *
     * @return The string representation of the statistics.
     */
    std::string toString() const;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
