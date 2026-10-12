cmake_minimum_required(VERSION 3.24)
project(PortalsPackageConsumer LANGUAGES CXX)

find_package(Portals 0.2 CONFIG REQUIRED)
cmake_path(IS_PREFIX CMAKE_PREFIX_PATH "${Portals_DIR}" NORMALIZE fromInstalledPrefix)
if(NOT fromInstalledPrefix)
  message(FATAL_ERROR "Portals was found outside the relocated prefix: ${Portals_DIR}")
endif()

# Both archives must be imported from the installation, with no private build settings.
foreach(target IN ITEMS portals::core portals::utf8proc)
  get_target_property(imported ${target} IMPORTED)
  if(NOT imported)
    message(FATAL_ERROR "${target} is not an installed imported target")
  endif()
  foreach(property IN ITEMS INTERFACE_COMPILE_OPTIONS INTERFACE_COMPILE_DEFINITIONS)
    get_target_property(value ${target} ${property})
    if(value)
      message(FATAL_ERROR "Private build settings leaked through ${target}: ${property}=${value}")
    endif()
  endforeach()
endforeach()

add_executable(portals_package_consumer main.cpp)
target_link_libraries(portals_package_consumer PRIVATE portals::core)
target_compile_definitions(portals_package_consumer PRIVATE
  EXPECTED_PORTALS_VERSION_MAJOR=${Portals_VERSION_MAJOR}
  EXPECTED_PORTALS_VERSION_MINOR=${Portals_VERSION_MINOR}
  EXPECTED_PORTALS_VERSION_PATCH=${Portals_VERSION_PATCH})
set_target_properties(portals_package_consumer PROPERTIES CXX_EXTENSIONS OFF)
if(WIN32)
  # The install test stages the existing process startup source beside this fixture.
  target_sources(portals_package_consumer PRIVATE no_dialogs.cpp)
endif()

enable_testing()
add_executable(portals_compact_consumer compact.cpp)
target_link_libraries(portals_compact_consumer PRIVATE portals::aggregate)
set_target_properties(portals_compact_consumer PROPERTIES CXX_EXTENSIONS OFF)
add_test(NAME consumer.compact-columns COMMAND portals_compact_consumer)
foreach(example IN ITEMS gallery dashboard custom_widget)
  add_executable(portals_installed_${example} examples/${example}/main.cpp)
  target_link_libraries(portals_installed_${example} PRIVATE portals::core)
  set_target_properties(portals_installed_${example} PROPERTIES CXX_EXTENSIONS OFF)
  if(WIN32)
    target_sources(portals_installed_${example} PRIVATE no_dialogs.cpp)
  endif()
endforeach()
add_test(NAME consumer.example-gallery COMMAND portals_installed_gallery --check)
add_test(NAME consumer.example-custom COMMAND portals_installed_custom_widget --check)
add_test(NAME consumer.example-dashboard COMMAND portals_installed_dashboard --check --mode replay --duration-ms 200 --rows 1000)
set_tests_properties(consumer.compact-columns consumer.example-gallery consumer.example-custom consumer.example-dashboard
  PROPERTIES TIMEOUT 20)
add_executable(portals_legacy_consumer legacy.cpp)
target_link_libraries(portals_legacy_consumer PRIVATE portals::core)
set_target_properties(portals_legacy_consumer PROPERTIES CXX_EXTENSIONS OFF)
if(WIN32)
  target_sources(portals_legacy_consumer PRIVATE no_dialogs.cpp)
endif()
add_test(NAME consumer.legacy-market-depth COMMAND portals_legacy_consumer)
set_tests_properties(consumer.legacy-market-depth PROPERTIES TIMEOUT 15)

add_test(NAME consumer.model-widget COMMAND portals_package_consumer)
set_tests_properties(consumer.model-widget PROPERTIES TIMEOUT 15)

# Complete before/after forms are consumers of the relocated public package only.
foreach(variant IN ITEMS before after declarative)
  add_executable(portals_form_${variant} form_${variant}.cpp)
  target_link_libraries(portals_form_${variant} PRIVATE portals::core)
  set_target_properties(portals_form_${variant} PROPERTIES CXX_EXTENSIONS OFF)
  if(WIN32)
    target_sources(portals_form_${variant} PRIVATE no_dialogs.cpp)
  endif()
  add_test(NAME consumer.form-${variant} COMMAND portals_form_${variant})
  set_tests_properties(consumer.form-${variant} PROPERTIES TIMEOUT 15)
endforeach()

add_test(NAME consumer.form-equivalence
  COMMAND "${CMAKE_COMMAND}"
    "-DBEFORE=$<TARGET_FILE:portals_form_before>"
    "-DAFTER=$<TARGET_FILE:portals_form_after>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compare_forms.cmake")
add_test(NAME consumer.form-typed-error COMMAND portals_form_after --failure)
set_tests_properties(consumer.form-equivalence consumer.form-typed-error PROPERTIES TIMEOUT 30)

add_test(NAME consumer.form-declarative-equivalence
  COMMAND "${CMAKE_COMMAND}"
    "-DBEFORE=$<TARGET_FILE:portals_form_after>"
    "-DAFTER=$<TARGET_FILE:portals_form_declarative>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compare_forms.cmake")
add_test(NAME consumer.form-declarative-typed-error COMMAND portals_form_declarative --failure)
set_tests_properties(consumer.form-declarative-equivalence consumer.form-declarative-typed-error PROPERTIES TIMEOUT 30)

# Nested virtual views use only the public headers from the relocated package.
foreach(variant IN ITEMS before after lifecycle managed managed_lifecycle declarative_lifecycle)
  add_executable(portals_virtual_${variant} virtual_${variant}.cpp)
  target_link_libraries(portals_virtual_${variant} PRIVATE portals::core)
  set_target_properties(portals_virtual_${variant} PROPERTIES CXX_EXTENSIONS OFF)
  if(WIN32)
    target_sources(portals_virtual_${variant} PRIVATE no_dialogs.cpp)
  endif()
  add_test(NAME consumer.virtual-${variant} COMMAND portals_virtual_${variant})
  set_tests_properties(consumer.virtual-${variant} PROPERTIES TIMEOUT 15)
endforeach()

add_test(NAME consumer.virtual-equivalence
  COMMAND "${CMAKE_COMMAND}"
    "-DBEFORE=$<TARGET_FILE:portals_virtual_before>"
    "-DAFTER=$<TARGET_FILE:portals_virtual_after>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compare_forms.cmake")
set_tests_properties(consumer.virtual-equivalence consumer.virtual-lifecycle PROPERTIES TIMEOUT 30)

add_test(NAME consumer.virtual-managed-equivalence
  COMMAND "${CMAKE_COMMAND}"
    "-DBEFORE=$<TARGET_FILE:portals_virtual_after>"
    "-DAFTER=$<TARGET_FILE:portals_virtual_managed>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compare_forms.cmake")
set_tests_properties(consumer.virtual-managed-equivalence PROPERTIES TIMEOUT 30)

# Identical form behavior, including virtual views and a custom final widget.
foreach(variant IN ITEMS before after named compact)
  add_executable(portals_composed_${variant} composed_${variant}.cpp)
  target_link_libraries(portals_composed_${variant} PRIVATE portals::core)
  set_target_properties(portals_composed_${variant} PROPERTIES CXX_EXTENSIONS OFF)
  if(WIN32)
    target_sources(portals_composed_${variant} PRIVATE no_dialogs.cpp)
  endif()
  add_test(NAME consumer.composed-${variant} COMMAND portals_composed_${variant})
  set_tests_properties(consumer.composed-${variant} PROPERTIES TIMEOUT 15)
endforeach()

add_test(NAME consumer.composed-compact-equivalence
  COMMAND "${CMAKE_COMMAND}"
    "-DBEFORE=$<TARGET_FILE:portals_composed_before>"
    "-DAFTER=$<TARGET_FILE:portals_composed_compact>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compare_forms.cmake")
set_tests_properties(consumer.composed-compact-equivalence PROPERTIES TIMEOUT 30)

add_test(NAME consumer.composed-equivalence
  COMMAND "${CMAKE_COMMAND}"
    "-DBEFORE=$<TARGET_FILE:portals_composed_before>"
    "-DAFTER=$<TARGET_FILE:portals_composed_after>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compare_forms.cmake")
set_tests_properties(consumer.composed-equivalence PROPERTIES TIMEOUT 30)

add_test(NAME consumer.composed-named-equivalence
  COMMAND "${CMAKE_COMMAND}"
    "-DBEFORE=$<TARGET_FILE:portals_composed_before>"
    "-DAFTER=$<TARGET_FILE:portals_composed_named>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compare_forms.cmake")
set_tests_properties(consumer.composed-named-equivalence PROPERTIES TIMEOUT 30)

# Invalid expressions must fail at named constraints, not inside type-erased callbacks.
foreach(kind IN ITEMS projection formatter)
  add_library(portals_invalid_${kind} OBJECT EXCLUDE_FROM_ALL invalid_column.cpp)
  target_link_libraries(portals_invalid_${kind} PRIVATE portals::core)
  if(kind STREQUAL "projection")
    target_compile_definitions(portals_invalid_${kind} PRIVATE PORTALS_INVALID_PROJECTION)
  endif()
  add_test(NAME consumer.invalid-${kind}
    COMMAND "${CMAKE_COMMAND}"
      "-DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}" "-DCONFIG=$<CONFIG>"
      "-DTARGET=portals_invalid_${kind}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/check_column_diagnostic.cmake")
  set_tests_properties(consumer.invalid-${kind} PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
endforeach()
