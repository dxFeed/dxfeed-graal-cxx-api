* **\[MDAPI-435]\[C++]** Portals (used by `MarketDepthModelSample`) is updated to v0.2.0 from its new repository
  (the old one was renamed, and the `v0.1.1` fallback no longer resolves): `third_party/portals-0.2.0`, or the pinned
  v0.2.0 commit when it is not vendored. The sample uses the `portals::legacy` API and is built with C++23. Portals is
  configured only for the samples: when CMake is older than 3.24 or the standard library lacks the C++23 and C++20
  parts it uses (`std::expected`, `std::format`, `std::move_only_function`, `std::stop_token`; Apple libc++ of
  Xcode 16 lacks the last two), `MarketDepthModelSample` is skipped with a status message. The library still needs
  CMake 3.21 and C++20. [#125](https://github.com/dxFeed/dxfeed-graal-cxx-api/issues/125)
