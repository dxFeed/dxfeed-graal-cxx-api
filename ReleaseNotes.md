* **\[MDAPI-454]\[C++]** With `DXFCXX_WARNINGS_AS_ERRORS=ON` the use of deprecated functions stays a warning instead of
  failing the build (MSVC: C4996 is disabled).
* **\[MDAPI-449]\[C++]** Added `IndexedEventTxModel` (the incremental model of indexed events of dxFeed Java API) with
  its builder (feed, symbol, source, listener, events batch limit, aggregation period, executor), and
  `SubscriptionController` (attach, detach, events batch limit, aggregation period, executor) that
  `IndexedEventTxModel::getSubscriptionController()` returns. Added `SimpleOrderBookSample`.
* **\[MDAPI-450]\[C++]** Added the client of the Order Reconstruction Service (ORCS), as in the .NET API:
  `PriceLevelService` (`create()`, `getOrders()`, `getQuotes()`, `getAuthOrderSource()`, `close()`),
  `AuthOrderSource` (the authorized order sources and their symbols) and `PriceLevelChecker::validate()`.
  Added `PriceLevelServiceClientSample`.
* **\[MDAPI-432]\[C++]** **\[BREAKING]** Fixed equality, type checks, the market depth model and the initialization
  of the constants.
    * **\[BREAKING]** Fixed inverted equality of `AuthToken`, `CFI`, `CFI::Attribute`, `CFI::Value`, `PriceIncrements`,
      `AdditionalUnderlyings`, `InstrumentProfile` and the tx model listeners: `operator==` returned `false` for equal
      objects and `true` for different ones. Comparing a `CFI`, `PriceIncrements` or `AdditionalUnderlyings` with
      itself no longer deadlocks.
    * **\[BREAKING]** `SharedEntity::is<T>()` returns `true` only if the entity is a `T` or derived from `T` (it always
      returned `true`).
    * `EventTypeEnum::isMarket()` returns whether the events are market events, as in Java (it returned
      `isOnlyIndexed()`, so `false` for `QUOTE`, `TRADE`, `GREEKS` and other market events).
    * `InstrumentProfile::setDateField()` stores a date (`2024-01-01`) in a custom field instead of a number of days
      (`19723`), which `getDateField()` could not read back.
    * `MarketDepthModel`: a new snapshot of a source (for example, after a reconnection) removes all the old orders of
      that source: some of them were left in the book, and removing the last order of the book was undefined
      behaviour. A model built without a listener no longer crashes on the first update.
    * The constants of the enums, candle attributes, schedule filters, event types, event flags, order sources,
      `InstrumentProfileField` and `WildcardSymbol::ALL` are constant-initialized: they can be used in the
      initializers of global variables of user code, with the static and the shared library (`CandleType::parse("h")`
      in such an initializer crashed). The `EMPTY` constants of `CFI`, `PriceIncrements` and `AdditionalUnderlyings`
      are still built at dynamic initialization.
    * **\[BREAKING]** The constants that are built at run time are functions with the same names: `String::NUL()`,
      `String::EMPTY()`, `CandleSymbol::NUL()`, `ATTRIBUTE_KEY()` of the candle attributes, `BY_STRING()` of
      `CandleType`, `CandlePrice`, `CandleAlignment` and `CandleSession`, `EventTypeEnum::ALL_BY_NAME()`,
      `ALL_BY_CLASS_NAME()` and `ALL_BY_ID()`, `OrderSource::PREDEFINED_SOURCES()` and
      `WildcardSymbol::RESERVED_PREFIX()`. Each returns a reference to an object that is created at the first call and
      is never destroyed, so it can be used in the constructors and the destructors of global variables.
    * **\[BREAKING]** The name getters of these constants return `std::string` by value instead of
      `const std::string &`: `getName()` and `toString()` of the enums, `EventFlag`, `CandleType` and `SessionType`,
      `InstrumentProfileField::getName()`, `EventTypeEnum::getName()` and `getClassName()`,
      `IndexedEventSource::name()`, `toString()` of `CandlePeriod`, `CandlePrice` and `CandleSession`, and
      `WildcardSymbol::getSymbol()`.
    * **\[BREAKING]** `EventTypeEnum::ALL` and the `VALUES` of `CandleType`, `CandlePrice`, `CandleAlignment` and
      `CandleSession` are `std::span<const std::reference_wrapper<const T>>` (they were `std::vector`). Each enum
      (`Side`, `Scope`, `OrderAction`, `InstrumentProfileType` and others) has a public `ALL` with its elements.
    * `valueOf()` of the enums (`Side::valueOf()` and others) and the `_et`, `_ws` and `_wcs` literals link with the
      shared library (DLL) on Windows.
    * The library, the tests and the samples build with clang-cl (the `ClangCL` toolset of Visual Studio), which is
      checked by CI.
    * **\[BREAKING]** ABI: the layouts of these classes changed, `SessionFilter` and `DayFilter` no longer have a mutex
      member, the `OrderSource` user sources are no longer static members, and the names of `IndexedEventSource`,
      `OrderSource`, `CandlePeriod` and `SessionFilter` are stored in the new internal `ConstString`.
* **\[MDAPI-430]\[C++]** **\[BREAKING]** Candle attributes are parsed and printed as in Java.
    * **\[BREAKING]** `CandlePeriod::parse()` and `CandlePriceLevel::parse()` follow Java `Double.parseDouble`: a sign
      (`+5m`), the `d`/`f` suffix and hexadecimal values (`0x1p3`) of a price level are accepted, and parsing no longer
      depends on the C locale. An invalid number throws `InvalidArgumentException`.
    * `CandlePeriod::toString()` and `CandlePriceLevel::toString()` print values as Java does (`1.0E19m`, `1.0E-5m`,
      `NaNm`, `Infinitym`). Values beyond the range of `std::int64_t` previously printed as wrong numbers.
    * `CandlePeriod::getPeriodIntervalMillis()` saturates as the Java `(long)` cast does (NaN gives 0) instead of
      causing undefined behaviour for large periods such as `1e30m`.
    * Fixed a data race in `CandlePeriod::toString()`: the string is computed in the constructor.
    * **\[BREAKING]** `CandlePeriod::valueOf()` and the `CandlePeriod` default constructor are no longer `noexcept`
      (they build the string representation).
    * `CandleSession()` is `CandleSession::ANY`: comparing a default-constructed `CandleSession` dereferenced a null
      pointer.
    * `CandleSymbol` normalization is idempotent: a default attribute is kept when removing it would leave a base symbol
      that itself ends with attributes (`A{=t}{=t}`, `A{=t}{=1t}` gives `A{=t}{=t}`). The session attribute is normalized
      as it is read (Java `Boolean.parseBoolean`): `A{tho=t}` gives `A` (any session) instead of `A{tho=true}`.
      Unlike Java, normalization still removes the default attributes (`IBM{=t}` gives `IBM`).
    * **\[BREAKING]** Copying and moving a `CandleSymbol` copy its fields instead of parsing the symbol again; these
      operations are no longer `noexcept`.
    * `RuntimeException` (and so every exception of the API) formats its stacktrace on the first `getStackTrace()` call
      instead of when it is thrown: with `DXFCXX_FEATURE_STACKTRACE` each exception resolved the names of all frames
      (on Linux by running `addr2line`), which made each throw slow (up to tens of seconds in a Debug build with
      ASan). The copies of an exception share the stacktrace.
    * A stacktrace frame without a path (`??:0`, a relative file name) prints its location instead of the whole frame.
* **\[MDAPI-429]\[C++]** **\[BREAKING]** Fixed `noexcept` functions that threw and the shared library (DLL) on Windows.
    * **\[BREAKING]** `OrderBase::getSource()`, `OrderBase::setExchangeCode(std::int16_t)` and the `withIndex()` /
      `withExchangeCode()` methods of `Order`, `SpreadOrder`, `AnalyticOrder` and `OtcMarketsOrder` are no longer
      `noexcept` (`IndexedEvent::getSource()` too). An invalid source id in the index, an invalid exchange code or a
      negative index now throws `InvalidArgumentException` (as Java throws `IllegalArgumentException`) instead of
      terminating the process.
    * **\[BREAKING]** `CmdArgsUtils::parseEventSources()` is no longer `noexcept`. An invalid source name now throws
      `InvalidArgumentException` instead of terminating the process.
    * `StringLike`, `StringHash`, `EventFlagsMask`, `DXFeedTimeSeriesSubscription` and the `_c` literal are exported
      from the shared library. An application that uses the DLL did not link calls with `const char*` and `std::string`
      arguments, `EventFlagsMask::contains()` and `DXFeedTimeSeriesSubscription::getFromTime()` / `setFromTime()`.
    * Fixed the shared library on Windows: the subscriptions and the listeners of the tx models that the code of the
      public headers creates were registered in managers of the application instead of the library, and the ids of the
      listeners added by the application and by the library collided, so these listeners were not called. The managers
      are kept in the library; the listener ids are unique within a handler (`Handler`, `SimpleHandler`).
* **\[MDAPI-305]\[C++]** The Windows `-static-mt` archives need no Visual C++ Redistributable, and a single exe or
  dll can be built from them. See "Windows without the Visual C++ Redistributable" in `README.md`.
    * The archives take the SDK DLL built with the static C runtime, and the C++ API DLL is built with `/MT` (`/MTd`).
    * The archives add `lib\DxFeedGraalNativeSdk.lib`, `lib\DxFeedGraalNativeSdk_static.lib` and
      `lib\link-flags.txt`: `dxFeedGraalCxxApi_static.lib` links either with the SDK DLL or with the static SDK
      library into one exe or dll that depends only on Windows DLLs.
    * Code built with `/MT` shall not link with `dxFeedGraalCxxApi.dll` of these archives: the DLL and the code each
      have their own C runtime, and the C++ API passes standard library objects between them.
    * The Windows archives are built with Visual Studio 2022 17.14 (v8.1.0 was built with Visual Studio 2026), so
      their static libraries link with Visual Studio 2022 17.14 and newer.
    * `StringLike`, `StringHash`, `EventFlagsMask`, `operator""_c` and `DXFeedTimeSeriesSubscription` are exported
      from `dxFeedGraalCxxApi.dll`: a program that used them with the DLL failed to link.
    * The release workflow checks the archives: no DLL imports the dynamic C runtime, and a program linked in each of
      the two ways runs in Windows Server Core without the Redistributable.
    * Known issue: a process that uses `dxFeedGraalCxxApi.dll` on Windows does not exit after `main()` returns.
* **\[MDAPI-438]\[C++]** Migrated to Graal SDK v3.9.0 (QDS 3.356).
    * Added `DXEndpoint::Builder::withExecutor()`: it sets the executor of the endpoint's notifications before the
      endpoint is built. `DXEndpoint::executor()` still works but is deprecated: called after `DXEndpoint::getFeed()`
      or `DXEndpoint::getPublisher()`, it leaves the feed with the default executor, which is shut down, and the last
      event promises (`DXFeed::getLastEventPromise()`, etc.) never complete (QD-1795). Until the migration, call it
      right after the endpoint is created. Its first call in a process logs a deprecation warning of the SDK.
    * QD: absolute Windows paths are accepted in `tape:` addresses (QD-1796); concurrent creation of endpoints with
      the `ext` scheme no longer fails (QD-1794); `MarketMaker` can be written to XML (QD-1799).
    * QD, incompatible: the `TimeAndSale` time in the XML format of tapes is an ISO 8601 date and time
      (`2025-10-02T09:00:00.000Z`) instead of milliseconds, as in the other events (QD-1798).
* **\[MDAPI-435]\[C++]** Portals (used by `MarketDepthModelSample`) is updated to v0.2.0 from its new repository
  (the old one was renamed, and the `v0.1.1` fallback no longer resolves): `third_party/portals-0.2.0`, or the pinned
  v0.2.0 commit when it is not vendored. The sample uses the `portals::legacy` API and is built with C++23. Portals is
  configured only for the samples: when CMake is older than 3.24 or the standard library lacks the C++23 and C++20
  parts it uses (`std::expected`, `std::format`, `std::move_only_function`, `std::stop_token`; Apple libc++ of
  Xcode 16 lacks the last two), `MarketDepthModelSample` is skipped with a status message. The library still needs
  CMake 3.21 and C++20. [#125](https://github.com/dxFeed/dxfeed-graal-cxx-api/issues/125)
* **\[MDAPI-434]\[C++]** Fixed `MarketDepthModel` with a nonzero aggregation period: `close()` now cancels the
  pending notification, the listener is not called after `close()` or after the model is destroyed, and the listener
  can call the model (`close()` included) without a deadlock. `Timer` no longer leaks itself and the state of its
  task (each aggregated notification leaked one): the task runs on a detached thread.
  [#124](https://github.com/dxFeed/dxfeed-graal-cxx-api/issues/124)
* **\[MDAPI-433]\[C++]** Added `scripts/release.cmake` (CMake only) and `scripts/release.py` (Python), which prepare a
  release: they set the version in all the places and the heading of `ReleaseNotes.md` (for the pre-releases `-alphaN`,
  `-betaN`, `-preN` and `-rcN` too), commit and tag it, and with `--push` push the branch and the tag; `-draftN` is a
  trial run of the release workflow. The text of a GitHub release is now the section of its version in
  `ReleaseNotes.md`. See `HOW_TO_RELEASE.md`.

## v8.1.0

* **\[MDAPI-427]\[C++]** Improved the quality infrastructure and the build.
    * Sanitizers now instrument both the shared and the static library and all their consumers (tests, samples,
      tools). ASan and UBSan can be enabled together. Added ThreadSanitizer support (`DXFCXX_ENABLE_TSAN`).
      The internal CMake functions `LinkAsan` and `LinkUbsan` were replaced by the `dxfcxx_sanitizers` target.
    * Public headers are now self-contained. The build with `DXFCXX_USE_PRECOMPILED_HEADERS=OFF` is fixed.
      Added the `DXFCXX_BUILD_HEADER_CHECK` option that compiles every public header on its own.
    * Unit tests no longer need external network and can run in parallel. The tests against `demo.dxfeed.com` are built
      with the new `DXFCXX_ENABLE_NETWORK_TESTS` option (CTest label `network`).
    * Unit tests compare the events of all types, the option chains of an IPF file and the tx model scenarios with
      the Java API through golden QD tapes (`tests/data`).
    * Fixed `DXFeed::getLastEventIfSubscribed()` and `DXFeed::getLastEvent()`: they crashed (null dereference) for a
      symbol without a subscription; now `getLastEventIfSubscribed()` returns `nullptr` and `getLastEvent()` leaves
      the event unchanged, as documented.
    * Fixed `DXFeed::getLastEvents()`: it did not compile for any collection (the element type was deduced as the
      iterator type). It accepts a `LastingEventCollection` (a collection of `std::shared_ptr` of lasting events) and
      returns a reference to a named collection and a temporary collection by value, so the result of a temporary can
      be used in a range-based for loop without a dangling reference.
    * Added libFuzzer targets for the parsers, string utilities and packed event fields (`fuzz/`, the
      `DXFCXX_BUILD_FUZZERS` option, LLVM Clang only) and the "Fuzzing" CI workflow in report mode: 60 s per target on
      pull requests, 30 min nightly.
    * The UI samples on Linux are built with the X11 backend of GLFW, the Wayland backend or both, whichever
      development packages are installed (it was always X11, and Wayland only with
      `DXFCXX_SAMPLE_GLFW_BUILD_WAYLAND=ON`, which now makes Wayland required); GLFW selects the backend at run time.
    * Added the `DXFCXX_BUILD_UI_SAMPLES` option (`AUTO` by default, `ON`, `OFF`). With `AUTO`, the UI samples are
      skipped with a status message instead of failing the configuration when OpenGL or both X11 and Wayland are
      missing on Linux.
    * GCC and Clang now compile with `-std=c++20` instead of `-std=gnu++20` (`CMAKE_CXX_EXTENSIONS OFF`; the previous
      `CXX_EXTENSIONS OFF` had no effect).
    * The configuration fails with a clear message for a platform without Graal Native SDK archives (it went on and
      failed to download a nonexistent archive).
    * Fixed a race in parallel builds: every test (and fuzzer, and both libraries) copied the Graal Native SDK library
      into the same directory after it was linked, and a copy sometimes failed with "No such file or directory". The
      libraries are now copied once per directory by one target (`dxfcxx_copy_runtime`).
    * The downloaded archives (Graal Native SDK, fmt, Boost, GLFW, Dear ImGui) are verified with SHA-256, also in the
      Full Source Bundle script. nanobench is fetched by commit. GitHub Actions are pinned by commit SHA and updated by
      Dependabot; the CI workflow has read-only repository permissions.
    * portals (used by `MarketDepthModelSample`) is updated to v0.1.1 (`third_party/portals-0.1.1`, or the `v0.1.1`
      tag instead of the default branch when it is not vendored); its new dependency bits v1.0.0 is vendored in
      `third_party/bits-1.0.0`.
    * The bit operations of `internal/Common.hpp` (`sal`, `sar`, `shl`, `shr`, `andOp`, `orOp`, `xorOp` and the
      others, still in `dxfcpp`) come from bits v1.0.0, a header-only dependency of the public headers now: its
      headers are installed with the library (`include/bits`). A shift by the minimum value of a signed shift type
      (for example `sal(x, INT32_MIN)`) is no longer undefined behavior (it recursed infinitely); the results of all
      other shifts are unchanged.
    * The packages no longer contain the headers and CMake package configs of the header-only dependencies that
      installed themselves: bits (also in the Samples and Tools packages) and config (a dependency of the library
      sources only).
    * The library, tests, samples, tools and fuzzers are compiled with `-Wshadow`, `-Wsign-conversion`,
      `-Wfloat-equal` and `-Wextra-semi` (MSVC: C4365), and their warnings are fixed; the CI builds treat the
      warnings as errors (the new `DXFCXX_WARNINGS_AS_ERRORS` option, `OFF` by default). The warnings are not passed to
      the projects that use the library, and the headers of the dependencies are system headers. clang-tidy fails the
      CI job on `bugprone-use-after-move` and `clang-analyzer-*` findings.
    * Added the "Static analysis" CI workflow: clang-tidy (LLVM 23, the bug-oriented profile in `.clang-tidy`) and
      cppcheck (2.22) on the library sources in report mode, with the findings counted in the job summaries.
    * Unit tests compare the edge values of all event types (NaN, ±0.0, infinities, number limits, special strings,
      all enum values, all event flags), the order sources of invalid ids and names and the events that the publisher
      rejects with the Java API.
    * Fixed a signed integer overflow (undefined behavior) in the time conversions of events with times near the
      `int64` minimum (`math::floorMod`, found by UBSan in the edge-value tests). `math::floorDiv` and
      `math::floorMod` now return the results of Java `Math.floorDiv` and `Math.floorMod` for all arguments,
      including `INT64_MIN / -1`, and throw `InvalidArgumentException` for a zero divisor.
    * The exception headers no longer include `internal/Common.hpp` (they include `internal/utils/StringUtils.hpp`),
      so `Common.hpp` can use the exceptions. Code that got `Common.hpp` only through an exception header must include
      it.
* Migrated to Graal SDK v3.8.0.
    * Linux and macOS: the process is no longer killed by `SIGPIPE` when an endpoint is disconnected or closed while
      its connector writes to the socket. When the isolate is created and the action of `SIGPIPE` or `SIGXFSZ` is the
      default one, the SDK installs a no-op handler (as the JVM does), so such a write fails with `EPIPE`; a handler
      or `SIG_IGN` set by the application before the first API call is kept.
    * Fixed memory leaks of the SDK: the strings of `NuamOrder` and the method names of the stack trace of an
      exception were not freed.
    * macOS: the SDK library declares macOS 14.0 on arm64 and 11.0 on x64 as its minimum version (it was the version
      of the build machine, macOS 26.0, so the linker warned for an application built for an older macOS).
    * The attachment of `Message` is transferred in both directions: a `Message` published from C++ keeps its
      attachment, and a received attachment is the string as is (it was JSON before: `"text"` with the quotes, the
      string `null` for no attachment). A non-string Java attachment is received as its `toString()`.
    * `Promise::awaitWithoutException()` now returns `false` when the wait times out (the promise is cancelled then);
      it returned `true` before.
    * The QD `TimeSyncTracker` (UDP multicast to `239.192.51.45:5145` from every process) is disabled by default. Set
      the system property `com.dxfeed.sdk.TimeSyncTracker.enable` to `true` before creating the first endpoint to
      enable it.
    * Connector properties in addresses (for example, `:7700[bindAddr=127.0.0.1]`) no longer fail with
      `MissingReflectionRegistrationError`.
    * A native listener is no longer called after its handle has been released.
* **\[MDAPI-85]\[C++]** Added DXFeedTimeAndSales API sample.
* **\[MDAPI-424]\[C++]** Fixed `OptionSeries` floating-point equality, ordering, and hashing
  to match Java semantics. Multiplier and SPC values now distinguish `-0.0` from `+0.0`, treat all NaN values as
  equal, and order NaN after positive infinity; NaN values are canonicalized for consistent hashing.
* Fixed `JavaHandle` leaks.
* Fixed `ConvertTapeFileSample`: the input and output addresses are now taken from the first and the second command
  line arguments; the sample used the program path as the input address before.
* **\[MDAPI-425]\[C++]** Improved lifecycle management for native callback-backed entities.

## v8.0.0

* **\[MDAPI-423]\[C++]** **\[BREAKING]** Fixed Java-to-C++ porting errors in packed event fields, native
  conversions, string handling, and candle attributes.
    * `setSequence()` and `OrderBase::setSource()` now preserve unrelated packed bits; `Message` native conversion
      preserves event time; `StringLike` copy/move operations and native C-string handling are now safe.
    * Candle equality, formatting, price-level validation/parsing, and session parsing now match Java QD semantics.
      Price levels reject negative values, negative zero, infinities, and trailing parse data; floating-point equality
      follows Java NaN/signed-zero rules; only case-insensitive `"true"` enables the regular candle session.
    * APIs that can fail validation or parsing are no longer `noexcept`; invalid input now throws an exception instead
      of terminating the process.
* Migrated to Graal SDK v3.2.13.

## v7.0.0

* **\[MDAPI-417]** **\[BREAKING]** All methods of the `Order` class and its descendants whose names begin with "with" are made virtual.
* **\[MDAPI-416]** **\[BREAKING]** All "publish" methods of the `DXPublisher` class are now const and can throw exceptions.
* The project now depends on the zip bundles of the dxFeed Graal Native SDK library, which are located in GitHub Releases.
* **\[BREAKING]** Project build speed has been improved. The implementation of all non-template methods of non-template classes is now located in .cpp files.
* Improved documentation. Classes are now divided into "modules". `Main Page - Topics - dxFeed Graal C++ API Modules.`

## v6.0.0

* **\[MDAPI-405]** Fixed project build errors that occurred when attempting to build a project using clang 19+.
* **\[MDAPI-406]** Fixed linking errors where some static fields could be uninitialized.
* **\[MDAPI-411]** Fixed the `toString()` method of the Summary event.
* Migrated to Graal SDK v3.2.0.
* **\[BREAKING]** Fixed a sporadic build error where the compiler failed to pick the correct method implementation when only a single event type was specified during subscription creation.
  These methods are now const:
  * `template <typename EventTypeIt> std::shared_ptr<DXFeedSubscription> createSubscription(EventTypeIt begin, EventTypeIt end) const;`
  * `template <typename EventTypesCollection> std::shared_ptr<DXFeedSubscription> createSubscription(const EventTypesCollection &eventTypes) const;`
  * `template <typename EventTypeIt> std::shared_ptr<DXFeedTimeSeriesSubscription> createTimeSeriesSubscription(EventTypeIt begin, EventTypeIt end) const;`
  * `std::shared_ptr<DXFeedTimeSeriesSubscription> createTimeSeriesSubscription(std::initializer_list<EventTypeEnum> eventTypes) const;`
  * `template <typename EventTypesCollection> std::shared_ptr<DXFeedTimeSeriesSubscription> createTimeSeriesSubscription(const EventTypesCollection &eventTypes) const;`
* Added the ability to collect and register metrics. Enabled by the DXFCXX_ENABLE_METRICS CMake option. Documentation: TBD.
* Added CMake option `DXFCXX_MSVC_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR`.  
  This option enables MSVC's `_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR` workaround for compatibility with older VC++ runtime 
  libraries. It should normally remain disabled and is intended only for environments that experience crashes caused by
  mixing newer STL headers with older `msvcp140.dll` versions.  
  See:
  * https://developercommunity.visualstudio.com/t/Invalid-code-generation-in-release-1940/10678572?sort=newest&viewtype=all
  * https://github.com/microsoft/STL/wiki/VS-2022-Changelog#vs-2022-1710
  * https://github.com/microsoft/STL/pull/3824
  * https://github.com/actions/runner-images/issues/10004 


## v5.0.0

* **\[MDAPI-262]\[C++]\[Linux]** Shared libraries now compiled with `noexecstack` flag.
* **\[MDAPI-238]\[С++]** Added the ability to set Java System properties.
    - The sources of system properties are:
        - Environment Variables.
            - All keys must start with the prefix `DXFEED_`, e.g. `DXFEED_log.file=log.txt`,
              `DXFEED_dxscheme.bat=millis`, etc.
            - This allows overriding a property without changing the files, in fact it is analogous to
              `java -Dkey=value -jar app.jar`.
            - In operating systems such as Linux and macOS, you can pass properties in the following ways:  
              `env DXFEED_key1=value1 DXFEED_key2=value2 ./app`
            - In Windows, properties can be passed using the following syntax:  
              `set DXFEED_key1=value1 && set DXFEED_key2=value2 && app.exe`,
              but be careful, this will set environment variables globally for the open terminal, to avoid setting
              properties globally, you can use the following method:
              `cmd /v/c "set DXFEED_key1=value1 && set DXFEED_key2=value2 && app.exe"`
        - `dxfeed.system.properties`
            - This file follows the `java.properties` or `INI` format and is generally compatible if you don't go into
              detail.
            - The path to this file can be specified by setting the environment variable
              `DXFEED_dxfeed.system.properties`.
            - By default, this file is searched for in the current runtime directory.
    - Priority and Overrides:
        - The configuration sources are applied in the following order, with each successive source overwriting values
          from the previous one:
            - `dxfeed.system.properties`
            - Environment Variables
            - Direct calls to `System::setProperty` in source code, in the order in which they were called.

          This order ensures that the most specific configuration settings are applied last, allowing for effective
          overrides and precise control over system properties.
* **\[MDAPI-178]\[C++]** Added com.dxfeed.glossary package
    * Added `AdditionUnderlyings` class.
    * Added `CFI` class.
    * Added `PriceIncrements` class.
    * Added `RoundingMode` enum.
* **\[BREAKING]** The `Day::getHashCode` method renamed to `Day::hashCode`.
* **\[BREAKING]** The `Session::getHashCode` method renamed to `Session::hashCode`.
* **\[BREAKING]** The `DXEndpoint::getEventTypes()` method now can throw exceptions.
* **\[BREAKING]** Most of the input string arguments in class methods have been replaced with a `StringLike` wrapper for
  unification.
* Fixed the naming of endpoints.
* Project build has been speeded up.
* The project has been migrated to fmtlib v12.1.0.
* **\[Linux]\[Windows]\[x64]** Added the ability to link with the debug version of the dxFeed Graal Native SDK on Linux x86_64 and Windows x86_64.
  This linking can be enabled using the `DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK` option.
* **\[MacOS]\[BREAKING]** We're now building the project on MacOS using Xcode 16.4.
  You can build the project yourself from source code using earlier versions.

## v4.3.1

* **\[MDAPI-275]\[C++]** Fixed compilation of code using `DXFeed::getLastEventsPromises()` method.
    * Added unit-tests for `DXFeed::getLastEventsPromises()` method.
    * Fixed documentation for `DXFeed::getLastEventsPromises()` method.
    * Added `testQuotePromises` function to the `QuoteAndTradeSample` to demonstrate how to use
      `DXFeed::getLastEventsPromises()`.

## v4.3.0

* **\[MDAPI-108]\[С++]** Added CandleWebService API sample.
* **\[MDAPI-253]\[C++]** Implemented HistoryEndpoint
    * Added `HistoryEndpoint` class.
    * Migrated to Graal SDK v2.6.2
* Renamed `DXFCXX_NODEFAULTLIB_LIBCMT` to `DXFCXX_NODEFAULTLIB` CMake project option for clearer purposes and added
  support for additional libraries in `/NODEFAULTLIB` configuration.
* **\[MDAPI-254]\[C++]** Migrated to Graal SDK v2.5.0
    * Added `AGGREGATE`, `COMPOSITE`, `REGIONAL` OrderSource. The new system property
      `dxscheme.unitaryOrderSource=true|false` has been added.
      It controls whether a single or unitary source is used when subscribing to all sources. It is set to 'false' by
      default.
      All separate sources, such as `COMPOSITE_ASK`, `COMPOSITE_BID`, `REGIONAL_ASK`, `REGIONAL_BID`, `AGGREGATE_ASK`
      and `AGGREGATE_BID` have been
      declared deprecated.
    * Added new Order source for BlueOcean ATS: ocea.
    * Added new Order sources for IG CFDs Gate: IGC, igc.
    * Added new Order sources for EDX Gate: EDX, edx.
    * Added new Order sources for Nuam Exchange Gate: NUAM, nuam.
* Added the ability to automatically generate the DXEndpoint name.
  If the user does not explicitly specify the endpoint name, it will be generated using the template `qdcxx{Id}`,  
  where `{Id}` will be an empty string for the first instance of the endpoint and "-2", "-3", etc. for subsequent
  instances.

## v4.2.0

* **\[MDAPI-249]\[C++]** Transitive dependencies are hidden.
* Fixed a segfault related to linking features: added a default constructor for `ApiContext`.
* **\[MDAPI-246]\[С++]** Added an ability to build the project with statically linked runtime libraries with Visual
  Studio.
* Fixed dynamic linking under Windows. These classes and functions are affected:
    * `PromiseImpl`, `VoidPromiseImpl`, `EventPromiseImpl`, `EventsPromiseImpl`, `PromiseListImpl`
    * `isolated::internal::IsolatedTools::parseSymbols`, `isolated::internal::IsolatedTools::parseSymbolsAndSaveOrder`,
      `isolated::internal::IsolatedTools::runTool`
    * All C-API functions.
* Added the ability to enable dynamic linking of tests using the CMake parameter `DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS`
* Added the ability to enable dynamic linking of samples using the CMake parameter `DXFCXX_DYNAMICALLY_LINK_SAMPLES`
* Added the ability to enable dynamic linking of tools using the CMake parameter `DXFCXX_DYNAMICALLY_LINK_TOOLS`
* Added the ability to statically link Windows runtime libraries (for Visual Studio) using the
  `DXFCXX_LINK_STATIC_RUNTIME` parameter
* Added the ability to ignore the libcmt/libcmtd library when statically linking Windows runtime libraries using the
  `DXFCXX_NODEFAULTLIB_LIBCMT` parameter (works only when the `DXFCXX_LINK_STATIC_RUNTIME` parameter is enabled)
* Added preparation of library releases for Windows, linked statically with runtime libraries (with parameters: `/MT`,
  `/MTd`, `/NODEFAULTLIB:LIBCMT`, `/NODEFAULTLIB:LIBCMTD`). The names of these artifacts end with `-static-mt`, for
  example `dxFeedGraalCxxApi-v4.2.0-rc1-x86_64-windows-Release-static-mt.zip`
* **\[MDAPI-248]\[C++]** Now documentation is automatically collected and published on GitHub Pages and released.

## v4.1.0

* **\[MDAPI-35]\[C++]** Added RequestProfileSample example
* **\[MDAPI-30]\[C++]** Added LastEventsConsoleSample sample
* **\[MDAPI-27]\[C++]** Implemented OptionChain
    * Added `OptionChain` class.
    * Added `OptionSeries` class.
    * Added `OptionChainsBuilder` class.
    * Added `OptionChainSample` example.
* **\[MDAPI-77]\[C++]** Added support for retrieving indexed events from feed
* **\[MDAPI-78]\[C++]** Added support for retrieving time-series events from feed
* **\[MDAPI-36]\[C++]** Implemented DXFeedTimeSeriesSubscription
    * Added `DXFeedTimeSeriesSubscription` class.
    * Added `DXFeed::createTimeSeriesSubscription` methods.
* **\[MDAPI-214]\[C++]\[Console]** Added MarketDepthModelSample
    * Added `MarketDepthModelSample`.
    * Added `CmdArgsUtils::parseEventSources` method.
* **\[MDAPI-216]\[C++]\[Console]** Added PriceLevelBook sample
* **\[MDAPI-76]\[C++]** Implemented TextMessage event
* Migrated to Graal SDK v2.2.1
* **\[MDAPI-214]\[C++]\[Console]** Added MarketDepthModelSample
    * Added `MarketDepthModelSample`.
    * Added `CmdArgsUtils::parseEventSources` method.

## v4.0.0

* **\[MDAPI-38]\[C++]** Added MultipleMarketDepthSample sample
    * Added `MultipleMarketDepthSample` sample.
    * Added `AuthSample` sample.
    * Added `ReconnectSample` sample.
    * Added `IncOrderSnapshotSample` sample.
* Migrated to Graal SDK v2.1.2
* **\[MDAPI-211]\[C++]** Implement logging management
    * Added `Logging` class.
* **\[MDAPI-82]\[C++]** Implement MarketDepthModel
    * Added `MarketDepthModel` class.
    * Added `MarketDepthModelListener` class.
* **\[MDAPI-84]\[C++]** Implement TimeSeriesTxModel
    * Added `TimeSeriesTxModel` class.
* **\[MDAPI-83]\[C++]** Implement IndexedTxModel
    * Added `TxModelListener` class.
    * Added `IndexedTxModel` class.
    * Added `EventSourceWrapper` class needed to pass heterogeneous event sources using a container.
* **\[MDAPI-79]\[C++]** Retrieve promise-based events from feed
    * Added `PromiseList<E>` class where `E` - event type: a list of event receiving results that will be completed
      normally or exceptionally in the future.
      It is a `std::vector<Promise<std::shared_ptr<E>>>` wrapper with Graal semantics.
    * Added `Promise<void>` class.
    * Added `Promises` class: utility methods to manipulate `Promise<>` promises.
        * Added `Promises::allOf(Collection &&collection)` method that returns a new promise that completes when all
          promises from the given collection complete normally or exceptionally.
    * Fixed the `Promise<std::shared_ptr<E>>` semantics.
    * Added `DXFeed::getLastEventPromise` method.
    * Added `DXFeed::getLastEventsPromises` method.
    * Added `DXFeed::getIndexedEventsPromise` method.
    * **\[BREAKING]** The `DXFeed::getTimeSeriesPromise` now returns
      `std::shared_ptr<Promise<std::vector<std::shared_ptr<E>>>>`
    * **\[BREAKING]** The `IndexedEventSubscriptionSymbol::getSource` method now returns
      `std::unique_ptr<IndexedEventSource>`

## v3.0.0

* **\[MDAPI-37]\[C++]** Retrieve the latest events from the feed.
    * Added `SymbolWrapper::toStringUnderlying` method that returns a string representation of the underlying symbol
      object.
    * Added `EventType::assign`. All `EventType` successors can populate their fields using `EventType` successors of
      the same type.
    * Added `DXFeed::getLastEventIfSubscribed` method.
    * Added `DXFeed::getLastEvent` and `DXFeed::getLastEvents` methods that use the `DXFeed::getLastEventIfSubscribed`
      method.
* **\[MDAPI-113]\[C++]\[Tools]** Tools should report invalid event type.
    * Added classes: `RuntimeException`, `InvalidArgumentException`.
    * `InvalidArgumentException`, `GraalException`, `JavaException` are now descendants of the `RuntimeException` class,
      which can collect stacktrace.
    * Now an `InvalidArgumentException` exception is thrown instead of the `std::invalid_argument` exception.
    * `Tools` now reports incorrect event types specified by the user.
* **\[MDAPI-80]\[C++]\[IPF]** Implement custom fields in InstrumentProfile.
    * The API was migrated to Graal SDK v1.1.22.
    * Added methods:
        * `InstrumentProfile::getField`
        * `InstrumentProfile::setField`
        * `InstrumentProfile::getNumericField`
        * `InstrumentProfile::setNumericField`
        * `InstrumentProfile::getDateField`
        * `InstrumentProfile::setDateField`
        * `InstrumentProfile::getNonEmptyCustomFieldNames`
    * **\[BREAKING]**: All `toString` methods can now throw exceptions.
* **\[MDAPI-26]\[C++]** Migrate to Graal SDK v1.1.21.
    * Added `Day::getSessions` method.
    * New order sources added: `CEDX` and `cedx`.
    * Added the ability to use dxFeed Graal SDK stub. In other words, now API can be built for any 32 and 64 platforms.
    * Added `TimePeriod` class.
    * Added `DXFeedSubscription::getAggregationPeriod` and `DXFeedSubscription::setAggregationPeriod` methods.
    * Added `DXFeedSubscription::getEventsBatchLimit` and `DXFeedSubscription::setEventsBatchLimit` methods.
    * Added `AuthToken` class.
    * Added `InstrumentProfileReader::readFromFile(address, token)` method.

## v2.0.0
