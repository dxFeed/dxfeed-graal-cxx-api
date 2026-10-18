# Optional developer targets; normal consumers do not link sanitizer runtimes.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "PORTALS_BUILD_FUZZERS requires Clang for both C and C++")
endif()

foreach(target IN ITEMS portals_core portals_utf8proc)
  target_compile_options(${target} PRIVATE
    -fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer)
  target_link_options(${target} INTERFACE -fsanitize=address,undefined)
endforeach()

foreach(component IN ITEMS input text)
  add_executable(portals_fuzz_${component} ${component}.cpp)
  target_link_libraries(portals_fuzz_${component} PRIVATE portals::core)
  target_compile_options(portals_fuzz_${component} PRIVATE
    -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
  target_link_options(portals_fuzz_${component} PRIVATE -fsanitize=fuzzer,address,undefined)
  portals_enable_warnings(portals_fuzz_${component})
endforeach()
