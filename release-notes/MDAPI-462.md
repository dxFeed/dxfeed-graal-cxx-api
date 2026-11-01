* **\[MDAPI-462]\[C++]** **\[BREAKING]** The public headers no longer pull in the internals of the library.
    * Windows: the headers can be included before or after `windows.h`. They no longer define `NOMINMAX` and
      `WIN32_LEAN_AND_MEAN` and no longer stop the build with an `#error` when `windows.h` comes first; the macros of
      `windows.h` (`min`, `max`, `NO_ERROR`, `DELETE`, `ERROR`, `TRUE`, `FALSE`) stay defined after them. Code that
      relied on the headers for `NOMINMAX` defines it itself. With `windows.h` included, `OrderAction::DELETE`,
      `Logging::Level::ERROR` and `Tristate::TRUE` / `FALSE` are reachable after `#undef` of the macro (or between
      `#pragma push_macro` and `pop_macro`).
    * The public headers no longer include the headers of `isolated/`, `internal/utils/debug/Debug.hpp`,
      `internal/context/ApiContext.hpp`, `internal/managers/EntityManager.hpp`, `internal/Isolate.hpp` and
      `internal/Id.hpp` where they do not need them; code that used them through the public headers includes them
      itself. `DXFCPP_DEBUG` and `DXFCPP_TRACE_ISOLATES` no longer change the public headers.
    * Removed `internal/detail/Formatter.hpp` and the concepts `InstanceOf`, `EnumConcept` and `Extends`: they were not
      used.
    * `ApiContext::getInstance()` returns a reference instead of `std::shared_ptr`.
