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
      `internal/`), `Tristate.hpp`, `Time.hpp` (`now()`) and `StringUtils.hpp` (`toString(double)`). Code that
      included them from `internal/` includes them from `util/`; the namespace of the classes is the same.
    * The installed `detail/` headers (`Conf.hpp`, `Enum.hpp`, `StringLike.hpp`, `JavaObjectHandle.hpp` and others)
      are not API: they are installed only because the public headers need them. The types they declare appear in
      public signatures (`StringLike`, the base of the enumerations), but their names and headers may change.
    * The other helpers of `internal/` are internal now and not available to applications, among them
      `enum_utils`, `StopWatch`, `Platform`, `Metrics`, `EntityManager`, `ApiContext`, `Isolate`, `Id`,
      `RawListWrapper`, `NonCopyable` and `ignoreUnused`. Applications that used them keep their own copy.
    * Removed `internal/detail/Formatter.hpp` and the concepts `InstanceOf`, `EnumConcept` and `Extends`: they were not
      used.
