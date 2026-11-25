## v9.0.0

* **\[MDAPI-518]\[C++]** **\[BREAKING]** `DXEndpoint`, `DXFeed`, `DXPublisher`, `DXFeedSubscription`,
  `DXFeedTimeSeriesSubscription`, `SubscriptionController` and their listeners call the `dxfg_v4_` functions of the
  Graal Native SDK instead of the deprecated `dxfg_` ones: the library calls no deprecated function of the SDK.
    * **\[BREAKING]** `DXEndpoint::executor()` is removed: it changed the executor of an endpoint that already existed,
      and the last event promises then never completed. Set the executor with `DXEndpoint::Builder::withExecutor()`
      before `build()`.
    * **\[BREAKING]** `JavaException::throwIfNullptr()`, `nullableOrThrow()`, `throwIfLessThanZero()`, `isMinusOne()`,
      `throwIfMinusOne()`, `throwIfMinusMin()` and `throwIfMinusInf()` are removed: they checked the results of the
      deprecated functions of the SDK.
    * `SubscriptionController` and `DXFeedSubscription::getTotalDroppedEvents()` use experimental functions of the
      SDK: they throw `RuntimeException` with both revisions when the SDK library has another advanced revision than
      the one the library is built for.
    * Building from the sources with GCC or Clang: a call of a deprecated function of the SDK is an error
      (`-Werror=deprecated-declarations`) in the code of this project; the projects that use the library do not get
      the option.
* **\[MDAPI-511]\[C++]** **\[BREAKING]** The Graal Native SDK 3.19.0 is the minimum version: the configuration fails
  with an older `DXFEED_GRAAL_NATIVE_SDK_VERSION`, also with `DXFG_LOCAL_ARCHIVE_PATH`.
    * macOS x86-64: the Graal isolate installs its handlers of `SIGPIPE` and `SIGXFSZ`, as on the other platforms
      (SDK 3.14.0+, MDAPI-500, SDK defect MDAPI-501): a write to a closed socket by a connector of an endpoint fails
      with `EPIPE` instead of killing the process.
* **\[MDAPI-493]\[C++]** **\[BREAKING]** Functions that take a string take `dxfcpp::StringParam` instead of
  `const dxfcpp::StringLike &`: a pointer and a length that does not own or copy the characters.
    * `StringParam` accepts a string literal, a `char` array, `const char *` and `char *` (a null pointer is an empty
      string), `std::string`, `std::string_view` and contiguous ranges of `char` (`std::vector<char>`,
      `std::array<char, N>`, `std::span<const char>`). Numbers, `bool`, `std::optional`, wide and UTF-8 strings and
      types that convert to `std::string` only by their own operator do not compile.
    * A `char` array is read up to its first NUL, but not past its end. It used to be read as `const char *`, up to a
      NUL that could lie past the array.
    * `StringLike` is removed. `String::EMPTY`, `String::NUL` and `StringHash` moved to
      `dxfeed_graal_cpp_api/util/StringUtils.hpp`, the `OptionalString` concept to
      `dxfeed_graal_cpp_api/detail/Concepts.hpp`; `dxfeed_graal_cpp_api/api.hpp` includes both.
    * The constructors and setters of events, `IndexedEventSource` and `SessionFilter` that store a string are no
      longer `noexcept`: they copy the string. `TradeETH(std::string)` is `explicit TradeETH(StringParam)`.
    * Overrides of virtual functions that take a string (`EventType::setEventSymbol`, `Order::withEventSymbol`,
      `Order::withMarketMaker` and the like) take `StringParam` and drop `noexcept`.
    * The exported symbols of the library change: rebuild applications.
* **\[MDAPI-462]\[C++]** **\[BREAKING]** The public headers no longer pull in the internals of the library.
    * Windows: the headers can be included before or after `windows.h`. They no longer define `NOMINMAX` and
      `WIN32_LEAN_AND_MEAN` and no longer stop the build with an `#error` when `windows.h` comes first; the macros of
      `windows.h` (`min`, `max`, `NO_ERROR`, `DELETE`, `ERROR`, `TRUE`, `FALSE`) stay defined after them. Code that
      relied on the headers for `NOMINMAX` defines it itself. With `windows.h` included, `OrderAction::DELETE`,
      `Logging::Level::ERROR` and `Tristate::TRUE` / `FALSE` are reachable after `#undef` of the macro (or between
      `#pragma push_macro` and `pop_macro`).
    * The headers of `internal/` and `isolated/` are no longer installed, and the headers of
      [bits](https://github.com/ttldtor/bits) are no longer installed with the library: it is a private
      dependency now. `api.hpp` includes only the public headers. `DXFCPP_DEBUG` and `DXFCPP_TRACE_ISOLATES` no
      longer change the public headers.
    * New public headers in `util/`: `TimeFormat.hpp`, `Timer.hpp`, `Handler.hpp` and `CmdArgsUtils.hpp` (moved from
      `internal/`), `Tristate.hpp`, `Time.hpp` (`now()`, `day_util::getDayIdByYearMonthDay()`) and `StringUtils.hpp`
      (`toString(double)`). Code that included them from `internal/` includes them from `util/`; the namespace of the
      classes is the same.
    * The installed `detail/` headers (`Conf.hpp`, `Enum.hpp`, `StringLike.hpp`, `JavaObjectHandle.hpp` and others)
      are not API: they are installed only because the public headers need them. The types they declare appear in
      public signatures (`StringLike`, the base of the enumerations), but their names and headers may change.
    * The other helpers of `internal/` are internal now and not available to applications, among them
      `enum_utils`, `StopWatch`, `Platform`, `Metrics`, `EntityManager`, `ApiContext`, `Isolate`, `Id`,
      `RawListWrapper`, `NonCopyable` and `ignoreUnused`. Applications that used them keep their own copy.
    * Removed `internal/detail/Formatter.hpp` and the concepts `InstanceOf`, `EnumConcept` and `Extends`: they were not
      used.
* **\[MDAPI-461]\[C++]** **\[BREAKING]** Build and packaging: the shared library exports only its public API and
  has a version, the packages contain a CMake package, the library targets keep their build options to themselves.
    * Linux and macOS: the shared library is built with hidden visibility and exports only the classes and functions
      of the public headers. The symbols of the internal classes and of the embedded `fmt`, `date` and Boost are no
      longer exported; applications that used them link their own copy.
    * The CMake target `dxfcxx` passes `DXFCPP_USE_DLLS` to the targets that link it: an application that uses the
      shared library through CMake no longer defines it. Without CMake, define `DXFCPP_USE_DLLS` as before.
    * The lists of the exported symbols (`abi/symbols-linux.txt`, `abi/symbols-macos.txt`, `abi/symbols-windows.txt`)
      are checked on every change, so a change of the ABI is visible in the pull request.
    * The shared library has a version: `libdxFeedGraalCxxApi.so.9` (SONAME) on Linux, `libdxFeedGraalCxxApi.9.dylib`
      on macOS, with the unversioned name as a link to it. Programs need the versioned file next to them or in the
      library path.
    * The `Lib` archives of a release for Linux and macOS are `.tar.gz` instead of `.zip`: tar keeps the links of the
      versioned library names for every unpacking tool. Windows archives stay `.zip`.
    * The packages contain a CMake package: `find_package(dxFeedGraalCxxApi CONFIG)` gives the targets `dxfcxx::dxfcxx`
      (the shared library), `dxfcxx::static` and `dxfcxx::graal`; the same targets exist when the API is added by
      `add_subdirectory` or `FetchContent`. The targets require C++20 from the targets that link them. A package of
      another major version is not found.
    * The library targets no longer pass their build options to the targets that link them (`/bigobj`, `/utf-8`,
      `/W4`, `-fPIC`, `-Wall`, `-Wextra`, `-pedantic`, `-Wa,--noexecstack`, `DXFCXX_VERSION`,
      `_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR` and the other definitions of the build). An application that relied on them
      sets them itself.
    * Release builds of the libraries are hardened: `/guard:cf`, `/sdl`, `/CETCOMPAT` with MSVC; `_FORTIFY_SOURCE=3`,
      stack protector, full RELRO and immediate binding with GCC and Clang (the `DXFCXX_HARDENING` option).
    * Added as a subproject, the API no longer adds install rules or a CPack configuration to the project and no
      longer overrides its `BOOST_RUNTIME_LINK`; `-DDXFCXX_INSTALL=ON` installs it with the project.
    * The public headers contain only ASCII characters.
    * Within a major version the shared library keeps its ABI on Linux: each release is checked against the previous
      one of the major and publishes its ABI as the asset `abi-x86_64-linux.xml`.
    * The Samples packages have all the samples directly in `bin/`, with one `dxfeed.system.properties`; the samples
      that were in subdirectories of `bin/` no longer need `LD_LIBRARY_PATH` to start on Linux. OnDemandSample sets
      the address of the on-demand service in code.
* **\[MDAPI-460]\[C++]** **\[BREAKING]** Added `ErrorManager` (`ErrorManager.hpp`) for the errors without a caller: an
  exception in a callback of the dxFeed Graal SDK no longer terminates the process. See the "Error handling" page.
    * The callbacks of the SDK (the events of a subscription, the state of an endpoint or of an instrument profile
      connection, a change of a subscription, the instrument profiles of a collector, the events of a model, the log
      messages) catch the exceptions and report them to `ErrorManager`.
    * `ErrorManager` passes each error (`ErrorInfo`: component, source location, message, the class and the stack trace
      of a Java exception) to the handler and keeps the last ones in a journal: `getErrors()`, `getLastError()`,
      `clearErrors()`, `setCapacity()` (128 by default). The default handler writes one line to `stderr`;
      `setHandler()` replaces it, a null handler turns the output off, `resetHandler()` restores it. The handler is
      called on the thread of the error, possibly on several threads at once.
    * An exception of a listener of `IndexedTxModel`, ignored before, now goes to `ErrorManager` (and to `stderr` by
      default).
    * An event of a type that the C++ API does not implement (`CONFIGURATION`, for example) no longer stops the
      delivery: the event is skipped, the other events of the batch are delivered, and one error per type and batch is
      reported with the number of the skipped events. A listener of a subscription is not called for a batch whose
      events are all skipped.
    * The destructors and the functions that free the native data of the SDK no longer throw: a failure of them (for
      example, of the `close()` that the destructors of `DXFeedSubscription` and of the models call) goes to
      `ErrorManager`, except while the library is shutting down.
    * `JavaException` got `getJavaClassName()`, `getJavaMessage()` and `getJavaStackTrace()`.
    * The Java exception of a failed call of the SDK no longer reaches a later, unrelated call: it was left in the
      thread when the failure was reported by a result (`System::setProperty()`, `System::getProperty()`) or ignored
      (`DXFeed::attachSubscription()`, `detachSubscription()`, `detachSubscriptionAndClear()`, the release of a handle).
    * `DXFeed::attachSubscription()`, `detachSubscription()`, `detachSubscriptionAndClear()` and `dxfcpp::gc()` throw
      `JavaException` when the SDK fails, instead of ignoring the failure.
    * A null result without a Java exception from an SDK function that has no null result throws `RuntimeException`
      instead of giving an empty object. The functions that have nothing to return keep giving an empty result: the
      `find*` functions of `Day`, `Session` and `Schedule`, the `*IfSubscribed` functions of `DXFeed`,
      `DXFeedSubscription::getAggregationPeriod()`, `AuthToken::createBasicTokenOrNull()` /
      `createBearerTokenOrNull()` / `getUser()` / `getPassword()`, `Promise::getResult()` / `getException()`.
    * `Promise::getException()` no longer reads an invalid pointer when the SDK fails.
    * **\[BREAKING]** Removed `ErrorHandlingManager` and `Error` (`internal/managers/ErrorHandlingManager.hpp`): they
      were not used. Use `ErrorManager`.
* **\[MDAPI-448]\[C++]** **\[BREAKING]** Added the `CandleDataType` candle attribute (the `dt` key) and
  `CandleSymbol::getDataType()`, as in the Java API.
    * The constants are `SOURCE`, `THO`, `ALL`, `RTH`, `ETH` and `UNDEFINED` (`DEFAULT`). Any other string is a valid
      data type: `CandleDataType::valueOf()` and `parse()` never throw, and the value is case-sensitive. Such a data
      type refers to a copy of its value that the library keeps until the program exits (one per distinct value).
    * `CandleSymbol` normalization removes the empty data type (`A{dt=}` gives `A`) and keeps it when removing it would
      change the base symbol (`A{=t}{dt=}`; Java gives `A{=t}`), as for the other attributes.
    * **\[BREAKING]** `CandleSymbolAttributeVariant` has the new alternative `CandleDataType`: a `std::visit` with an
      overload for each alternative needs one for it.
    * **\[BREAKING]** The layout of `CandleSymbol` changed (ABI).
* **\[MDAPI-441]\[C++]** **\[BREAKING]** The listeners are called on the notifying thread (the executor thread of the
  dxFeed Graal SDK, as in the Java API), not on a new thread for each notification, and no internal lock is held while
  they run.
    * `SimpleHandler` calls the listeners from a snapshot of the list: a listener can remove itself, add a listener
      (called from the next notification) and notify the same handler. If the source notifies on several threads at
      once, the listeners are called on these threads at once.
    * `SimpleHandler::remove()` returns after the call of that listener in progress on another thread ends; called from
      a listener or a callback of the library, it does not wait. `MarketDepthModel::close()` waits the same way.
    * An exception of a listener goes to `ErrorManager`, and the other listeners of the batch are still called. It was
      lost before, together with the rest of the batch.
    * `InstrumentProfileCollector` and `DXPublisherObservableSubscription` call the listeners and the SDK without
      holding their locks: `removeUpdateListener()` from a listener of the collector no longer hangs.
    * Removed the unused `Handler` (`internal/Handler.hpp`). Use `SimpleHandler`.
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
    * `DXFeedSubscription::setAggregationPeriod()` threw `InvalidArgumentException` ("The `period` handle is
      invalid") for `TimePeriod::ZERO` or `UNLIMITED` that had not been used before.
    * The constants of the enums, candle attributes, schedule filters, event types, event flags, order sources,
      `InstrumentProfileField` and `WildcardSymbol::ALL` are constant-initialized: they can be used in the
      initializers of global variables of user code, with the static and the shared library (`CandleType::parse("h")`
      in such an initializer crashed).
    * **\[BREAKING]** The constants can be used in the destructors of global variables of user code: their destruction
      at exit, which can come before such a destructor, no longer changes them (a virtual call on a candle attribute
      constant there could call a pure virtual function). The candle attributes, `CandleType`, `EventTypeEnum`,
      `SessionType`, `DayFilter` and `WildcardSymbol` are trivially destructible: `CandleSymbolAttribute` has a
      protected non-virtual destructor, so an attribute can not be deleted through a pointer to it, and the attributes
      are `final`. `IndexedEventSource`, `OrderSource`, `AuthToken`, `TimePeriod` and `TimeFormat` are not polymorphic:
      their destructors and `IndexedEventSource::toGraal()` and `toGraalUnique()` are not virtual, `OrderSource` no
      longer overrides them, and a copy of an order source to `IndexedEventSource` is converted to Graal as before. The
      Java objects of the constants are released when the Graal isolate is torn down, not when the constants are
      destroyed.
    * **\[BREAKING]** The string keys and literals are `static constexpr char[]` instead of `const std::string`, with
      the same names: `String::NUL` and `EMPTY`, `ATTRIBUTE_KEY` of the candle attributes,
      `WildcardSymbol::RESERVED_PREFIX`, the `*_PROPERTY` keys and `DXSCHEME_ENABLED_PROPERTY_PREFIX` of `DXEndpoint`,
      `AuthToken::BASIC_SCHEME` and `BEARER_SCHEME`. Code that calls member functions of `std::string` on them or adds
      them to each other or to a string literal needs `std::string(...)` or `std::string_view`; comparing a
      `const char *` with them compares pointers.
    * **\[BREAKING]** The maps of constants are constant-initialized `dxfcpp::ConstantMap` instead of
      `std::unordered_map`, with the same names: `BY_STRING` of `CandleType`, `CandlePrice`, `CandleAlignment` and
      `CandleSession`, `EventTypeEnum::ALL_BY_NAME`, `ALL_BY_CLASS_NAME` and `ALL_BY_ID`,
      `OrderSource::PREDEFINED_SOURCES`. A `ConstantMap` has `at()`, `find()`, `contains()`, `size()` and iterators over
      `std::pair<Key, std::reference_wrapper<const T>>` sorted by key; the string keys are `std::string_view`, and the
      keys of `PREDEFINED_SOURCES` are `std::variant<std::int32_t, std::string_view>`. The maps can be used in the
      initializers and the destructors of global variables.
    * **\[BREAKING]** `CandleSymbol::NUL()` returns a new `CandleSymbol` by value. The `EMPTY()` of `CFI`,
      `PriceIncrements` and `AdditionalUnderlyings` are functions that return the object created at the first call, so
      they can be used in the initializers of global variables. These objects and the `OrderSource` user sources
      (`valueOf("CNST")`) are destroyed at exit, so the destructors of global variables should not use them.
    * `TimePeriod::ZERO` and `UNLIMITED`, `AuthToken::NULL_TOKEN` and the `TimeFormat` instances are
      constant-initialized. The Java objects of `TimePeriod`, `TimeFormat`, `CFI`, `PriceIncrements` and
      `AdditionalUnderlyings` constants are created at the first use (`TimePeriod::ZERO.getTime()` and others), and
      their getters no longer lock a mutex on each call. Such calls need the Graal isolate, so the destructors of
      global variables should not make them: the isolate may already be torn down there. The other constants do not
      create the Graal isolate; the new internal `Isolate::isCreated()` tells whether it was created.
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
      member or a Java handle, the `OrderSource` user sources are no longer static members, the names of
      `IndexedEventSource`, `OrderSource` and `SessionFilter` are stored in the new internal `ConstString`,
      `CandlePeriod` no longer stores its string, the classes above have no virtual destructors and `IndexedEventSource`
      has no virtual functions (`OrderSource` has no `toGraal()` and `toGraalUnique()` of its own), `TimePeriod`,
      `TimeFormat`, `CFI`, `PriceIncrements` and `AdditionalUnderlyings` keep their Java object in the new internal
      `LazyJavaObjectHandle` (an atomic state and a function pointer instead of a mutex and a `std::function`), and the
      string keys of `DXEndpoint`, `AuthToken`, `String` and `WildcardSymbol` are no longer exported variables.
* **\[MDAPI-431]\[C++]** **\[BREAKING]** Event fields, defaults and `toString()` as in Java.
    * **\[BREAKING]** Added `getTradeId()` and `setTradeId()` to `Trade`, `TradeETH`, `TimeAndSale` (and so to
      `NuamTrade` and `NuamTimeAndSale`). The trade id is transferred to and from the native SDK (it was lost) and is
      printed by `toString()` as in Java. The layout of these classes has changed, so code that uses them must be
      recompiled.
    * The default day volume and day turnover of `Trade`, `TradeETH` and `NuamTrade` are `NaN`, as in Java (they
      were 0).
    * **\[BREAKING]** The text of `TextMessage` is null by default, as in Java (it was an empty string), and a null text
      received from the native SDK stays null instead of becoming the text `"<null>"`. `getText()` returns `"<null>"`
      for a null text; the new `getTextOpt()` returns `std::nullopt`. `toString()` prints the text in quotes, as in
      Java (`text='null'`). The layout of `TextMessage` has changed, so code that uses it must be recompiled.
    * **\[BREAKING]** The text `"<null>"` in a string field of an event, including the event symbol, is passed to the
      native SDK as text, as in Java (it was passed as null). Only an unset field is passed as null, and a null event
      symbol received from the native SDK stays null. `getX()` still returns `"<null>"` for a null field; `getXOpt()`
      tells null from the text.
    * The string setters of events (except `setEventSymbol()`) and their `with*()` methods also take
      `std::optional<std::string>` and `std::nullopt`, which sets null; `setX(other.getXOpt())` copies a field, null
      included.
    * Exchange codes from 0x80 to 0xFF set as `char` are kept as Latin-1 characters, as in Java (they became 0).
    * `System::setProperty()` and `System::getProperty()` cut a key or a value that is not valid UTF-8 to its longest
      valid prefix (the native SDK replaced the invalid bytes with U+FFFD).
    * `OptionSeries::toString()` prints the expiration and the last trade day as `yyyyMMdd` and the multiplier and
      the SPC as Java does (`1.0E7`); they were printed as day ids and in the C++ notation.
    * `day_util::getYearMonthDayByDayId()` and `day_util::getDayIdByYearMonthDay()` give the results of Java for any
      `int32` value, the limits included (an overflow was undefined behaviour). `day_util::getDayIdByYearMonthDay()` is
      exported from the shared library (code linked to it could not call the function).
    * `Profile::toString()` prints `shares` and `freeFloat` as Java does (`shares=1E7, freeFloat=0.0009`), and a
      negative `index` of `OptionSale` and `Series` is printed as an unsigned hexadecimal number
      (`index=0xffffffffffffffff`), as in Java.
* **\[MDAPI-430]\[C++]** **\[BREAKING]** Candle attributes are parsed and printed as in Java.
    * **\[BREAKING]** `CandlePeriod::parse()` and `CandlePriceLevel::parse()` follow Java `Double.parseDouble`: a sign
      (`+5m`), the `d`/`f` suffix and hexadecimal values (`0x1p3`) of a price level are accepted, and parsing no longer
      depends on the C locale. An invalid number throws `InvalidArgumentException`.
    * `CandlePeriod::toString()` and `CandlePriceLevel::toString()` print values as Java does (`1.0E19m`, `1.0E-5m`,
      `NaNm`, `Infinitym`). Values beyond the range of `std::int64_t` previously printed as wrong numbers.
    * `CandlePeriod::getPeriodIntervalMillis()` saturates as the Java `(long)` cast does (NaN gives 0) instead of
      causing undefined behaviour for large periods such as `1e30m`.
    * Fixed a data race in `CandlePeriod::toString()`: the string is computed on each call instead of being cached.
    * **\[BREAKING]** `CandlePeriod::valueOf()` and the `CandlePeriod` default constructor are no longer `noexcept`.
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
* **\[MDAPI-632]\[C++]** `System::getBuildInfo` tells how the Graal Native SDK library was built and what runs it: the
  version of QDS, the vendor and version of the runtime, the target machine and the garbage collectors
  (`System::BUILD_INFO_QD_VERSION`, `BUILD_INFO_VENDOR`, `BUILD_INFO_MARCH`, `BUILD_INFO_GC`).
    * The Graal Native SDK 3.19.0 is the minimum version: the library calls its `dxfg_v4_system_get_build_info`.
* **\[MDAPI-629]\[C++]** The Graal Native SDK 3.19.0 is the default version.
    * The transactional models, `SubscriptionController`, `DXFeedSubscription::getTotalDroppedEvents`, the events
      `NuamOrder`, `NuamTrade`, `NuamTimeAndSale` and `OrderImbalance` and their enums are no longer experimental: the
      SDK has made their C functions and structures stable.
* **\[MDAPI-600]\[C++]** `Tools NetTest` follows the NetTest specification `nettest/1`: the command line, the error
  messages and the exit codes are the same as in the NetTest of the Graal Native SDK.
    * The producer mode `p` publishes generated `Trade` and `TimeAndSale` events and prints the statistics lines
      (`{*NetTestProducerSide-<name>*}`, one line per instance) and the runtime line (`{*NetTestRuntime-<name>*}`, `n/a`
      for a value the runtime does not give) once per period `-s`, aligned to the system clock.
    * The consumer mode `c` receives the events, `-C` instances sharing the symbols, and prints the end-to-end lag of
      the records (`{*NetTestConsumerSide-<name>*}`) from their time and nanosecond part; a negative lag (a clock skew
      between the hosts) is kept with its sign. The consumer exits with 1 and `no_data` when the open connections bring
      no records for 30 s after the last record (60 s after the connection), and with `nano_missing` when the first
      1000 events have no nanosecond part (`-Ddxscheme.nanoTime=false` receives without it).
    * `--summary <file>` writes the summary of the run in the format `nettest-summary/1`: the intervals of the
      statistics, the end-to-end lag percentiles of the consumer (an HdrHistogram with the layout of the Java one),
      the publishing time and the schedule slip of the producer, the runtime and the statistics of the connectors. The
      file is written through `<file>.tmp` and a rename, also when the run fails. `--warmup <time>` marks the intervals
      from the first batch or record as warm-up: they stay out of the percentiles of the summary.
    * The summary names the QDS version (`tool.qd_version`) and the runtime of the Graal Native SDK library
      (`tool.build`: the vendor, the target machine and the garbage collectors) from `System::getBuildInfo()`.
    * `--metrics-port [<host>:]<port>` serves `/metrics` in the Prometheus text format over HTTP (127.0.0.1 without a
      host, a free port for 0): the records and the connections per instance, the publishing calls and the schedule
      slip of the producer, the end-to-end lag histogram of the consumer, the statistics of the connectors and the
      runtime. The tool prints `{*NetTestMetrics-<name>*} http://<host>:<port>/metrics` and warns when the host is not a
      loopback one, since the endpoint has no authentication and no TLS; a port in use ends the run with 1.
    * `Tools help NetTest` prints the usage with the line `spec_version nettest/1`, as `--help`, `-h` or `-help`
      anywhere in the arguments of `Tools NetTest` (except as the value of an option) does, also after an invalid
      argument.
    * The scheme of `Tools` is `ext:opt:sysprops,opt:dxprops,resource:dxlink.xml`, as in the Tools of the .NET API: the
      scheme properties of the endpoint builders and of the `-p` option, such as `dxscheme.nanoTime`, take effect. The
      dxLink example of the README and the `dxfeed.system.properties` of the samples show the same scheme.
    * The options `-Xmx<size>`, `-Xms<size>`, `-Xmn<size>`, `-Xss<size>` and `-XX:<option>` go to the Graal isolate
      when it is created, in the command line order, and the summary lists them in `config.isolate_arguments`. An
      option that the isolate rejects ends the tool with 2 and the line `NetTest: the isolate rejected one of the
      options: ...`; other `-X` arguments are command line errors.
    * Ctrl+C while the symbols load still writes the summary, with the status `stopped_by_signal`; a second Ctrl+C ends
      the process at once.
    * `--legacy`, the flags of the QD library (`/r`, `{len,max}`, `;<time>`) and the wildcard are not supported and end
      with `unsupported option` or `unsupported QD flag`.
* **\[MDAPI-599]\[C++]** The features of the Graal Native SDK 3.18.0.
    * `MessageConnectorInfo::ioStatistics` has the I/O statistics of a message connector (`IoStatistics`): the data
      and subscription records read and written, the bytes of the QTP messages, the sums of the lags and of the
      round-trip times of the records in microseconds, and the counts of the open and the closed connections. It is
      `std::nullopt` with a library that does not support them.
    * The SDK deserializes only the classes of an allow-list (the dxFeed and QD classes, the JDK collections, boxed
      values and exceptions): in the attachments of `Message`, the object fields of the records and the results of
      `PriceLevelService`. An attachment of another class reads as `std::nullopt`, a result of `PriceLevelService` of
      another class fails the call.
* **\[MDAPI-545]\[C++]** The files of a release can be verified: each archive has an SBOM next to it, and the release
  has the SHA-256 checksums of its files and a build provenance attestation (see "Verifying a download" in the README).
    * The SBOM of an archive is `<name>.cdx.json` (CycloneDX 1.6): the C++ libraries of the archive, the Graal Native
      SDK with the GraalVM and the JDK that built it and the Java libraries of the SDK, with their versions and
      licenses.
    * `SHA256SUMS` has the checksums of the archives and the SBOMs: `sha256sum -c SHA256SUMS --ignore-missing`.
    * `gh attestation verify <file> --repo dxFeed/dxfeed-graal-cxx-api` checks that the file was built by the release
      workflow of this repository.
* **\[MDAPI-531]\[C++]** `System::getRuntimeStatistics()` returns the runtime statistics of the Graal isolate and of its
  process as `RuntimeStatistics`: the heap (used, committed, maximum, and the eden, survivor and old pools), the counts
  and the times of the young and full collections, the threads, the CPU time of the process, the uptime, the allocated
  bytes and the available processors.
    * A value that the platform or the garbage collector does not give is `-1` ("not given"): the pool and GC values
      with a GC other than the Serial GC of the SDK builds, `allocatedBytes` with GraalVM 25.5.
    * With a Graal Native SDK library that does not support the runtime statistics the call throws `RuntimeException`
      that names the feature and the version of the library.
* **\[MDAPI-519]\[C++]** `dxfcpp::shutdown()` stops the listeners with `dxfg_v4_system_shutdown` of the Graal Native
  SDK: after it returns, no listener of the endpoints, the subscriptions, the models and the instrument profile
  connections is called, and none of their calls is in progress, however long the calls take.
    * `dxfcpp::shutdown()` called from a listener or a callback of the library throws `RuntimeException` and stops
      nothing: it would wait for the call it is made from.
* **\[MDAPI-517]\[C++]** Instrument profiles, `InstrumentProfileReader`, `InstrumentProfileCollector`,
  `InstrumentProfileConnection`, promises (`Promise`, `Promises::allOf`) and the transactional models
  (`IndexedTxModel`, `TimeSeriesTxModel`, `IndexedEventTxModel`, `MarketDepthModel`) call the `dxfg_v4_` functions of
  the Graal Native SDK instead of the deprecated `dxfg_` ones.
  * The library checks the version of the Graal Native SDK library when the GraalVM isolate is created: with a library
    older than 3.19.0 the creation fails with the code 13 (`ISOLATE_INITIALIZATION_FAILED`), and `GraalException`
    names the found and the required versions. An older library lacks some of the `dxfg_v4_` functions that the
    library calls, so where the symbols are resolved at load (Windows) it fails to load before the check.
  * The transactional models use the experimental functions of the SDK: `newBuilder()` and the creation of a model
    listener throw `RuntimeException` with both revisions when the SDK library has another advanced revision than the
    one the library is built for.
* **\[MDAPI-516]\[C++]** The common types and the leaf areas call the `dxfg_v4_` functions of the Graal Native SDK
  instead of the deprecated `dxfg_` ones.
    * Strings, lists, `TimeFormat`, `TimePeriod`, event types, `InPlaceExecutor`, the system properties and the parsing
      of symbol lists.
    * `AuthToken`, `CFI`, `PriceIncrements` and `AdditionalUnderlyings`.
    * `Schedule`, `Day`, `Session`, `PriceLevelService`, `OnDemandService`, `HistoryEndpoint` and the logging
      listener.
    * `HistoryEndpoint::newBuilder`, `HistoryEndpoint::Builder::build`, `PriceLevelService::create` and
      `PriceLevelService::getAuthOrderSource` throw `RuntimeException` instead of using an invalid handle when the SDK
      returns no object.
    * `IndexedTxModel::getSources` no longer leaks the array of the sources that the SDK returns.
* **\[MDAPI-486]\[C++]** Samples are fixed:
    * `OptionChainSample` prints the put bid and ask under their own headers (they were swapped).
    * `QuoteAndTradeSample` prints that a quote did not arrive instead of crashing on a null result.
    * `MultipleMarketDepthSample` stops at the end of the input instead of printing the book in a loop.
    * `PriceLevelBookSample` removes all the levels of a cleared source (it skipped some and could increment the end
      iterator).
    * `MarketDepthModelSample` and `PriceLevelBookSample` show the bids under "BID" and the asks under "ASK".
    * `MarketDepthModelSample` hides the cursor while drawing the book.
* **\[MDAPI-480]\[C++]** `DXEndpoint` tells why a connection fails (an experimental API, it may change in future
  versions).
    * `DXEndpoint::getLastConnectionError()` returns the newest connection error of the endpoint as
      `ConnectionError`: the time, the address, the category (`DNS`, `CONNECT_FAILED`, `TLS`, `CLOSED_BY_REMOTE`,
      `HEARTBEAT_TIMEOUT` and others), whether an established connection dropped, the Java exception, its message and
      the number of errors.
    * `DXEndpoint::getConnectors()` returns a snapshot of each message connector as `MessageConnectorInfo`: the state,
      the address, the addresses connected now, the numbers of the open and the closed connections, the statistics and
      the last error.
    * `DXEndpoint::addMessageConnectorListener()` adds a listener called after a connector changes its state, with the
      error of the change already recorded; `DXEndpoint::removeMessageConnectorListener()` removes it.
    * The `Connect` tool prints each new connection error to the standard error stream.
    * The new documentation page "Connection troubleshooting" describes these functions and the usual causes of each
      category.
* **\[MDAPI-476]\[C++]** Documentation: a troubleshooting guide and notes on the data model.
    * `TROUBLESHOOTING.md`: the first steps when there is no data (the `Diagnose` command of the Tools, the versions in
      the log, the QD log in a file), a table "log line or exception, likely cause, first check" for the connection
      errors, the causes of "no data", how to measure the delay of the events, and the reconnect contract of the C++
      API: what is restored after a drop (the connection, the subscriptions) and what is not guaranteed.
    * `DATA_MODEL.md`: conflated events versus `TimeAndSale`, `Trade` versus `TradeETH`, event flags and snapshots,
      `NaN` values, the precision of the time, order sources and scope, why an order book can be crossed, candle
      symbols.
    * Both pages are linked from the README and included in the Doxygen documentation.
    * Recipes: the samples for frequent tasks take an address and `--duration <seconds>`, stop by themselves and tell by
      the exit code whether they got data. `ReconnectSample` connects to a list of addresses and logs the state of the
      connectors with the last connection error, `SimpleOrderBookSample` shows the scope of each order and filters by
      scope, `ScheduleSample` tells whether the market is open at a time, `IpfLiveSample` also reads an IPF file, and
      `CandleSample` stops after the given time. The section "Recipes" of `TROUBLESHOOTING.md` lists the commands.
* **\[MDAPI-475]\[C++]** `CandleSymbol::valueOf` warns about the typical mistakes in a candle symbol, and the
  documentation has a page on how to build one.
    * A candle symbol string without a period (`AAPL`) gives tick candles: `CandleSymbol::valueOf(string)` and the
      `_c` literal report a warning through `ErrorManager` with the symbols to use instead (`AAPL{=1m}`, `AAPL{=d}`,
      or `AAPL{=t}` for tick candles); the default handler prints it to `stderr`. The symbol is created as before.
    * A period of the type `M` (`AAPL{=5M}`) is a minute, a month is `mo`; `PR` is a price, `pr` a Renko price:
      such a period gets a warning too.
    * A warning is reported once per string, for the first 1000 strings in the process. The symbols of the received
      candles, the overloads of `CandleSymbol::valueOf` with attributes and a symbol with the `gr` attribute get none.
    * The page "Candle symbols" of the documentation describes the period, the price, the session, the alignment, the
      exchange, the price level, the data type and the typical mistakes.
* **\[MDAPI-474]\[C++]** A subscription can report that its data stopped while the connection stays up.
    * `DXFeedSubscription::setSilenceWatchdog(timeout, onSilence)` turns on a watchdog: when the subscription receives
      no events for `timeout` while the endpoint of its feed is connected, a warning is reported through
      `ErrorManager` with the subscription (its number and event types) and the time of the silence, then `onSilence`
      is called. Once per silence: the next batch of events or a new connection arms it again.
    * The watchdog is off by default: a symbol that does not trade, a quiet market or a subscription without symbols
      is silent too. A subscription that is attached to no feed or whose endpoint is not connected is not watched.
    * `onSilence` is called on one thread of the library for all the subscriptions, outside of its locks, and may
      call the subscription and the endpoint; the documentation shows a reconnect of the endpoint from it.
    * Fixed: after an `IllegalStateException` of `addEventListener()` (a subscription without event listeners that is
      attached and has symbols), a later call no longer takes the listener for added without delivering events.
* **\[MDAPI-473]\[C++]** `MarketDepthModel` can warn when its book is crossed, and its documentation explains why
  books cross.
    * `MarketDepthModel::Builder::withCrossedBookWarning()` enables a check after each transaction: when the best buy
      order is above the best sell order, a warning is reported through `ErrorManager` with the symbol, the sources
      and both prices. Equal prices are not reported. The check is off by default.
    * A model reports at most one warning per minute (the second parameter changes the period, `0` reports every
      crossing); the next warning tells how many transactions with a crossed book were since the previous one.
    * The section "Crossed book" of the `MarketDepthModel` documentation lists the usual causes (lost packets,
      buffer overflow, conflation, one-sided or combined sources) and states that the model shows what the endpoint
      received.
* **\[MDAPI-472]\[C++]** A subscription warns when its listeners are slow or its events come late, and the
  documentation explains on which threads the listeners are called and why they must not block.
    * When the event listeners of a subscription take longer than 0.1 s for one batch, a warning is reported through
      `ErrorManager` with the subscription (its number and event types), the time and the size of the batch; the
      default handler prints it to `stderr`. The system property `dxfcpp.subscription.slowListenerThreshold`
      (`DXFeedSubscription::SLOW_LISTENER_THRESHOLD_PROPERTY`) changes the threshold, `0` turns the check off.
    * With the system property `dxfcpp.subscription.eventLagThreshold` set (off by default), a warning is reported
      when the newest `Quote`, `Trade`, `TradeETH` or `TimeAndSale` of a received batch is older than the threshold.
      Time series subscriptions are not checked.
    * Each check of a subscription reports at most one warning per minute (`dxfcpp.subscription.warningPeriod`); the
      next warning tells how many excesses were since the previous one and the largest of them. The properties are
      time periods (`0.1s`, `5s`, `1m`) read when a subscription is created.
    * The page "Listeners and threads" of the documentation describes the threads of the listeners and the checks.
* **\[MDAPI-471]\[C++]** The first endpoint of the process reports the versions, and `DXEndpoint::connect` rejects
  the URLs that are not addresses of an endpoint.
    * The first endpoint created in the process reports one `INFO` line through `ErrorManager` with the versions of
      the C++ API and of the Graal Native SDK and the platform; the default handler prints it to `stderr`:
      `[dxfcpp] INFO #1 DXEndpoint: dxFeed Graal C++ API v9.0.0, dxFeed Graal Native SDK 3.19.0, Windows x86_64(16 cores)`
      (`ErrorInfo::toString()` gives no location for an `INFO` message). The version of QD is in the log of the SDK (`Using QDS-<version>, (C) Devexperts`).
    * The platform of macOS is named `MacOS`; it was `iOS|tvOS|watchOS-Simulator` (also in the output of the
      `PerfTest`, `PerfTest2` and `LatencyTest` tools).
    * `ErrorInfo::Level` has a new value `INFO` after `WARN`; only the library reports it. A custom handler and the
      journal get the `INFO` line too: a handler that treats every report as an error should check `level`.
    * **\[BREAKING]** `DXEndpoint::connect` with an address that starts with `http://`, `https://`, `ws://` or `wss://`
      throws `InvalidArgumentException` instead of `JavaException`. The message names the address form
      `host:port[login=...]`, and `dxlink:wss://...` for a WebSocket URL; such addresses never worked. A QD
      configuration URL (`https://host/path/name.config#name`) and the other forms are passed to the SDK as before.
      `dxfc_dxendpoint_connect` returns `DXFC_EC_ERROR` for them, as before.
* **\[MDAPI-470]\[C++]** The timeout exception of a promise from `DXFeed::getLastEventPromise`,
  `getLastEventsPromises`, `getIndexedEventsPromise` and `getTimeSeriesPromise` tells what was requested, how long the
  wait took and the state of the endpoint.
    * `Promise::await(timeout)` still throws the same `JavaException` (`java.util.concurrent.CancellationException`);
      its message gets the method, the event type, the symbol, the source or the time range, the elapsed time, the
      timeout and the endpoint state (`unknown` when the endpoint is gone).
    * `TimeSeriesTxModel::Builder::withFromTime` and `TimeSeriesTxModel::setFromTime` report a warning through
      `ErrorManager` when `fromTime` is now or in the future; the value is still set.
    * `ErrorInfo` has a new last field `level` (`ErrorInfo::Level::ERROR` by default, `WARN` for warnings), and
      `ErrorManager::reportWarning` reports a warning; the default handler prints `WARN` before its number.
      A custom handler and the journal get warnings too: a handler that treats every report as an error should
      check `level`.
* **\[MDAPI-469]\[C++]** Tools: the `Diagnose <address> <types> <symbols>` command checks the way to the data step by
  step and prints a verdict.
    * It prints the versions (API, Graal Native SDK, platform, QD), resolves each host of the address to its IP
      addresses and tries a TCP connect to each, prints the states of the endpoint with their times and the connection
      errors, and for `--duration` seconds (10 by default) collects the events: the first ones of each type, their
      flags, the number per type, the time of the last event per symbol and the receive time minus the event time.
    * The verdict is `DATA`, `PARTIAL DATA`, `CONNECTED, NO DATA` (with the hint "likely not entitled or not in the
      endpoint filter; check the endpoint IPF"), `CANNOT CONNECT` (with the last connection error) or
      `CANNOT RESOLVE`; the exit code is 0 for `DATA`, 2 to 5 for the others and 1 for an error.
    * `--compare <address2>` checks a second address after the first one and compares the two.
    * `--report <file>` writes the output to a file. Logins, passwords, tokens and `user:password@` are hidden in the
      output and in the file.
    * The README names `Diagnose` as the first step when there is no data.
* **\[MDAPI-467]\[C++]** `InstrumentProfileCollector::addUpdateListener()` throws `InvalidArgumentException` when the
  listener ids of the collector are exhausted (about 2^32 listeners added on a 64-bit platform). Before, the
  notifications of such a listener silently went to another listener.
* **\[MDAPI-459]\[C++]** Fixed `toString()` of the events to print as the Java API does. Only the text changes: the
  event data, publishing and receiving are not affected.
    * Numbers are printed as Java `Double.toString()` does: `1.0E-5`, `9.0E-4`, `1.2345678E7`, `2.5E8` (previously
      `1e-05`, `0.0009`, `12345678.0`, `250000000.0`).
    * An unset string field is printed as `null` rather than `<null>`: `ESC` and `optionSymbol` of `OptionSale`,
      `description` and `statusReason` of `Profile`, `marketMaker` of `Order`, `AnalyticOrder` and `OtcMarketsOrder`,
      `spreadSymbol` of `SpreadOrder`.
    * `marketMaker` and `spreadSymbol` are quoted: `marketMaker='NSDQ'`.
    * `Summary` prints `dayLow` without quotes.
* **\[MDAPI-457]\[C++]** Migrated to Graal SDK v3.11.0 and added its features of the Java API.
    * The requested aggregation period of an endpoint and the aggregation period info, as in the Java API.
        * `DXEndpoint::setRequestedAggregationPeriod()` sets the period that the endpoint requests from the server
          (`TimePeriod`, `std::chrono::milliseconds` or milliseconds); `std::nullopt` clears it. Zero requests
          real-time delivery; a period over 24 hours throws `JavaException`.
          `DXEndpoint::getRequestedAggregationPeriod()` returns the period or `std::nullopt` (`null` in Java).
          The period can also be set with the `DXEndpoint::DXFEED_REQUESTED_AGGREGATION_PERIOD_PROPERTY` property
          (`"dxfeed.requestedAggregationPeriod"`).
        * The new `TimePeriodInfo` is the range of aggregation periods: `getMin()`, `getMax()`, `isUnknown()`,
          `TimePeriodInfo::UNKNOWN` and `EMPTY`, `valueOf(min, max)`, and `toString()` as in Java
          (`{"min":1.5,"max":2}`, in seconds).
        * `DXEndpoint::getAggregationPeriodInfo()` returns the periods that the servers report over the connections of
          the endpoint, and `DXFeedSubscription::getAggregationPeriodInfo()` the period applied to the delivered events;
          both are `TimePeriodInfo::UNKNOWN` until known.
    * `DXFeedSubscription::getTotalDroppedEvents()` (experimental, as in the Java API): the number of events dropped for
      the subscription since it was created.
    * `Schedule::getDefinition()`: the definition string of the schedule, as accepted by `Schedule::getInstance()`.
* **\[MDAPI-455]\[C++]** The unreleased items of the release notes are files of `release-notes/`, one per ticket, so
  pull requests do not conflict in `ReleaseNotes.md`: `scripts/release.py` and `scripts/release.cmake` move them under
  the heading of the version on a release or a pre-release. See `release-notes/README.md` and `HOW_TO_RELEASE.md`.
* **\[MDAPI-454]\[C++]** With `DXFCXX_WARNINGS_AS_ERRORS=ON` the use of deprecated functions stays a warning instead of
  failing the build (MSVC: C4996 is disabled).
* **\[MDAPI-452]\[C++]** Tools and samples:
    * `Connect` and `Dump`: the quiet option is `-q, --quiet` as in Java. The former `--quite` is still accepted.
    * `LatencyTest`: `--ignore-exchanges <list>` skips the events of the specified exchange codes (e.g. `Q,Z`):
      `Quote` by the bid or ask exchange, `Trade`, `TradeETH` and `TimeAndSale` by the exchange.
    * Added `CandleSample`: subscribes to `Candle` events of a symbol from a time, with the candle symbol attributes
      given as arguments.
* **\[MDAPI-451]\[C++]** Added the constants and helpers of QDS 3.330–3.351 that the Java API has.
    * `InstrumentProfileType::CERTIFICATE`. It follows `WARRANT` in `InstrumentProfileType::ALL`, as in Java, but has the
      next free code, so the codes of the other types do not change.
    * `Schedule::UTC()` (a 5-day week in UTC with no holidays) and `Schedule::NYSE()`: the schedules of the Java
      `Schedule.UTC` and `Schedule.NYSE`, created at the first call.
    * The property keys `DXEndpoint::DXFEED_STRIPE_PROPERTY` (`dxfeed.stripe`),
      `DXFEED_STICKY_SUBSCRIPTION_PROPERTY` (`dxfeed.stickySubscriptionPeriod`) and
      `DXFEED_REQUESTED_AGGREGATION_PERIOD_PROPERTY` (`dxfeed.requestedAggregationPeriod`).
    * `MarketEventSymbols::hasAttributes()`, `buildSpreadSymbol()`, `getExchangesByPattern()`, `EXCHANGES_PATTERN` and
      `SUPPORTED_EXCHANGES`. `getExchangesByPattern()` takes an ECMAScript character class (`std::regex`), not the Java
      regular expression syntax.
    * `DXFeedSubscription::OPTIMAL_BATCH_LIMIT` and `MAX_BATCH_LIMIT` are `static constexpr`, so code that binds them by
      reference (`std::max`, the comparison macros of test frameworks) links with the shared library. Their type and
      values do not change.
* **\[MDAPI-450]\[C++]** Added the client of the Order Reconstruction Service (ORCS), as in the .NET API:
  `PriceLevelService` (`create()`, `getOrders()`, `getQuotes()`, `getAuthOrderSource()`, `close()`),
  `AuthOrderSource` (the authorized order sources and their symbols) and `PriceLevelChecker::validate()`.
  Added `PriceLevelServiceClientSample`.
* **\[MDAPI-449]\[C++]** Added `IndexedEventTxModel` (the incremental model of indexed events of dxFeed Java API) with
  its builder (feed, symbol, source, listener, events batch limit, aggregation period, executor), and
  `SubscriptionController` (attach, detach, events batch limit, aggregation period, executor) that
  `IndexedEventTxModel::getSubscriptionController()` returns. Added `SimpleOrderBookSample`.
* **\[MDAPI-447]\[C++]** Added the order sources of QDS 3.356: `OrderSource::MOON`, `moon` (MOON ATS),
  `OrderSource::OTCM`, `otcm` (OTC Overnight), `OrderSource::BOSS`, `boss` (Bruce book) and `OrderSource::NEO`, `neo`
  (CBOE NEO book). `OrderSource::valueOf("NEO")` and the others now return the predefined sources.
    * Added `OrderSource::isPublishable(eventType)`, `OrderSource::isFullOrderBook()` and the lists
      `OrderSource::publishable(eventType)` and `OrderSource::fullOrderBook()`, as in the Java API. The lists are
      `std::span<const std::reference_wrapper<const OrderSource>>` created at the first call.
    * `OrderSource::DEFAULT`, `NTV`, `ARCA` and `NUAM` are marked as publishable sources of `OrderImbalance` events, as
      in the Java API (the `OrderImbalance` event itself is not added yet).
* **\[MDAPI-446]\[C++]** Fixed: a process that uses `dxFeedGraalCxxApi.dll` on Windows hung at exit after any call
  that needs the Graal isolate (the fix is the exit model of MDAPI-440).
* **\[MDAPI-442]\[C++]** Closed subscriptions and endpoints are released, as in the Java API: an application that
  creates and closes them no longer grows in memory, and the listeners of a subscription are not called after
  `close()` returns.
    * The code that uses the API stays the same: only the behaviour after `close()` changes.
    * An attached subscription is owned by its feed until it is closed or detached, or the endpoint is closed: a
      subscription without a pointer of the user receives events until then. A subscription without a feed is closed
      with its last pointer.
    * A created endpoint lives until `close()` or `closeAndAwaitTermination()`; then it is released with its feed and
      publisher when the user releases it. The default endpoints stay for the whole process.
    * `DXFeedSubscription::close()` returns after the calls of its listeners in progress on other threads end; called
      from a listener, it does not wait. A subscription or an endpoint can be closed from a listener of a subscription.
    * `CLOSED` is the last state notification of an endpoint. A closed subscription delivers no events; adding a
      listener to it, attaching it to a feed or closing it again does nothing.
    * `DXEndpoint::executor()` called after `getFeed()` applies to the next events of the existing subscriptions, as
      in the Java API.
    * `OnDemandService::getInstance(endpoint)` returns the one service of the endpoint, as in the Java API; the
      service is released with its endpoint. `getEndpoint()` of a service kept longer returns an empty pointer.
    * Transaction models, `InstrumentProfileCollector`, `InstrumentProfileReader` and `HistoryEndpoint` are released
      with their last pointer. A model owns its listener; an `InstrumentProfileConnection` owns its collector.
    * New `AutoClose<T>` closes a subscription, an endpoint or another entity with `close()` at the end of the scope,
      as try-with-resources does in Java. The documentation page "Lifetime of the entities" describes the owners and
      the guarantees of `close()`.
    * The ids of the entities are unique when the library is linked dynamically: a subscription created from a header
      template and one created in the library could get the same id, and the events of the first went to the second.
* **\[MDAPI-440]\[C++]** Fixed: the process hung at exit after a live `InstrumentProfileConnection` was opened (with
  or without `close()`), and could hang or crash at exit with an open endpoint (with the DLL on Windows as well).
    * The library no longer tears down the Graal isolate at exit (the tear-down waits for every Java thread): the
      process ends the Java threads. The library's registries are never destroyed, so the listener calls during the
      exit do not use destroyed objects.
    * The library's exit handler (registered when the library is first used) stops the listener calls and waits up
      to 2 seconds for the calls in progress, so the listeners can use the static objects created before the library
      was first used, with the Windows DLL as well (except a DLL built with MinGW or with the static C runtime: there
      the handler runs after the static objects of the executable are destroyed). If the listeners use the static
      objects created later, close the endpoints and the connections, or call `dxfcpp::shutdown()`, before they are
      destroyed.
    * Added `dxfcpp::shutdown()` (`System.hpp`): closes all the endpoints and the instrument profile connections,
      waits for the termination of the endpoints and the listener calls in progress, and releases the Java objects.
      Calling it is optional. Afterwards the listeners are not called, the functions that need Java fail (most throw
      `GraalException`), and the objects of the library can still be destroyed.
* **\[MDAPI-428]\[C++]** Arguments of the Graal isolate and the error of its creation.
    * Added `System::setIsolateArguments()`: the arguments that the isolate is created with (`-Dmy.prop=value`,
      `-Xmx512m`, `-XX:-EnableSignalHandling`). A repeated call replaces them, an empty vector clears them. It does not
      create the isolate and returns `false` once the creation has started.
    * Added the `DXFCPP_ISOLATE_ARGUMENTS` environment variable with the isolate arguments, split as `JDK_JAVA_OPTIONS`
      is (whitespace, quotes, no escapes). Its arguments go after those of `System::setIsolateArguments()`. The only
      way to give arguments to the isolate in the C API.
    * If the isolate cannot be created (for example, `-Xmx64q`, an unknown `-XX:` option, an unmatched quote in the
      variable), the first call that needs Java and every next one throw `GraalException` with the numeric code, its
      text and the reason; the isolate is not created again. The error used to be lost and showed up later, in
      another call. The values of the arguments are not in the messages.
    * `System::getProperty()` and `System::setProperty()` throw `GraalException` when the isolate cannot be created,
      instead of returning an empty string or `false`.
    * `dxfc_system_set_property()`, `dxfc_system_get_property()` and `dxfc_dxendpoint_new_builder()`,
      `dxfc_dxendpoint_get_instance()`, `dxfc_dxendpoint_get_instance2()`, `dxfc_dxendpoint_create()`,
      `dxfc_dxendpoint_create2()` return `DXFC_EC_ERROR` when the isolate cannot be created: the exception does not
      leave the C function.
* **\[MDAPI-256]\[C++]** Added the events of QDS 3.356 that the Java API has: `OrderImbalance`, `NuamOrder`,
  `NuamTrade`, `NuamTimeAndSale`, `MarketMaker` and `TextConfiguration`.
    * `OrderImbalance` (experimental, as in the Java API, `EventTypeEnum::ORDER_IMBALANCE`): the imbalance between buy
      and sell orders during auctions, with the reference, near and far prices, the paired and imbalance sizes,
      `ImbalanceSide` and `AuctionType`.
        * The event has an order source (`getSource()`): `OrderSource::DEFAULT`, `NTV`, `ARCA` and `NUAM` publish it.
          A subscription by symbol receives the events of all sources. The event is not indexed, as in the Java API.
        * `OrderSource::isPublishable(EventTypeEnum::ORDER_IMBALANCE)` and
          `OrderSource::publishable(EventTypeEnum::ORDER_IMBALANCE)` no longer throw.
        * `toString()` prints the event as the Java API does.
    * `NuamOrder`, `NuamTrade` and `NuamTimeAndSale` (experimental, as in the Java API, `EventTypeEnum::NUAM_ORDER`,
      `NUAM_TRADE`, `NUAM_TIME_AND_SALE`) for the NUAM exchange, with the enums `NuamOrderType` and
      `NuamTimeInForceType`.
        * `NuamOrder` extends `Order` with the actor, participant and submitter ids, the client order id, the customer
          account and info, the exchange info, the time in force and its data, the trigger order book id, price and
          session type, the order type and the order, display, refresh, leaves and matched quantities. The strings
          are `null` until set (`getClientOrderIdOpt()` and the like). `OrderSource::DEFAULT`, `NUAM` and `nuam`
          publish the event: `OrderSource::isPublishable(EventTypeEnum::NUAM_ORDER)` and
          `OrderSource::publishable(EventTypeEnum::NUAM_ORDER)` no longer throw and answer these three sources.
        * `NuamTrade` extends `Trade` with the trade stat time, the last significant price, the last price for all,
          the number of trades and the VWAP; `NuamTimeAndSale` extends `TimeAndSale` with the match id. Code that
          handles `Trade` and `TimeAndSale` handles them too (`is<Trade>()`, `std::dynamic_pointer_cast<Trade>()`), so
          `Trade` and `TimeAndSale` are no longer `final`.
        * `toString()` prints the events as the Java API does, except for what comes from the C++ base classes: there
          is no `tradeId` (the C++ `Trade` and `TimeAndSale` do not have it), and the inherited numbers keep the format
          of `Order`, `Trade` and `TimeAndSale` (`250000000.0` where Java prints `2.5E8`).
    * `MarketMaker` and `TextConfiguration` (`EventTypeEnum::MARKET_MAKER`, `TEXT_CONFIGURATION`).
        * `MarketMaker` is the bid and ask of a market maker: the bid and ask times, prices, sizes and counts. The
          index packs the exchange code and the market maker id, a QD short string that keeps the last 4 characters
          up to U+00FF (`getMarketMakerOpt()` is `std::nullopt` for an unset or empty id); `setMarketMaker()` throws
          `InvalidArgumentException` for more than 8 characters, a character above U+00FF or invalid UTF-8, as the
          Java API does. The default scheme transfers the bid and ask times in seconds.
        * `TextConfiguration` is a lasting event with a text, a version, time and sequence. The text is `null` until
          set (`getTextOpt()`).
        * `toString()` prints the events as the Java API does.
    * `TimeAndSale::toString()` prints unset values as the Java API does: `ESC='null'` (was `ESC='<null>'`), and
      `buyer` and `seller` only when they are set, an empty string included (it printed `buyer='<null>'` for an unset
      buyer and skipped an empty one).
    * `toString()` of the events prints an exchange code from U+8000 to U+FFFF as `\uXXXX`, as the Java API does (it
      printed a negative number).
* **\[MDAPI-32]\[C++]** Strings passed to the dxFeed Graal SDK are checked for valid UTF-8.
    * A stray byte, a truncated or overlong sequence, an encoded surrogate or a value above U+10FFFF throw
      `InvalidArgumentException` naming the function, the parameter, the byte offset and the bytes of the invalid
      sequence, for example `DXEndpoint::connect(address): invalid UTF-8 at byte 12: 0xC3 0x28`. For a user name, a
      password, a token and the values of the `dxfeed.user` and `dxfeed.password` properties the message names the
      offset only.
    * `System::setProperty` and `System::getProperty` throw instead of cutting a key or a value to its longest valid
      prefix; `dxfc_system_set_property` and `dxfc_system_get_property` return `DXFC_EC_ERROR`.
    * The C functions `dxfc_dxendpoint_user`, `dxfc_dxendpoint_password`, `dxfc_dxendpoint_connect`,
      `dxfc_dxendpoint_builder_with_name`, `dxfc_dxendpoint_builder_with_property`,
      `dxfc_dxendpoint_builder_with_properties` and `dxfc_dxendpoint_builder_supports_property` return
      `DXFC_EC_ERROR` instead of letting an exception out.
    * `DXEndpoint::Builder::withProperty` checks the key and the value before it stores them: on invalid UTF-8 the
      builder does not change.
    * A property from `dxfeed.system.properties` or a `DXFEED_*` environment variable that is not valid UTF-8 is not
      set.
    * The string fields and the symbols of events are checked when the events are published
      (`DXPublisher::publishEvents`, writing to a tape); the message names the event type and the field, for example
      `TimeAndSale::exchangeSaleConditions: invalid UTF-8 at byte 1: 0xFF`. The setters of events do not check.
    * Symbols are checked when they go to a subscription (`DXFeedSubscription::addSymbols` and the other functions with
      symbols): `StringSymbol(symbol)`, `CandleSymbol(symbol)`, `IndexedEventSource(name)`.
    * `fillGraalData` of the event types, `SymbolWrapper::toGraal`, `SymbolWrapper::toGraalUnique`,
      `EventSourceWrapper::toGraal`, `EventSourceWrapper::toGraalUnique` and `IndexedEventSource::toGraalUnique` are no
      longer `noexcept`.

## v8.2.0

* **\[MDAPI-511]\[C++]** Migrated to the Graal Native SDK 3.16.0. The Windows `-static-mt` archives have no
  `dxFeedGraalCxxApi.dll` and its import library, and the Full Source Bundle is split into two, one for each C runtime
  of the SDK. See "Full Source Bundles" in `HOW_TO_BUILD.md`.
    * With the static C runtime, the DLL and the code that uses it each have their own C runtime, and the C++ API
      passes standard library objects between them; link `dxFeedGraalCxxApi_static.lib`. The DLL is still built with
      `/MT` (`/MTd`) but not installed: only its C API can be used across this boundary.
    * `dxFeedGraalCxxApi-<version>-x86_64-windows-Full-Source-Bundle.zip` has the SDK with the dynamic C runtime,
      Release and Debug. `dxFeedGraalCxxApi-<version>-x86_64-windows-Full-Source-Bundle-static-mt.zip` has the SDK
      with the static C runtime, Release and Debug, DLL and static library, and builds with
      `DXFCXX_LINK_STATIC_RUNTIME=ON` by default.
    * CMake takes the SDK of a bundle that `DXFCXX_LINK_STATIC_RUNTIME` and `DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK`
      select, as for the downloaded SDK. An option that needs the SDK of the other bundle fails the configuration
      with the name of that bundle (the v8.1.0 bundle built with `DXFCXX_LINK_STATIC_RUNTIME=ON` and the SDK with the
      dynamic C runtime, with a warning).
    * Fixed: TLS connections trusted certificate authorities that are no longer trusted. The SDK is built with
      GraalVM 25.5 and trusts the root CAs of its JDK, so it no longer trusts the eMudhra and Comodo roots that
      JDK 25.0.5 removed and rejects the eMudhra TLS CAs G3B that it blocks (CVE-2026-91403, CVE-2026-114820). A
      connection to a server whose certificate chains up to one of them now fails the TLS handshake, unless the
      application sets `javax.net.ssl.trustStore`. See the
      [Graal Native SDK release notes](https://github.com/dxFeed/dxfeed-graal-native-sdk/releases/tag/v3.15.0).
* **\[MDAPI-521]\[C++]** Fixed the shared library (DLL) on Windows: the `==` and `<` operators of
  `std::reference_wrapper<const EventTypeEnum>` (also used by `std::set` of these references) were not exported, so
  the code that used them did not link.
* **\[MDAPI-512]\[C++]** Fixed the shared library (DLL) on Windows: the listeners of the subscriptions and tx models
  that the code of the public headers creates were not called, and the listener ids of the application and of the
  library collided. Applications that use `dxFeedGraalCxxApi.dll` are rebuilt with the new headers: the layout of
  `Handler`, `SimpleHandler` and `InstrumentProfileCollector` changed.
    * The managers of the entities are kept in the library, one of each type for the library and the application.
    * The listener ids are unique within a handler (`Handler`, `SimpleHandler`), not across handlers.
* **\[MDAPI-445]\[C++]** Fixed: the Linux packages (library, Samples, Tools; x64 and aarch64) did not load on
  Ubuntu 22.04, Debian 12 and Oracle Linux / RHEL 8 and 9: they needed glibc 2.38 and the `libstdc++` of GCC 13.
  * The packages are built on Oracle Linux 8 with GCC Toolset 14 and need glibc 2.28 and the `libstdc++` of GCC 8.
  * The release workflow checks that the packages load on Oracle Linux 8 and Ubuntu 22.04.
  * The library targets link the thread library (`Threads::Threads`, public): with glibc older than 2.34 the
    programs that use the library did not link (`libpthread.so.0: error adding symbols: DSO missing from command line`).
  * With CMake older than 3.28, the installation no longer contains the CMake package of `bits`.
* **\[MDAPI-305]\[C++]** The Windows `-static-mt` archives need no Visual C++ Redistributable, and a single exe or
  dll can be built from them. See "Windows without the Visual C++ Redistributable" in `README.md`.
    * The archives take the SDK DLL built with the static C runtime.
    * The archives add `lib\DxFeedGraalNativeSdk.lib`, `lib\DxFeedGraalNativeSdk_static.lib` and
      `lib\link-flags.txt`: `dxFeedGraalCxxApi_static.lib` links either with the SDK DLL or with the static SDK
      library into one exe or dll that depends only on Windows DLLs.
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
