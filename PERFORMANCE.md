# Performance

This page lists what changes the throughput and the CPU cost of the C++ API, and by how much. Every number comes
from a measurement on one computer and is meant for comparison, not as a guarantee: a laptop with an Intel Core
i7-1165G7 (4 cores, 8 threads), Windows 11, the C++ API built by MSVC 2022 in Release and linked statically, the Graal
Native SDK 3.19.0, `Trade` events, November 2026. The rates, the latencies and the CPU of a process come from
`Tools NetTest` (see [NetTest](README.md#nettest)) over the loopback interface; the costs of the C++ conversions alone
come from micro-benchmarks without the SDK, and the page says so where they are used. A statement without a
measurement says so.

- [Where the time goes](#where-the-time-goes)
- [The variant of the Graal Native SDK](#the-variant-of-the-graal-native-sdk)
- [Publishing](#publishing)
- [Receiving](#receiving)
- [The memory allocator](#the-memory-allocator)
- [Build options](#build-options)
- [How to measure](#how-to-measure)

## Where the time goes

Most of the time is spent in the Graal Native SDK, not in the C++ code (the C++ part is measured by micro-benchmarks
of the conversion, without the SDK):

| Process  | Whole cost per event                 | The C++ API part                                                         |
|----------|--------------------------------------|--------------------------------------------------------------------------|
| Producer | ~700 ns in `DXPublisher::publishEvents`, batches of 100 | ~50 ns: the conversion of a batch to the native events and its release |
| Consumer | ~1800 ns of CPU                      | ~100 ns: the conversion of the native events to C++ objects and their release; two thirds of it is one allocation and one release of the event on the heap |

With one listener thread (`dxfeed.threadPoolSize=1`) the CPU of the consumer is spent by two threads: the one that
reads the socket (~850 ns per event) and the one that delivers the events (~1000 ns per event: the QD data
structures, the Java events, their mapping to the native structures, the C++ conversion and the call of the
listener). So the levers differ by side: a producer gains most from batches (2.9 times the rate from 1 to 100 events
per call, see [Publishing](#publishing)), then from the variant of the SDK (+22 %); a consumer gains most from the
variant of the SDK (−21 % CPU per event), then a little from the allocator (~2 %).

## The variant of the Graal Native SDK

The SDK built with the `-O3` optimization level is faster on both sides. The CMake option
`DXFCXX_GRAAL_NATIVE_SDK_VARIANT=oracle` takes such a build from the SDK release (see
[Graal Native SDK variants](HOW_TO_BUILD.md#graal-native-sdk-variants) for the platforms, the limits and the license).
The same `Tools.exe` with the library of each variant, three alternated rounds, medians:

| Measured value               | `ce` (the default) | `oracle`        |
|------------------------------|--------------------|-----------------|
| Producer, events per second  | 1.38M              | 1.70M (+22 %)   |
| Producer, `publishEvents` per event | 718 ns      | 584 ns          |
| Consumer, CPU per event      | 1805 ns            | 1451 ns (−21 %) |
| Consumer, latency p50        | ~450 µs            | ~340 µs         |

The library of the SDK is larger: `DxFeedGraalNativeSdk.dll` of 3.19.0 is 99 MB instead of 56 MB. The rate of the
consumer did not change (~0.8M events per second for both): there the producer and the connection were the limit.

## Publishing

- Publish in batches: pass a whole list to `DXPublisher::publishEvents`. The C++ API converts a batch of the event
  classes of the library to the native events in one block of memory and releases it once, after the SDK took the
  batch. An event of a class defined in the application (a subclass of a library event too) is converted with its own
  allocations.
- The SDK takes a batch in one call, so its cost per call is shared by the events of the batch. One producer thread
  and no consumer, three alternated rounds, medians:

  | Events per `publishEvents` call | Events per second | `publishEvents` per event |
  |---------------------------------|-------------------|---------------------------|
  | 1                               | 0.49M             | 1987 ns                   |
  | 10                              | 1.15M             | 858 ns                    |
  | 100                             | 1.40M             | 709 ns                    |

- Reuse the event objects between batches when the application can: a new event for each publication
  (`std::make_shared` and a copy) cost ~81 ns per event in a micro-benchmark of the NetTest producer code, filling an
  existing object ~24 ns. The C++ API
  copies the events into the SDK, so an object can be changed as soon as `publishEvents` returns.

## Receiving

- `dxfeed.threadPoolSize` (`DXEndpoint::DXFEED_THREAD_POOL_SIZE_PROPERTY`) sets the threads that call the listeners
  of a `FEED` endpoint, the number of processors by default. It did not change the CPU per event with one
  subscription and a trivial listener: 1762, 1796 and 1757 ns for 1, 2 and 8 threads. With one thread the listeners
  run one after another on it; with more threads the same work is spread among them.
- The listener is called with a list of events, 110–160 events per call in this test, so the cost of a call is
  negligible next to the cost of an event. Keep the listener fast: a slow listener delays all the events of its
  subscription (see [Latency](TROUBLESHOOTING.md#latency)).
- `dxfeed.aggregationPeriod` (`DXEndpoint::DXFEED_AGGREGATION_PERIOD_PROPERTY`) limits how often the listeners are
  notified: `0.1s` means at most 10 notifications per second, and the lasting events (`Quote`, `Trade`, ...) of a
  symbol are conflated between them. Use it when the application needs the latest values, not every change. Not
  measured here.

## The memory allocator

The C++ API allocates each received event with `std::make_shared`, so the allocator of the C++ runtime is a part of
the cost of receiving. Replacing `operator new` and `operator delete` of the application with
[mimalloc](https://github.com/microsoft/mimalloc) 3.6.1 (`mimalloc-new-delete.h`, MSVC `/MD`, the static library of
the C++ API linked into the executable), measured on the conversion alone, without the SDK; with the shared library
of the C++ API the effect was not checked:

| Conversion of received events, CPU per event | Default heap | mimalloc |
|----------------------------------------------|--------------|----------|
| `Trade`                                      | 105 ns       | 72 ns    |
| `TimeAndSale`                                | 127 ns       | 87 ns    |

This is ~2 % of the CPU of the consumer per event. The SDK allocates its native events with `malloc` of the C
runtime, which `operator new` does not cover: that needs a replacement of `malloc` of the whole process
(`mimalloc-redirect.dll` on Windows, `LD_PRELOAD` or linking mimalloc into the executable on Linux). It works only
for the modules that use the C runtime as a DLL (`/MD`) on Windows; a module with a static C runtime keeps its own
heap, so the SDK of `DXFCXX_LINK_STATIC_RUNTIME` is expected to stay on it (not checked). The effect of a
process-wide replacement on the whole consumer was not measured here.

## Build options

- The numbers above are of Release builds (`CMAKE_BUILD_TYPE=Release`) of the C++ API and the regular (not `-debug`)
  SDK.
- Keep `DXFCXX_ENABLE_METRICS` off (the default). When it is on, every creation and destruction of an event takes a
  global mutex and builds a counter name. Not measured here.

## How to measure

`Tools NetTest` measures the rate, the CPU and the latency of each side and writes them to a summary file. Run the
producer and the consumer as separate processes, warm up first, and alternate the variants that you compare, several
rounds of each, taking the median:

```shell
Tools NetTest p :7001[bindAddr=127.0.0.1] -S 1000 -C 1 --batch 100 --warmup 5s --duration 25s --summary producer.json
Tools NetTest c 127.0.0.1:7001 -S 1000 --warmup 5s --duration 20s --summary consumer.json
```

`publish_call.ns_per_event_mean` of the producer summary is the time of `publishEvents` per event.
`runtime.cpu_percent_mean` is the CPU of the process in percent of one core, so the CPU per event in nanoseconds is
`cpu_percent_mean * 10^7 / rate`. `System::getRuntimeStatistics()` gives the heap
and the garbage collections of the isolate, to log next to the measured values.
