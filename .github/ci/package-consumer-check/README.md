# Package consumer check

A project that uses the C++ API the way its users do and checks that it builds and runs without options of its own:

| Mode (`DXFCXX_CONSUMER_MODE`) | How the API gets into the project                                             |
|-------------------------------|-------------------------------------------------------------------------------|
| `package`                     | An installed or unpacked package: `find_package(dxFeedGraalCxxApi CONFIG)`    |
| `subdirectory`                | The sources as a directory of the project: `add_subdirectory()`               |
| `fetchcontent`                | The sources by `FetchContent` (`SOURCE_DIR`, no network)                      |

For each library target it gets (`dxfcxx::static`, `dxfcxx::dxfcxx`) it builds and runs two programs:

- `local_hub_<kind>`: publishes a quote and receives it through a `LOCAL_HUB` endpoint (`../static-mt-check/main.cpp`);
- `types_<kind>`: the types of the API across the library boundary (`dynamic_pointer_cast` of events, an exception of
  the API caught by its type).

The project compiles its own code with hidden visibility and sets no C++ standard: the library targets require C++20
themselves. In the `package` mode it also checks that the package is found for its major version and not for the
previous one. With the Ninja and Makefile generators it checks that the compile commands of its own sources have none
of the build options and definitions of the library (`check-options.cmake`). In the `subdirectory` and
`fetchcontent` modes it checks that the API leaves the project's `BOOST_RUNTIME_LINK` alone and adds no install rules
and no CPack configuration (`check-subproject.cmake`).

```shell
cmake --install build --prefix build/install
cmake -S .github/ci/package-consumer-check -B build/consumer -DCMAKE_PREFIX_PATH=$PWD/build/install
cmake --build build/consumer
ctest --test-dir build/consumer --output-on-failure

cmake -S .github/ci/package-consumer-check -B build/consumer-subdir -DDXFCXX_CONSUMER_MODE=subdirectory \
      -DDXFCXX_SOURCE_DIR=$PWD
cmake --build build/consumer-subdir
ctest --test-dir build/consumer-subdir --output-on-failure
```

The `-static-mt` packages (Windows) have no `dxfcxx::dxfcxx`: check them with `-DDXFCXX_CONSUMER_EXPECT_SHARED=OFF`
and `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>`; only the static library is checked then.
Elsewhere a missing `dxfcxx::dxfcxx` fails the check.
