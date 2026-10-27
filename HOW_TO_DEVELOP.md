# How to develop

Notes for the developers of the library: how the code is layered, what it must do at exit, how to write a callback
of the dxFeed Graal SDK and a release path, what to change when a new entity or a new event type is added, and how
to test it. How to build and run the tests:
[HOW_TO_BUILD.md](HOW_TO_BUILD.md); how to release: [HOW_TO_RELEASE.md](HOW_TO_RELEASE.md).

## Layers

| Layer | Where | What it does |
|---|---|---|
| Public API | `include/dxfeed_graal_cpp_api/<module>/X.hpp`, `src/<module>/X.cpp` | The classes of the users, as in the Java API. They call `dxfg_*` functions through the isolated layer; only a few low-level calls are direct (`System`, `DXFeed` attach and detach). |
| Isolated layer | `include/dxfeed_graal_cpp_api/isolated/<module>/IsolatedX.hpp`, `src/isolated/<module>/IsolatedX.cpp` | One C++ function per `dxfg_*` function: checks the handles, calls the SDK in the isolate thread, turns the result into a C++ value or an exception. |
| Handles | `JavaObjectHandle<T>` (`internal/JavaObjectHandle.hpp`) | Owns a Java object of the SDK and releases it (`dxfg_JavaObjectHandler_release`). |
| Entities and managers | `EntityManager<T>`, `ApiContext` (`internal/managers`, `internal/context`) | Find a C++ object by its id in a callback. |
| Graal SDK | `DxFeedGraalNativeSdk` (C API `dxfg_*`) | The Java API compiled by GraalVM Native Image. |

A call goes public class → isolated function → `dxfg_*`. A callback goes the other way: the SDK calls a static
function of the library on its own thread, the function finds the C++ object by id and calls the listeners of the user.

## Calling the SDK

Write a function of the isolated layer for each `dxfg_*` function, next to the others of its module
(`src/isolated/api/IsolatedDXFeedSubscription.cpp` is a good example):

```cpp
void addEventListener(const JavaObjectHandle<DXFeedSubscription> &sub,
                      const JavaObjectHandle<dxfcpp::DXFeedEventListener> &listener) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_addEventListener`. The `sub` handle is invalid");
    }
    // ... the same check for `listener`

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_addEventListener,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_feed_event_listener_t *>(listener.get()));
}
```

- The `runGraalFunctionAndThrow*` helpers of `isolated/IsolatedCommon.hpp` run the function in the isolate thread
  and throw `JavaException` when the result means an error: `...IfNullptr` for functions that return a pointer,
  `...IfLessThanZero` and `...IfMinusOne` for the status codes, `...IfMinusMin`, `...IfMinusInf` for numbers. Pick
  the one that matches what the C API documents for the function.
- A pointer: `...IfNullptr` when the function returns `NULL` only on error (`NULL` without a Java exception then
  throws `RuntimeException`), `runGraalFunctionNullableOrThrow` when `NULL` is also a normal result ("NULL if no
  session is found"). Read the header of the SDK and the Java entry point: a few functions have another error value
  (`dxfg_Promise_getException` returns `(dxfg_exception_t *)-1`).
- A failed function of the SDK leaves its Java exception in the thread until it is read, and a successful call does
  not clear it. So the result of every call is checked right away: through a helper, or, in a direct call that turns
  the failure into a result (`System::setProperty()` returns `false`), with `isolated::clearThreadException()`.
  Otherwise a later, unrelated call that checks its result gets this exception.
- A function that creates a Java object returns `JavaObjectHandle<T>`. Strings and lists that the SDK returns are
  copied into C++ values and released at once with the matching `dxfg_*_release` function.
- After `dxfcpp::shutdown()` or at the exit of the process the isolate does not run any more: the `runIsolated*`
  functions of `internal/Isolate.hpp` return `CEntryPointErrorsEnum::SHUT_DOWN` (or throw it), so a call of the SDK
  fails instead of hanging. See [Lifetime and exit](#lifetime-and-exit).

## Lifetime and exit

The library stops in one of two ways: `dxfcpp::shutdown()`, or its exit handler, which is registered when the library
is first used and runs at the exit of the process. From the start of either, `Isolate::isShuttingDown()` is `true`,
and:

- a call of the SDK fails with `SHUT_DOWN` instead of running (see [Calling the SDK](#calling-the-sdk));
- a callback returns at once (`runCallback`), and the calls in progress are waited for (at exit, up to 2 seconds);
- a failed release is not reported (`runRelease`): it is expected while the SDK is being closed.

What follows for the code of the library:

- **The objects of the library outlive the SDK.** The users destroy their endpoints, subscriptions and models after
  `shutdown()` and at exit, so a destructor must work when the SDK no longer answers: release through `runRelease`,
  never wait for the SDK, never assume that a release succeeds.
- **State that callbacks, destructors or reports use at exit is never destroyed.** `ErrorManager` and the managers of
  `ApiContext` are created on the heap and never deleted (`static auto &x = *new T{...};`), so they work during the
  destruction of the static objects and after it. Follow this pattern for new state of this kind instead of a static
  object with a destructor.
- **No static object whose destructor calls the SDK.** Its order against the exit handler depends on when it was
  created, and with the library as a DLL built with MinGW or with the static C runtime the handler runs after the
  static objects of the executable are destroyed.
- A new behaviour at exit or after `shutdown()` gets a scenario in the exit tests (see [Tests](#tests)): a failure
  there is a hang, a crash or a report at exit, which a unit test does not see.

The rules for the users are in the documentation of `dxfcpp::shutdown()` (`system/System.hpp`).

## Callbacks of the Graal SDK

A callback is a function of the library that the SDK calls: listeners of events, of a state change, of a
subscription change, of instrument profiles, of a log record, of a model. It runs on a thread of the SDK, so an
exception thrown out of it terminates the process, and nobody waits for its result.

Rules:

1. **Wrap the whole body in `runCallback`** (`internal/CallbackBarrier.hpp`). It is `noexcept`. It returns at once
   when the isolate is shutting down (`Isolate::CallbackScope`: the exit of the process and `dxfcpp::shutdown()`
   wait for the callbacks in progress), and it reports any exception of the body to `ErrorManager` with the name of
   the component and the source location. Do not swallow exceptions inside the body: catch only what the callback
   really handles, and let the rest reach `runCallback`.
2. **Pass an id, not a pointer, as the user data.** The C++ object may be destroyed while the SDK still holds the
   listener. Register the object in `EntityManager<T>` and pass `Id<T>`; the callback looks it up and does nothing
   when the object is gone. The managers are never destroyed, so the lookup works at exit as well. A second template
   argument gives the id its own tag type (`EntityManager<TxModelListenerCommon, TxModelListenerTag>`). A pointer is
   allowed only to an object that owns the native listener and releases it first, before the rest of its members:
   releasing a native listener waits for its calls in progress (`IndexedEventTxModelImpl::Listener` in
   `src/model/IndexedEventTxModel.cpp`).
3. **Convert events with `EventMapper::fromGraalListSkippingErrors`.** It skips an event that cannot be converted
   (for example, a type the C++ API does not implement) and reports one error per type with the number of the skipped
   events, so one bad event does not lose the batch. Do not call a listener with an empty batch. The strict
   `EventMapper::fromGraalList` is for the synchronous calls, where the caller gets the exception.
4. **Do not keep pointers to the native data** after the callback returns: copy what you need into C++ values.
5. **Do not hold a lock of the library while calling the user's code**, and do not release a listener under a lock
   that its callback takes: the callback may run on another thread and wait for that lock.
6. **Call the user's listeners through `SimpleHandler`** (`internal/Handler.hpp`). It calls them on the current
   thread, outside its lock, reports an exception of each listener to `ErrorManager` with the component given to its
   constructor, and its `remove()` waits for the call in progress on another thread. A wait of your own for a call in
   progress (as `MarketDepthModel::close()` does) must be skipped when `ListenerCallScope::isActive()`: from a listener
   or a callback of the library it would deadlock.

The canonical example, without its metrics, is `DXFeedSubscription::Impl::onEvents` (`src/api/DXFeedSubscription.cpp`):

```cpp
static void onEvents(graal_isolatethread_t * /*thread*/, dxfg_event_type_list *graalNativeEvents, void *userData) {
    runCallback("DXFeedSubscription", [&] {
        const auto id = Id<DXFeedSubscription>::from(dxfcpp::bit_cast<Id<DXFeedSubscription>::ValueType>(userData));

        const auto manager = ApiContext::getInstance()->getManager<EntityManager<DXFeedSubscription>>();

        if (const auto sub = manager->getEntity(id)) {
            auto &&events = EventMapper::fromGraalListSkippingErrors(static_cast<void *>(graalNativeEvents));

            if (events.empty()) {
                return;
            }

            sub->onEvent_(events);
        }
    });
}
```

The native listener is created with the address of the static function and the id (`setEventListenerHandle()` in the
same file), and its `JavaObjectHandle` is released with the entity. All the callbacks that use `runCallback` are the
examples to follow: `DXEndpoint`, `DXFeedSubscription`, `ObservableSubscriptionChangeListener`,
`InstrumentProfileCollector`, `InstrumentProfileConnection`, `Logging`, `IndexedEventTxModel`, `TxModelListener`.

### Errors without a caller

`ErrorManager` (`exceptions/ErrorManager.hpp`, the Doxygen page "Error handling") receives the errors that have no
caller: the exceptions of the callbacks and of the code the library runs on its own. Code that has a caller throws;
code that has none calls `runCallback`, or `ErrorManager::report()` / `ErrorManager::reportException()` directly.
`ErrorManager` never calls the SDK and is never destroyed, so it is safe at exit and after `dxfcpp::shutdown()`.

### Release paths and `noexcept` functions

Destructors, deleters, the release functions of the handles and every `noexcept` function must not throw: an
exception that leaves them terminates the process. In order of preference:

1. **Call only code that does not throw.** For example, `operator*` after a check instead of `std::optional::value()`,
   `std::get_if` instead of `std::get`, no `fmt` in a `noexcept` `toString()`.
2. **Wrap a release that calls the SDK in `runRelease`** (`internal/CallbackBarrier.hpp`): it is `noexcept` and
   reports an exception of the body to `ErrorManager`, except while the library is shutting down (then the failure is
   expected and is not reported). The destructors of the entities and models do so:

   ```cpp
   ~IndexedTxModel() noexcept override {
       runRelease("IndexedTxModel", [this] {
           close();
       });
   }
   ```

   Do not write an empty `catch (...)`: a failure that nobody sees is a bug that nobody fixes.
3. **`DXFCPP_FINALLY`** (`internal/Common.hpp`) runs its body at the end of the scope; an exception of the body goes
   to `ErrorManager` in the same way.
4. **`NOLINT(bugprone-exception-escape)`** only for a proven false positive, with a comment that says why the call
   cannot throw.

`bugprone-exception-escape` of clang-tidy fails the static analysis job of CI. It sees only the bodies in the same
translation unit: a call to a function defined in another `.cpp`, a virtual call or a `std::function` is taken as
non-throwing, so a clean run does not prove the rules above; follow them by hand.

## Adding an entity

An entity is a C++ class that wraps a Java object of the SDK (`DXFeedSubscription`, `DXPublisher`,
`OnDemandService`, `InstrumentProfileCollector`, models). The commit of `IndexedEventTxModel` and
`SubscriptionController` is a complete example: `git show --stat 35e63475`.

1. **Public header** `include/dxfeed_graal_cpp_api/<module>/X.hpp` and `src/<module>/X.cpp`.
   - Derive from `SharedEntity` (`entity/SharedEntity.hpp`), or from `RequireMakeShared<X>`, which brings
     `SharedEntity`, for a class that is created only through `std::make_shared`: its constructor takes a
     `LockExternalConstructionTag` (see `DXFeedSubscription`).
   - Mark the class `DXFCPP_EXPORT` and follow the rules next to the macro in `internal/Conf.hpp`: every function
     defined in a `.cpp` file is exported, and no mutable state in the headers outside exported classes (a static of
     a template, of an inline function or an inline variable has its own copy in the DLL and in each executable on
     Windows).
   - Code defined in a public header (a template, an inline function, such as the destructors of the models) is
     compiled into the executable of the user, so it may call only exported functions and the members of exported
     classes: the destructors of the models call `runRelease`, which calls `Isolate::isShuttingDown()` and
     `ErrorManager`, both exported. A call of a function that is not exported links with the static library and
     fails only with the DLL on Windows.
   - Keep the Java handle in a `JavaObjectHandle<X>` member.
   - Add the header to the module header, if the module has one (for example `api/ApiModule.hpp`), and to
     `include/dxfeed_graal_cpp_api/api.hpp`.
   - If the entity is created from the private handle of `DXFeed` or `DXEndpoint`, add a `friend` declaration there
     (see `IndexedEventTxModelImpl` in `api/DXFeed.hpp`).
2. **`JavaObjectHandle<X>`**: forward-declare the class, or a `...Tag` struct for a handle without its own public
   class (the builder and the listener of a model: `IndexedEventTxModelBuilderTag`,
   `IndexedEventTxModelListenerTag`), and add `template struct JavaObjectHandle<X>;` in
   `src/internal/JavaObjectHandle.cpp`, next to the others.
3. **Isolated layer**: `isolated/<module>/IsolatedX.hpp` and `src/isolated/<module>/IsolatedX.cpp` with one function
   per `dxfg_*` function the class uses (see [Calling the SDK](#calling-the-sdk)).
4. **Callbacks**: if the SDK calls the entity back, register it in `EntityManager<X>` when it is created
   (`registerEntity`), pass its id as the user data and write the callback as in
   [Callbacks of the Graal SDK](#callbacks-of-the-graal-sdk). `ApiContext::getManager<EntityManager<X>>()` creates
   the manager on first use; nothing else needs to be registered.
5. **CMake**: add the `.cpp` files to the source lists of their groups in the root `CMakeLists.txt`
   (`IsolatedX.cpp` to `dxFeedGraalCxxApi_Isolated_Sources`, `X.cpp` to the list of its module, for example
   `dxFeedGraalCxxApi_Api_Sources`).
6. **Tests**: a test file in `tests/<module>/`, added to the list in `tests/CMakeLists.txt`. Test the static and the
   dynamic library (`-DDXFCXX_DYNAMICALLY_LINK_UNIT_TESTS=ON`): a missing `DXFCPP_EXPORT` is a link error only with
   the DLL.
7. **Docs**: Doxygen comments in the public header, as in the Java API. When the entity is something the users start
   with, a sample: `samples/cpp/<group>/<Name>Sample/` with its `CMakeLists.txt`, `dxfeed.system.properties` and
   `src/main.cpp`, an `add_subdirectory` in the samples block of the root `CMakeLists.txt` and a line in the list of
   the samples in `README.md`.
8. **Release note**: see [Release notes](#release-notes).

## Adding an event type

The event types follow the Java API (`com.dxfeed.event`) field by field. The commit of `OrderImbalance` is a complete
example: `git show --stat 1d92540a` (and `aec2a65e` for `MarketMaker` and `TextConfiguration`).

1. **The event class** `include/dxfeed_graal_cpp_api/event/<module>/X.hpp`, `src/event/<module>/X.cpp`, derived from
   the same base as in Java (`MarketEvent`, `LastingEvent`, `IndexedEvent`, `TimeSeriesEvent`):
   - `static const EventTypeEnum &TYPE;`, defined as `EventTypeEnum::X` in the `.cpp` file;
   - `fromGraal(void *)`, `toGraal()`, `freeGraal(void *)` and the helpers `fillData`, `fillGraalData`,
     `freeGraalData` (copy every field of `dxfg_x_t` of the SDK in both directions, release the strings);
   - `assign()` and the `with*` setters, as in the other events;
   - `toString()` that prints what Java `toString()` prints: the same field names and order, numbers, unset values and
     quoted strings as in Java (use `TimeFormat::DEFAULT_WITH_MILLIS` for the times);
   - new enums of the event as their own classes (`AuctionType`, `ImbalanceSide`).
   Add the header to the module header (for example `event/market/MarketModule.hpp`).
2. **`EventTypeEnum`**: the constant in `event/EventTypeEnum.hpp` and, in `src/event/EventTypeEnum.cpp`, its
   definition (the `dxfg_event_clazz_t` value, the name, the class name and the flags `isLasting`, `isIndexed`,
   `isTimeSeries`, `isMarket` as in Java; `isMarket` is `true` by default) and its entries in all the lists and maps
   of that file (the list of all types, by name, by class name, by `dxfg_event_clazz_t`).
3. **`EventMapper`** (`src/event/EventMapper.cpp`): the include and the `case DXFG_EVENT_X` in `fromGraal` and in
   `freeGraal`. Without them the event is skipped in the callbacks and reported to `ErrorManager`.
4. **`isolated/IsolatedCommon.hpp`**: the name of `DXFG_EVENT_X` in `toString(dxfg_event_clazz_t)`.
5. **`OrderSource`**, if the event is published per order source as in Java: its flag in the masks of the sources and
   in `OrderSource::getEventTypeMask` (`src/event/market/OrderSource.cpp`), otherwise `isPublishable()` throws for the
   new type; `tests/api/OrderSourceTest.cpp`.
6. **CMake**: the new `.cpp` files in the source list of their group in the root `CMakeLists.txt` (for example
   `dxFeedGraalCxxApi_EventMarket_Sources`).
7. **Tests**:
   - a test of the class in `tests/api/` (fields, flags, `toString()`), with the expected Java values in
     `tests/api/EventsJavaParity.hpp` recorded from the Java API, not written by hand;
   - the row of the type in the table of `tests/event/EventTypeTest.cpp` (the same flags as in `EventTypeEnum`);
     `tests/event/ConstantsTest.cpp` when the event adds enums;
   - the round trip through QD: the event in `tests/support/EventFactory.cpp` (and the counts of the types in the
     comments of `EventFactory.hpp`), the records of the event appended to the golden tapes of `tests/data/tapes`
     as the Java API writes them, the edge-value tapes written again (both as its README says, with its table
     updated), `tests/support/TapeSupport.cpp` when the event is published per source, `KNOWN_NULL_STRING_DEFECTS`
     of `tests/event/EdgeValuesTapeTest.cpp` when a string of the event does not survive the tape as null;
   - add new test files to `tests/CMakeLists.txt`.
8. **Docs**: the event in the list of the supported events in `README.md`, Doxygen comments as in the Java API.
9. **Release note**: see [Release notes](#release-notes).

The SDK may send an event class that this version does not know (a newer SDK): `EventMapper` reads the class as an
integer and skips such events with a report, so do not cast an unknown value to `dxfg_event_clazz_t` elsewhere.

## Tests

The unit tests use doctest. Each file in `tests/<module>/` is listed in `DXFC_TEST_SOURCES` of `tests/CMakeLists.txt`
and builds its own executable, `dxFeedGraalCxxApi_<module>_<Name>`. A fix starts with a test that fails without it.

- **Support** (`tests/support/TestSupport.hpp`):
  - `LocalHubFixture`: a `LOCAL_HUB` endpoint with an in-place executor; `publishAndProcess()` delivers the published
    events to the listeners of the feed before it returns. No network, no waiting.
  - `waitUntil()` for the work of other threads.
  - `LoopbackTcpServer` and `findFreeLoopbackPort()` for a connection without the internet.
  - `ErrorManagerGuard`: turns the output of `ErrorManager` off and clears its journal for the test, then restores
    the defaults. Use it in every test that expects a reported error, and check the error by `getLastError()` or
    `getErrors()`.
  - `EventFactory` and `TapeSupport` for the round trip of the events through QD (see
    [Adding an event type](#adding-an-event-type)).
- **Process tests** check what a unit test cannot: a hang, a crash or a report at the exit of the process.
  - `tests/exit/ExitTest.cpp`: one scenario per behaviour at exit or after `dxfcpp::shutdown()`. A scenario is a
    function that returns the exit code (0 when it passes), a branch in `main()` and a name in `DXFC_EXIT_SCENARIOS`.
    Each scenario runs with the static and the shared library; a hang fails it by its timeout.
  - `tests/static-init/`: the constants of the library read from the static initializers of the test, which run
    before those of a statically linked library (with and without the Graal SDK, and with the DLL).
- **Network tests** (`tests/network/`) run only with `-DDXFCXX_ENABLE_NETWORK_TESTS=ON` and have the label `network`.
  The other tests do not use the network.
- **Sanitizers**: build options and the canary tests are in [HOW_TO_BUILD.md](HOW_TO_BUILD.md#sanitizers). Add a
  suppression to `tests/sanitizers/*.supp` only for a report inside the Graal SDK, with a comment that says why.
- **Do not depend on one compiler.** For example, `std::source_location::current()` as a default argument gives the
  first line of a call that spans several lines with GCC and MSVC and the last one with Clang: compare such a line with
  a range, or write the call on one line.

## Release notes

A user-visible change adds a file to `release-notes/` instead of editing `ReleaseNotes.md`: `release-notes/MDAPI-NNN.md`
for a ticket, one item of the top level with nested items for the details. The rules are in
[release-notes/README.md](release-notes/README.md); the release scripts check them.

## Code style

- `clang-format` with `.clang-format` (LLVM based, 4 spaces, 120 columns).
- `clang-tidy` with `.clang-tidy` finds bugs in CI; it does not check the style.
- Each file starts with the copyright and `SPDX-License-Identifier: MPL-2.0` lines; headers use `#pragma once`; code
  is inside `DXFCPP_BEGIN_NAMESPACE` / `DXFCPP_END_NAMESPACE`.
- Names: classes in `PascalCase`, functions and variables in `camelCase`, data members with a trailing underscore
  (`handle_`), constants in `UPPER_CASE`.
- Warnings are errors in CI (`-DDXFCXX_WARNINGS_AS_ERRORS=ON`): build with it locally before a PR.
- CI builds with MSVC, clang-cl, AppleClang, Clang and GCC (`.github/ci/build-matrix.json`), and each compiler has
  warnings of its own: for example, GCC reports a constructor parameter that shadows a member (`-Wshadow`), and MSVC
  and Clang may stay silent on the same code.
  Before a PR, build and run the tests with at least one compiler besides your usual one.
