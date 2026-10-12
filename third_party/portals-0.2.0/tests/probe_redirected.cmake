# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0

# execute_process captures stdout through a pipe, so queries must never be sent.
foreach(arguments IN ITEMS "default" "--no-query" "--headless")
  if(arguments STREQUAL "default")
    set(options --timeout-ms 1)
    set(expected "skipped: redirected or unavailable native terminal handles")
  else()
    set(options ${arguments} --timeout-ms 1)
    set(expected "disabled")
  endif()

  execute_process(COMMAND "${PROBE}" ${options}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 3)
  if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Redirected probe failed or hung (${arguments}): ${result} ${error}")
  endif()
  string(FIND "${output}" "query.stop=\"${expected}\"" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "Redirected probe did not report the expected query disposition")
  endif()
  string(ASCII 27 escape)
  string(FIND "${output}" "${escape}" found)
  if(NOT found EQUAL -1)
    message(FATAL_ERROR "Redirected probe emitted raw ESC bytes")
  endif()
  string(FIND "${output}" "input.bytes_consumed=\"0\"" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "Redirected probe consumed input")
  endif()
endforeach()

string(ASCII 10 newline)
set(host_hint "host${escape}${newline}\"\\")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "TERM_PROGRAM=${host_hint}" "${PROBE}" --headless
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 3)
string(FIND "${output}" "${escape}" raw_escape)
string(FIND "${output}" "env.TERM_PROGRAM=\"host\\x1b\\x0a\\\"\\\\\"" escaped_hint)
if(NOT result STREQUAL "0" OR NOT raw_escape EQUAL -1 OR escaped_hint EQUAL -1)
  message(FATAL_ERROR "Untrusted environment values were not escaped: ${result} ${error}")
endif()

string(REPEAT "x" 1025 oversized_hint)
string(REPEAT "x" 1024 bounded_hint)
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "TERM_PROGRAM=${oversized_hint}" "${PROBE}" --headless
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 3)
string(FIND "${output}" "env.TERM_PROGRAM=\"${bounded_hint}\"" bounded_field)
string(FIND "${output}" "env.TERM_PROGRAM.truncated=\"true\"" truncated_field)
if(NOT result STREQUAL "0" OR bounded_field EQUAL -1 OR truncated_field EQUAL -1)
  message(FATAL_ERROR "Oversized environment value did not preserve its bounded field and truncation marker")
endif()

execute_process(COMMAND "${PROBE}" --help RESULT_VARIABLE result OUTPUT_VARIABLE output TIMEOUT 3)
if(NOT result STREQUAL "0" OR NOT output MATCHES "Usage: portals_terminal_probe")
  message(FATAL_ERROR "Probe help failed")
endif()

foreach(value IN ITEMS "0" "10001" "-1" "abc" "42949672960")
  execute_process(COMMAND "${PROBE}" --timeout-ms "${value}"
    RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET TIMEOUT 3)
  if(NOT result STREQUAL "2")
    message(FATAL_ERROR "Invalid timeout did not produce usage error: ${value}, result=${result}")
  endif()
endforeach()
