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
