# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
if(NOT DEFINED PROGRAM)
  message(FATAL_ERROR "PROGRAM is required")
endif()

execute_process(COMMAND "${PROGRAM}" --check
  RESULT_VARIABLE result OUTPUT_VARIABLE report ERROR_VARIABLE errors TIMEOUT 30)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "managed benchmark check: exit ${result}: ${errors}")
endif()

function(check_report report)
  foreach(flag complete correctness_verified check_only)
    string(JSON value GET "${report}" "${flag}")
    if(NOT value)
      message(FATAL_ERROR "managed benchmark check: ${flag} is false")
    endif()
  endforeach()

  string(JSON cases LENGTH "${report}" cases)
  string(JSON samples GET "${report}" samples_per_case)
  string(JSON warmup GET "${report}" warmup_per_case)
  string(JSON unit GET "${report}" unit)
  if(NOT cases EQUAL 6 OR NOT samples EQUAL 3 OR NOT warmup EQUAL 1 OR NOT unit STREQUAL "ns")
    message(FATAL_ERROR "managed benchmark check: wrong finite check workload or units")
  endif()

  set(apis low_level managed descriptions)
  set(phases construction initial_style_register initial_prepare dirty_prepare theme_switch_prepare remove_rows cleanup)
  foreach(case RANGE 0 5)
    math(EXPR api_index "${case} % 3")
    list(GET apis ${api_index} expected_api)
    if(case LESS 3)
      set(expected_rows 2)
    else()
      set(expected_rows 8)
    endif()

    math(EXPR expected_nodes "${expected_rows} * 3 + 1")
    math(EXPR expected_controls "${expected_rows} * 2")
    string(JSON api GET "${report}" cases ${case} api)
    string(JSON rows GET "${report}" cases ${case} rows)
    string(JSON nodes GET "${report}" cases ${case} scene_nodes)
    string(JSON controls GET "${report}" cases ${case} styled_controls)
    string(JSON phase_count LENGTH "${report}" cases ${case} phases)
    if(NOT api STREQUAL expected_api OR NOT rows EQUAL expected_rows OR NOT nodes EQUAL expected_nodes OR
       NOT controls EQUAL expected_controls OR NOT phase_count EQUAL 7)
      message(FATAL_ERROR "managed benchmark check: case ${case} workload differs")
    endif()

    foreach(phase IN LISTS phases)
      string(JSON data GET "${report}" cases ${case} phases "${phase}")
      string(JSON count GET "${data}" count)
      string(JSON retained LENGTH "${data}" samples)
      if(NOT count EQUAL 3 OR NOT retained EQUAL count)
        message(FATAL_ERROR "managed benchmark check: case ${case}/${phase} sample count differs")
      endif()

      set(values)
      foreach(index RANGE 0 2)
        string(JSON value GET "${data}" samples ${index})
        if(NOT value MATCHES "^[0-9]+$")
          message(FATAL_ERROR "managed benchmark check: invalid duration")
        endif()
        list(APPEND values "${value}")
      endforeach()

      list(SORT values COMPARE NATURAL ORDER ASCENDING)
      list(GET values 0 expected_min)
      list(GET values 1 expected_p50)
      list(GET values 2 expected_p95)
      set(expected_max "${expected_p95}")
      foreach(stat min p50 p95 max)
        string(JSON actual GET "${data}" "${stat}")
        if(NOT actual EQUAL expected_${stat})
          message(FATAL_ERROR "managed benchmark check: case ${case}/${phase}/${stat} aggregation differs")
        endif()
      endforeach()
    endforeach()
  endforeach()
endfunction()

check_report("${report}")

# Each invocation owns one unique file; concurrent build configurations cannot collide.
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef suffix)
set(output_path "${CMAKE_CURRENT_BINARY_DIR}/managed-check-${suffix}.json")
execute_process(COMMAND "${PROGRAM}" --check --output "${output_path}"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(NOT result EQUAL 0 OR NOT output STREQUAL "" OR NOT EXISTS "${output_path}")
  message(FATAL_ERROR "managed benchmark file output: exit ${result}: ${errors}")
endif()
file(READ "${output_path}" report)
file(REMOVE "${output_path}")
check_report("${report}")

# No parent exists at this unique path, so opening the output must fail visibly.
execute_process(COMMAND "${PROGRAM}" --check --output "${output_path}/missing/result.json"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
if(result EQUAL 0 OR NOT output STREQUAL "" OR errors STREQUAL "")
  message(FATAL_ERROR "managed benchmark accepted an unwritable output path")
endif()

foreach(arguments "--rows;0" "--rows;1025" "--samples;0" "--samples;201" "--warmup;51" "--unknown;1")
  execute_process(COMMAND "${PROGRAM}" ${arguments}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 10)
  if(result EQUAL 0 OR NOT output STREQUAL "" OR errors STREQUAL "")
    message(FATAL_ERROR "managed benchmark check: invalid options accepted: ${arguments}")
  endif()
endforeach()

