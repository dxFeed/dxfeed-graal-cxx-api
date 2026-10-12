cmake_minimum_required(VERSION 3.24)
project(Portals VERSION 0.2.0 LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Use an out-of-tree build directory, for example cmake -S . -B build/debug")
endif()

option(PORTALS_BUILD_TESTS "Build Portals headless tests" ${PROJECT_IS_TOP_LEVEL})
option(PORTALS_BUILD_EXAMPLES "Build Portals example consumer" ${PROJECT_IS_TOP_LEVEL})
option(PORTALS_BUILD_FUZZERS "Build opt-in Clang libFuzzer targets with sanitizers" OFF)
option(PORTALS_BUILD_DOCS "Build Portals API documentation with Doxygen" OFF)
option(PORTALS_BUILD_TOOLS "Build the standalone terminal diagnostic probe" ${PROJECT_IS_TOP_LEVEL})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include("${CMAKE_CURRENT_SOURCE_DIR}/vendor/utf8proc/verify.cmake")
add_library(portals_utf8proc STATIC vendor/utf8proc/utf8proc.c)
set_target_properties(portals_utf8proc PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
target_compile_definitions(portals_utf8proc PRIVATE UTF8PROC_STATIC)
target_include_directories(portals_utf8proc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/utf8proc")

add_library(portals_core STATIC src/version.cpp src/geometry.cpp src/text.cpp src/input.cpp src/cell_buffer.cpp src/render.cpp src/render/diff.cpp src/terminal/writer.cpp src/runtime/scheduler.cpp src/runtime/application.cpp src/runtime/native_application.cpp src/runtime/diagnostics.cpp src/widgets/widget.cpp src/layout/linear.cpp src/layout/grid.cpp src/layout/overlay.cpp src/widgets/focus.cpp src/widgets/event_routing.cpp src/models/model.cpp src/widgets/basic_controls.cpp src/widgets/line_edit.cpp src/widgets/managed.cpp src/views/virtual_list.cpp src/views/virtual_table.cpp src/views/scroll.cpp src/styles/style.cpp src/styles/pss.cpp src/charts/canvas.cpp src/charts/chart.cpp)
target_sources(portals_core PRIVATE src/terminal/legacy.cpp)
if(WIN32)
  target_sources(portals_core PRIVATE src/terminal/windows.cpp)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_sources(portals_core PRIVATE src/terminal/posix.cpp src/terminal/posix_io.cpp)
  target_link_libraries(portals_core PRIVATE Threads::Threads)
else()
  target_sources(portals_core PRIVATE src/terminal/unsupported.cpp)
endif()
target_link_libraries(portals_core PRIVATE portals_utf8proc)
target_include_directories(portals_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/utf8proc")
target_compile_definitions(portals_core PRIVATE UTF8PROC_STATIC)
add_library(portals::core ALIAS portals_core)
target_compile_features(portals_core PUBLIC cxx_std_23)
set_target_properties(portals_core PROPERTIES CXX_EXTENSIONS OFF EXPORT_NAME core)
set_target_properties(portals_utf8proc PROPERTIES EXPORT_NAME utf8proc)
target_include_directories(portals_core PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# Aggregate reflection is opt-in; ordinary core headers do not include PFR.
include("${CMAKE_CURRENT_SOURCE_DIR}/vendor/pfr/verify.cmake")
add_library(portals_aggregate INTERFACE)
add_library(portals::aggregate ALIAS portals_aggregate)
set_target_properties(portals_aggregate PROPERTIES EXPORT_NAME aggregate)
target_link_libraries(portals_aggregate INTERFACE portals::core)
target_include_directories(portals_aggregate SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/pfr/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Portals/pfr>"
)
target_compile_definitions(portals_aggregate INTERFACE BOOST_PFR_USE_CPP26=0)

set(PORTALS_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Portals")
set(PORTALS_PACKAGE_NEEDS_THREADS OFF)
if(UNIX)
  set(PORTALS_PACKAGE_NEEDS_THREADS ON)
endif()
configure_package_config_file(cmake/PortalsConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/PortalsConfig.cmake"
  INSTALL_DESTINATION "${PORTALS_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/PortalsConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion
)
install(TARGETS portals_core portals_utf8proc portals_aggregate EXPORT PortalsTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/$<CONFIG>"
)
install(DIRECTORY include/portals DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES include/portals.hpp DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY vendor/pfr/include/boost DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Portals/pfr")
install(FILES vendor/pfr/LICENSE_1_0.txt DESTINATION "${CMAKE_INSTALL_DATADIR}/Portals/licenses" RENAME pfr-LICENSE.txt)
install(FILES vendor/pfr/manifest.json DESTINATION "${CMAKE_INSTALL_DATADIR}/Portals/pfr")
install(EXPORT PortalsTargets FILE PortalsTargets.cmake NAMESPACE portals::
  DESTINATION "${PORTALS_INSTALL_CMAKEDIR}"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/PortalsConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/PortalsConfigVersion.cmake"
  DESTINATION "${PORTALS_INSTALL_CMAKEDIR}"
)
install(FILES LICENSE.txt DESTINATION "${CMAKE_INSTALL_DATADIR}/Portals/licenses")
install(FILES vendor/utf8proc/LICENSE.md
  DESTINATION "${CMAKE_INSTALL_DATADIR}/Portals/licenses" RENAME utf8proc-LICENSE.md
)
install(FILES vendor/utf8proc/manifest.json
  DESTINATION "${CMAKE_INSTALL_DATADIR}/Portals/utf8proc"
)
install(FILES docs/package.md DESTINATION "${CMAKE_INSTALL_DATADIR}/Portals/docs")
install(FILES vendor/textual/LICENSE
  DESTINATION "${CMAKE_INSTALL_DATADIR}/Portals/licenses" RENAME textual-LICENSE.txt)
install(FILES vendor/textual/README.md DESTINATION "${CMAKE_INSTALL_DATADIR}/Portals/textual")
target_compile_definitions(portals_core PRIVATE
  PORTALS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  PORTALS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  PORTALS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

function(portals_enable_warnings target)
  if(MSVC)
    # Sources and narrow string literals use UTF-8 independently of the host code page.
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

portals_enable_warnings(portals_core)

if(PORTALS_BUILD_TOOLS)
  add_executable(portals_compact_benchmark tools/benchmarks/compact.cpp)
  target_link_libraries(portals_compact_benchmark PRIVATE portals::core)
  portals_enable_warnings(portals_compact_benchmark)
  add_executable(portals_managed_benchmark tools/benchmarks/managed.cpp)
  target_link_libraries(portals_managed_benchmark PRIVATE portals::core)
  portals_enable_warnings(portals_managed_benchmark)
  target_compile_definitions(portals_managed_benchmark PRIVATE PORTALS_BENCHMARK_BUILD_TYPE="$<CONFIG>")
  add_executable(portals_mvp_benchmark tools/benchmarks/mvp.cpp)
  target_link_libraries(portals_mvp_benchmark PRIVATE portals::core)
  portals_enable_warnings(portals_mvp_benchmark)
  add_executable(portals_terminal_probe tools/terminal_probe/main.cpp tools/terminal_probe/query.cpp)
  if(WIN32)
    target_sources(portals_terminal_probe PRIVATE tools/terminal_probe/native_windows.cpp)
  elseif(UNIX)
    target_sources(portals_terminal_probe PRIVATE tools/terminal_probe/native_posix.cpp)
  else()
    message(FATAL_ERROR "The terminal probe requires Windows or a POSIX platform; disable PORTALS_BUILD_TOOLS")
  endif()
  target_compile_features(portals_terminal_probe PRIVATE cxx_std_23)
  set_target_properties(portals_terminal_probe PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    # The single-threaded diagnostic only reads getenv results; keep this CRT
    # portability choice private to the tool rather than changing core warnings.
    target_compile_definitions(portals_terminal_probe PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  portals_enable_warnings(portals_terminal_probe)
endif()

if(PORTALS_BUILD_EXAMPLES)
  add_subdirectory(examples/consumer)
  add_subdirectory(examples/gallery)
  add_subdirectory(examples/dashboard)
  add_subdirectory(examples/custom_widget)
  add_subdirectory(examples/replay)
  portals_enable_warnings(portals_consumer)
endif()

if(PORTALS_BUILD_TESTS)
  add_test(NAME portals.release-package
    COMMAND "${CMAKE_COMMAND}"
      "-DTEST_ROOT_DIR=${CMAKE_CURRENT_BINARY_DIR}/release-package-fixtures"
      "-DC_COMPILER=${CMAKE_C_COMPILER}" "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
      "-DGENERATOR=${CMAKE_GENERATOR}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/release_package.cmake")
  set_tests_properties(portals.release-package PROPERTIES TIMEOUT 90)

  find_package(Python3 3.10 QUIET COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME portals.release-metadata COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/release_metadata.py")
    add_test(NAME portals.release-publish COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/release_publish.py")
    add_test(NAME portals.release-start COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/release_start.py")
    set_tests_properties(portals.release-metadata portals.release-publish PROPERTIES TIMEOUT 60)
    # This integration test exercises disposable Git repositories and both shells.
    set_tests_properties(portals.release-start PROPERTIES TIMEOUT 180)
  endif()

  include("${CMAKE_CURRENT_SOURCE_DIR}/vendor/doctest/verify.cmake")
  add_executable(portals_compact_text tests/compact_text.cpp)
  target_include_directories(portals_compact_text SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/doctest")
  target_link_libraries(portals_compact_text PRIVATE portals::core)
  portals_enable_warnings(portals_compact_text)
  add_test(NAME portals.compact-text COMMAND portals_compact_text)
  add_executable(portals_aggregate_columns tests/aggregate_columns.cpp)
  target_include_directories(portals_aggregate_columns SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/doctest")
  target_link_libraries(portals_aggregate_columns PRIVATE portals::aggregate)
  portals_enable_warnings(portals_aggregate_columns)
  add_test(NAME portals.aggregate-columns COMMAND portals_aggregate_columns)
  set_tests_properties(portals.compact-text portals.aggregate-columns PROPERTIES TIMEOUT 15)
  add_executable(portals_legacy tests/legacy.cpp)
  target_include_directories(portals_legacy SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/doctest")
  target_link_libraries(portals_legacy PRIVATE portals::core)
  portals_enable_warnings(portals_legacy)
  add_test(NAME portals.legacy COMMAND portals_legacy)
  if(WIN32)
    add_executable(portals_legacy_windows tests/legacy_windows_probe.cpp)
    target_link_libraries(portals_legacy_windows PRIVATE portals::core user32)
    portals_enable_warnings(portals_legacy_windows)
    add_test(NAME portals.legacy.windows-mode COMMAND portals_legacy_windows)
    set_tests_properties(portals.legacy.windows-mode PROPERTIES TIMEOUT 15)
  endif()
  set_tests_properties(portals.legacy PROPERTIES TIMEOUT 15)

  add_executable(portals_ui tests/ui.cpp)
  target_link_libraries(portals_ui PRIVATE portals::core)
  portals_enable_warnings(portals_ui)
  add_test(NAME portals.ui COMMAND portals_ui)
  set_tests_properties(portals.ui PROPERTIES TIMEOUT 20)
  add_executable(portals_ui_virtual tests/package_consumer/virtual_declarative_lifecycle.cpp)
  target_link_libraries(portals_ui_virtual PRIVATE portals::core)
  portals_enable_warnings(portals_ui_virtual)
  add_test(NAME portals.ui-virtual COMMAND portals_ui_virtual)

  add_executable(portals_ui_named tests/ui_named.cpp)
  target_link_libraries(portals_ui_named PRIVATE portals::core)
  portals_enable_warnings(portals_ui_named)
  add_test(NAME portals.ui-named COMMAND portals_ui_named)

  add_executable(portals_table_column tests/table_column.cpp)
  target_link_libraries(portals_table_column PRIVATE portals::core)
  portals_enable_warnings(portals_table_column)
  add_test(NAME portals.table-column COMMAND portals_table_column)
  set_tests_properties(portals.ui-virtual PROPERTIES TIMEOUT 20)

  add_executable(portals_managed tests/managed.cpp)
  target_link_libraries(portals_managed PRIVATE portals::core)
  portals_enable_warnings(portals_managed)
  add_test(NAME portals.managed COMMAND portals_managed)
  set_tests_properties(portals.managed PROPERTIES TIMEOUT 20)
  add_executable(portals_terminal_cleanup tests/terminal_cleanup.cpp)
  target_link_libraries(portals_terminal_cleanup PRIVATE portals::core)
  if(WIN32)
    target_sources(portals_terminal_cleanup PRIVATE tests/support/native_windows.cpp)
    target_link_libraries(portals_terminal_cleanup PRIVATE user32)
  elseif(UNIX)
    target_sources(portals_terminal_cleanup PRIVATE tests/support/native_posix.cpp)
    find_package(X11 QUIET)
    if(X11_FOUND AND TARGET X11::Xtst)
      target_compile_definitions(portals_terminal_cleanup PRIVATE PORTALS_NATIVE_X11)
      target_link_libraries(portals_terminal_cleanup PRIVATE X11::X11 X11::Xtst)
    endif()
  endif()
  portals_enable_warnings(portals_terminal_cleanup)
  # Native launcher tests are opt-in: an ordinary CTest process has no emulator.
  if(PORTALS_BUILD_TOOLS)
    add_test(NAME portals.managed-measurement COMMAND "${CMAKE_COMMAND}"
      -DPROGRAM=$<TARGET_FILE:portals_managed_benchmark>
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/benchmarks/check_managed.cmake")
    set_tests_properties(portals.managed-measurement PROPERTIES TIMEOUT 30)
    add_test(NAME portals.mvp-measurement COMMAND "${CMAKE_COMMAND}"
      -DPROGRAM=$<TARGET_FILE:portals_mvp_benchmark> -DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/benchmarks/check.cmake")
    set_tests_properties(portals.mvp-measurement PROPERTIES TIMEOUT 30)
  endif()
  add_executable(portals_mvp_acceptance tests/mvp_acceptance.cpp)
  target_link_libraries(portals_mvp_acceptance PRIVATE portals::core)
  portals_enable_warnings(portals_mvp_acceptance)
  add_test(NAME portals.mvp-acceptance COMMAND portals_mvp_acceptance)
  set_tests_properties(portals.mvp-acceptance PROPERTIES TIMEOUT 20)
  add_executable(portals_managed_virtual tests/package_consumer/virtual_managed_lifecycle.cpp)
  target_link_libraries(portals_managed_virtual PRIVATE portals::core)
  portals_enable_warnings(portals_managed_virtual)
  add_test(NAME portals.managed-virtual COMMAND portals_managed_virtual)
  set_tests_properties(portals.managed-virtual PROPERTIES TIMEOUT 20)

  add_executable(portals_dashboard_tests tests/dashboard.cpp)
  target_link_libraries(portals_dashboard_tests PRIVATE portals::core)
  portals_enable_warnings(portals_dashboard_tests)
  add_test(NAME portals.dashboard COMMAND portals_dashboard_tests)
  set_tests_properties(portals.dashboard PROPERTIES TIMEOUT 20)
  add_executable(portals_application tests/application.cpp)
  target_link_libraries(portals_application PRIVATE portals::core)
  portals_enable_warnings(portals_application)
  add_test(NAME portals.application COMMAND portals_application)
  set_tests_properties(portals.application PROPERTIES TIMEOUT 15)
  add_executable(portals_style tests/style.cpp)
  target_link_libraries(portals_style PRIVATE portals::core)
  portals_enable_warnings(portals_style)
  add_test(NAME portals.style COMMAND portals_style "${CMAKE_CURRENT_SOURCE_DIR}/themes")
  set_tests_properties(portals.style PROPERTIES TIMEOUT 15)
  enable_testing()
  add_test(NAME portals.bundle-scripts COMMAND "${CMAKE_COMMAND}"
    "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}" "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/bundle_script_test.cmake")
  set_tests_properties(portals.bundle-scripts PROPERTIES TIMEOUT 30 LABELS "package")
  set(portals_package_test_command "${CMAKE_COMMAND}"
    "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}" "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    "-DCONFIG=$<CONFIG>" "-DGENERATOR=${CMAKE_GENERATOR}"
    "-DGENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}" "-DGENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
    "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}" "-DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
    "-DMSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}")
  add_test(NAME portals.package-install COMMAND ${portals_package_test_command}
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/package_install.cmake")
  if("Debug" IN_LIST CMAKE_CONFIGURATION_TYPES AND "Release" IN_LIST CMAKE_CONFIGURATION_TYPES)
    add_test(NAME portals.package-install-multi-config COMMAND ${portals_package_test_command}
      "-DINSTALL_CONFIGS=Debug;Release" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/package_install.cmake")
    set_tests_properties(portals.package-install-multi-config PROPERTIES TIMEOUT 300 LABELS "package")
  endif()
  set_tests_properties(portals.package-install PROPERTIES TIMEOUT 180 LABELS "package")
  add_executable(portals_diagnostics tests/diagnostics.cpp)
  target_link_libraries(portals_diagnostics PRIVATE portals::core)
  portals_enable_warnings(portals_diagnostics)
  add_test(NAME portals.diagnostics COMMAND portals_diagnostics)
  set_tests_properties(portals.diagnostics PROPERTIES TIMEOUT 15)
  add_test(NAME portals.diagnostics-inflight COMMAND portals_diagnostics --inflight)
  add_test(NAME portals.diagnostics-snapshot-allocation COMMAND portals_diagnostics --snapshot-allocation)
  set_tests_properties(portals.diagnostics-inflight portals.diagnostics-snapshot-allocation PROPERTIES TIMEOUT 15)

  add_executable(portals_diagnostics_flow tests/diagnostics_flow.cpp tests/support/test_terminal.cpp)
  target_link_libraries(portals_diagnostics_flow PRIVATE portals::core)
  portals_enable_warnings(portals_diagnostics_flow)
  add_test(NAME portals.diagnostics-flow COMMAND portals_diagnostics_flow)
  set_tests_properties(portals.diagnostics-flow PROPERTIES TIMEOUT 15)
  add_test(NAME portals.diagnostics-export-failure COMMAND "${CMAKE_COMMAND}"
    -DPROGRAM=$<TARGET_FILE:portals_diagnostics_flow> -DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/diagnostics_export.cmake")
  set_tests_properties(portals.diagnostics-export-failure PROPERTIES TIMEOUT 15)

  add_executable(portals_chart tests/chart.cpp)
  target_link_libraries(portals_chart PRIVATE portals::core)
  portals_enable_warnings(portals_chart)
  add_test(NAME portals.chart COMMAND portals_chart)
  set_tests_properties(portals.chart PROPERTIES TIMEOUT 15)

  add_executable(portals_layout_grid tests/layout_grid.cpp tests/support/test_terminal.cpp)
  target_link_libraries(portals_layout_grid PRIVATE portals::core)
  portals_enable_warnings(portals_layout_grid)
  add_test(NAME portals.layout-grid COMMAND portals_layout_grid)
  set_tests_properties(portals.layout-grid PROPERTIES TIMEOUT 15)

  if(PORTALS_BUILD_EXAMPLES)
    add_executable(portals_gallery_managed tests/gallery_managed.cpp)
    target_link_libraries(portals_gallery_managed PRIVATE portals::core)
    portals_enable_warnings(portals_gallery_managed)
    add_test(NAME portals.gallery-managed COMMAND portals_gallery_managed)
    set_tests_properties(portals.gallery-managed PROPERTIES TIMEOUT 20)
    set(portals_launcher_test_command "${CMAKE_COMMAND}"
      "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}" "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
      "-DPROGRAM=$<TARGET_FILE:portals_gallery>")
    if(WIN32)
      find_program(PORTALS_POWERSHELL_EXECUTABLE NAMES pwsh powershell)
      if(PORTALS_POWERSHELL_EXECUTABLE)
        list(APPEND portals_launcher_test_command "-DPOWERSHELL_EXECUTABLE=${PORTALS_POWERSHELL_EXECUTABLE}")
      else()
        set(portals_launcher_test_command "")
        message(STATUS "Bundle launcher test unavailable: PowerShell was not found")
      endif()
    endif()
    if(portals_launcher_test_command)
      add_test(NAME portals.bundle-launcher COMMAND ${portals_launcher_test_command}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/bundle_launcher.cmake")
      set_tests_properties(portals.bundle-launcher PROPERTIES TIMEOUT 40 LABELS "package")
    endif()
    add_test(NAME portals.gallery-replay COMMAND portals_gallery --check)
    add_test(NAME portals.dashboard-check COMMAND portals_dashboard --check --ansi16 --ascii)
    add_test(NAME portals.dashboard-replay COMMAND portals_replay_example --check --delay-ms 250)
    add_test(NAME portals.custom-widget COMMAND portals_custom_widget --check)
    foreach(palette IN ITEMS modern turbo nord gruvbox tokyo-night catppuccin-latte)
      foreach(program IN ITEMS gallery custom_widget)
        add_test(NAME portals.${program}-${palette}
          COMMAND portals_${program} --check --ansi16 --theme ${palette})
        set_tests_properties(portals.${program}-${palette} PROPERTIES TIMEOUT 20)
      endforeach()
    endforeach()
    set_tests_properties(portals.dashboard-check portals.dashboard-replay portals.custom-widget PROPERTIES TIMEOUT 20)
    set_tests_properties(portals.gallery-replay PROPERTIES TIMEOUT 15)
  endif()
  add_executable(portals_line_edit tests/line_edit.cpp tests/support/test_terminal.cpp)
  target_link_libraries(portals_line_edit PRIVATE portals::core)
  portals_enable_warnings(portals_line_edit)
  add_test(NAME portals.line-edit COMMAND portals_line_edit)
  set_tests_properties(portals.line-edit PROPERTIES TIMEOUT 20)
  add_test(NAME portals.line-edit.native COMMAND portals_line_edit --native)
  set_tests_properties(portals.line-edit.native PROPERTIES TIMEOUT 30)

  add_executable(portals_basic_controls tests/basic_controls.cpp tests/support/test_terminal.cpp)
  target_link_libraries(portals_basic_controls PRIVATE portals::core)
  portals_enable_warnings(portals_basic_controls)
  add_test(NAME portals.basic-controls COMMAND portals_basic_controls)
  set_tests_properties(portals.basic-controls PROPERTIES TIMEOUT 15)

  add_executable(portals_layout_linear tests/layout_linear.cpp)
  target_link_libraries(portals_layout_linear PRIVATE portals::core)
  portals_enable_warnings(portals_layout_linear)
  add_test(NAME portals.layout-linear COMMAND portals_layout_linear)
  set_tests_properties(portals.layout-linear PROPERTIES TIMEOUT 15)

  add_executable(portals_scroll tests/scroll.cpp)
  target_link_libraries(portals_scroll PRIVATE portals::core)
  portals_enable_warnings(portals_scroll)
  add_test(NAME portals.scroll COMMAND portals_scroll)
  set_tests_properties(portals.scroll PROPERTIES TIMEOUT 15)

  add_executable(portals_virtual_table tests/virtual_table.cpp tests/support/test_terminal.cpp)
  target_link_libraries(portals_virtual_table PRIVATE portals::core)
  portals_enable_warnings(portals_virtual_table)
  add_test(NAME portals.virtual-table COMMAND portals_virtual_table)
  set_tests_properties(portals.virtual-table PROPERTIES TIMEOUT 15)

  add_executable(portals_virtual_list tests/virtual_list.cpp tests/support/test_terminal.cpp)
  target_link_libraries(portals_virtual_list PRIVATE portals::core)
  portals_enable_warnings(portals_virtual_list)
  add_test(NAME portals.virtual-list COMMAND portals_virtual_list)
  set_tests_properties(portals.virtual-list PROPERTIES TIMEOUT 15)

  add_executable(portals_model tests/model.cpp)
  target_link_libraries(portals_model PRIVATE portals::core)
  portals_enable_warnings(portals_model)
  add_test(NAME portals.model COMMAND portals_model)
  set_tests_properties(portals.model PROPERTIES TIMEOUT 15)

  add_executable(portals_property tests/property.cpp)
  target_link_libraries(portals_property PRIVATE portals::core)
  portals_enable_warnings(portals_property)
  add_test(NAME portals.property COMMAND portals_property)
  set_tests_properties(portals.property PROPERTIES TIMEOUT 15)

  add_executable(portals_focus_layout tests/focus_layout.cpp tests/support/test_terminal.cpp)
  target_link_libraries(portals_focus_layout PRIVATE portals::core)
  portals_enable_warnings(portals_focus_layout)
  add_test(NAME portals.focus-layout COMMAND portals_focus_layout)
  set_tests_properties(portals.focus-layout PROPERTIES TIMEOUT 15)
  add_executable(portals_focus tests/focus.cpp)
  target_link_libraries(portals_focus PRIVATE portals::core)
  portals_enable_warnings(portals_focus)
  add_test(NAME portals.focus COMMAND portals_focus)
  set_tests_properties(portals.focus PROPERTIES TIMEOUT 15)

  add_executable(portals_widget_tree tests/widget_tree.cpp)
  target_link_libraries(portals_widget_tree PRIVATE portals::core)
  portals_enable_warnings(portals_widget_tree)
  add_test(NAME portals.widget-tree COMMAND portals_widget_tree)
  set_tests_properties(portals.widget-tree PROPERTIES TIMEOUT 15)
  add_executable(portals_scheduler tests/scheduler.cpp)
  target_link_libraries(portals_scheduler PRIVATE portals::core)
  portals_enable_warnings(portals_scheduler)
  add_test(NAME portals.scheduler COMMAND portals_scheduler)
  set_tests_properties(portals.scheduler PROPERTIES TIMEOUT 15)
  add_executable(portals_smoke tests/smoke.cpp)
  target_link_libraries(portals_smoke PRIVATE portals::core)
  target_compile_definitions(portals_smoke PRIVATE
    EXPECTED_PORTALS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    EXPECTED_PORTALS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    EXPECTED_PORTALS_VERSION_PATCH=${PROJECT_VERSION_PATCH})
  portals_enable_warnings(portals_smoke)
  add_test(NAME portals.smoke COMMAND portals_smoke)
  add_executable(portals_geometry tests/geometry.cpp)
  target_link_libraries(portals_geometry PRIVATE portals::core)
  portals_enable_warnings(portals_geometry)
  add_test(NAME portals.geometry COMMAND portals_geometry)
  add_executable(portals_text tests/text.cpp)
  target_link_libraries(portals_text PRIVATE portals::core)
  portals_enable_warnings(portals_text)
  add_test(NAME portals.text COMMAND portals_text "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unicode/GraphemeBreakTest-18.0.0.txt")

  add_executable(portals_cell_buffer tests/cell_buffer.cpp)
  target_link_libraries(portals_cell_buffer PRIVATE portals::core)
  portals_enable_warnings(portals_cell_buffer)
  add_test(NAME portals.cell-buffer COMMAND portals_cell_buffer)
  add_executable(portals_writer tests/writer.cpp)
  target_link_libraries(portals_writer PRIVATE portals::core)
  portals_enable_warnings(portals_writer)
  add_test(NAME portals.writer COMMAND portals_writer)
  set_tests_properties(portals.writer PROPERTIES TIMEOUT 15)
  add_executable(portals_diff tests/diff.cpp)
  target_link_libraries(portals_diff PRIVATE portals::core)
  portals_enable_warnings(portals_diff)
  add_test(NAME portals.diff COMMAND portals_diff)
  set_tests_properties(portals.diff PROPERTIES TIMEOUT 10)
  add_executable(portals_render tests/render.cpp)
  target_link_libraries(portals_render PRIVATE portals::core)
  portals_enable_warnings(portals_render)
  add_test(NAME portals.render COMMAND portals_render)
  add_test(NAME portals.unicode-provenance COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/vendor/utf8proc/verify.cmake")
  set_tests_properties(portals.smoke portals.geometry portals.text portals.cell-buffer portals.render portals.unicode-provenance PROPERTIES TIMEOUT 10)

  add_executable(portals_replay tests/replay.cpp tests/support/test_terminal.cpp)
  target_link_libraries(portals_replay PRIVATE portals::core)
  portals_enable_warnings(portals_replay)
  add_test(NAME portals.replay COMMAND portals_replay "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/traces")
  set_tests_properties(portals.replay PROPERTIES TIMEOUT 15)

  add_executable(portals_input tests/input.cpp)
  target_link_libraries(portals_input PRIVATE portals::core)
  portals_enable_warnings(portals_input)
  add_test(NAME portals.input COMMAND portals_input)
  set_tests_properties(portals.input PROPERTIES TIMEOUT 20)
  if(UNIX)
    add_executable(portals_terminal_posix tests/terminal_posix.cpp)
    target_link_libraries(portals_terminal_posix PRIVATE portals::core Threads::Threads)
    target_include_directories(portals_terminal_posix PRIVATE src/terminal)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      target_compile_definitions(portals_terminal_posix PRIVATE PORTALS_TEST_WRAP_TERMIOS)
      target_link_options(portals_terminal_posix PRIVATE "-Wl,--wrap=tcsetattr" "-Wl,--wrap=write")
    endif()
    portals_enable_warnings(portals_terminal_posix)
    add_test(NAME portals.terminal.posix COMMAND portals_terminal_posix)
    set_tests_properties(portals.terminal.posix PROPERTIES TIMEOUT 15)
  endif()
  if(WIN32)
    add_executable(portals_terminal_windows tests/terminal_windows.cpp)
    target_link_libraries(portals_terminal_windows PRIVATE portals::core)
    portals_enable_warnings(portals_terminal_windows)
    add_test(NAME portals.terminal.windows COMMAND portals_terminal_windows)
    # Sequential ConPTY cases retain their own deadlines; older hosts drain large batches more slowly.
    set_tests_properties(portals.terminal.windows PROPERTIES TIMEOUT 120)
  endif()

  add_executable(portals_probe_query_tests tests/probe_query.cpp tools/terminal_probe/query.cpp)
  target_include_directories(portals_probe_query_tests PRIVATE tools/terminal_probe)
  target_compile_features(portals_probe_query_tests PRIVATE cxx_std_23)
  set_target_properties(portals_probe_query_tests PROPERTIES CXX_EXTENSIONS OFF)
  portals_enable_warnings(portals_probe_query_tests)
  add_test(NAME portals.probe.query COMMAND portals_probe_query_tests)
  set_tests_properties(portals.probe.query PROPERTIES TIMEOUT 10)

  if(WIN32)
    add_executable(portals_probe_windows_tests tests/probe_windows.cpp
      tools/terminal_probe/query.cpp)
    target_include_directories(portals_probe_windows_tests PRIVATE tools/terminal_probe)
    target_compile_features(portals_probe_windows_tests PRIVATE cxx_std_23)
    set_target_properties(portals_probe_windows_tests PROPERTIES CXX_EXTENSIONS OFF)
    portals_enable_warnings(portals_probe_windows_tests)
    add_test(NAME portals.probe.windows-fixture COMMAND portals_probe_windows_tests)
    set_tests_properties(portals.probe.windows-fixture PROPERTIES TIMEOUT 10)
  endif()

  if(PORTALS_BUILD_TOOLS)
    add_test(NAME portals.probe.redirected
      COMMAND ${CMAKE_COMMAND} -DPROBE=$<TARGET_FILE:portals_terminal_probe>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/probe_redirected.cmake)
    set_tests_properties(portals.probe.redirected PROPERTIES TIMEOUT 10)

    if(UNIX)
      add_executable(portals_probe_posix_tests tests/probe_posix.cpp)
      target_compile_features(portals_probe_posix_tests PRIVATE cxx_std_23)
      set_target_properties(portals_probe_posix_tests PROPERTIES CXX_EXTENSIONS OFF)
      portals_enable_warnings(portals_probe_posix_tests)
      add_test(NAME portals.probe.posix-fixture
        COMMAND portals_probe_posix_tests $<TARGET_FILE:portals_terminal_probe>)
      set_tests_properties(portals.probe.posix-fixture PROPERTIES TIMEOUT 15)
    endif()
  endif()

endif()

if(PORTALS_BUILD_DOCS)
  find_package(Doxygen 1.18 REQUIRED)
  find_program(PORTALS_MERMAID_EXECUTABLE NAMES mmdc mmdc.cmd REQUIRED
    DOC "Mermaid CLI used to render offline documentation diagrams")
  get_filename_component(portals_mermaid_directory "${PORTALS_MERMAID_EXECUTABLE}" DIRECTORY)
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_LATEX NO)
  set(DOXYGEN_HAVE_DOT NO)
  set(DOXYGEN_DOT_IMAGE_FORMAT svg)
  set(DOXYGEN_MERMAID_RENDER_MODE CLI)
  set(DOXYGEN_MERMAID_PATH "${portals_mermaid_directory}")
  set(DOXYGEN_MERMAID_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/docs/mermaid.json")
  set(DOXYGEN_HTML_EXTRA_STYLESHEET "${CMAKE_CURRENT_SOURCE_DIR}/docs/diagrams.css")
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/docs/index.md")
  set(DOXYGEN_LAYOUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/docs/DoxygenLayout.xml")
  # Layout 2.0 intentionally inherits Doxygen's member-page defaults.
  set(DOXYGEN_WARN_LAYOUT_FILE NO)
  set(DOXYGEN_GENERATE_TREEVIEW YES)
  set(DOXYGEN_DISABLE_INDEX YES)
  set(DOXYGEN_FULL_SIDEBAR YES)
  set(DOXYGEN_EXAMPLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests/package_consumer"
    "${CMAKE_CURRENT_SOURCE_DIR}/examples" "${CMAKE_CURRENT_SOURCE_DIR}/tests/support")
  set(DOXYGEN_EXAMPLE_RECURSIVE YES)
  set(DOXYGEN_IMAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/docs/images")
  set(DOXYGEN_WARN_AS_ERROR YES)
  set(DOXYGEN_WARN_NO_PARAMDOC YES)
  set(DOXYGEN_WARN_IF_INCOMPLETE_DOC YES)
  set(DOXYGEN_WARN_IF_UNDOC_ENUM_VAL YES)
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")

  doxygen_add_docs(portals_docs include ReleaseNotes.md docs/index.md docs/development.md docs/releasing.md docs/geometry.md docs/text.md docs/cell-buffer.md docs/render.md docs/diff.md docs/testing.md docs/scheduler.md docs/widget-tree.md docs/layout.md docs/grid.md docs/property.md docs/diagnostics.md docs/focus.md docs/model.md docs/input.md docs/terminal.md docs/terminal-probe.md docs/controls.md docs/line-edit.md docs/virtual-list.md docs/virtual-table.md docs/scroll.md docs/chart.md
    docs/styles.md docs/legacy.md docs/managed-controls.md docs/declarative-ui.md docs/compact-examples.md docs/managed-performance.md docs/package.md docs/application.md docs/virtual-preparation.md docs/examples.md docs/architecture.md docs/getting-started.md docs/limitations.md docs/api-design.md experiments/module-spike/README.md fuzz/README.md
    vendor/utf8proc/README.md vendor/utf8proc/LICENSE.md vendor/textual/README.md vendor/doctest/README.md vendor/pfr/README.md
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generate Portals API documentation"
  )
  add_custom_command(TARGET portals_docs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt" "${CMAKE_CURRENT_BINARY_DIR}/docs/html/LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/docs/html/vendor/textual"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/textual/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/docs/html/vendor/textual/LICENSE"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/measurements" "${CMAKE_CURRENT_BINARY_DIR}/docs/html/measurements"
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "${CMAKE_CURRENT_BINARY_DIR}/docs/html/tests/fixtures/unicode"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unicode/Unicode-LICENSE.txt"
      "${CMAKE_CURRENT_BINARY_DIR}/docs/html/tests/fixtures/unicode/Unicode-LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} "-DHTML_DIR=${CMAKE_CURRENT_BINARY_DIR}/docs/html"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FinalizeDocumentation.cmake"
    COMMENT "Finalize HTML documentation and linked license notices"
  )
endif()

if(WIN32 AND PORTALS_BUILD_TESTS)
  add_executable(portals_no_dialogs tests/no_dialogs.cpp)
  target_compile_features(portals_no_dialogs PRIVATE cxx_std_23)
  portals_enable_warnings(portals_no_dialogs)
  add_test(NAME portals.test-processes
    COMMAND ${CMAKE_COMMAND} -DPROBE=$<TARGET_FILE:portals_no_dialogs>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/no_dialogs.cmake)
  set_tests_properties(portals.test-processes PROPERTIES TIMEOUT 40)
  include(cmake/TestProcesses.cmake)
  portals_configure_test_processes("${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(PORTALS_BUILD_FUZZERS)
  add_subdirectory(fuzz)
endif()
