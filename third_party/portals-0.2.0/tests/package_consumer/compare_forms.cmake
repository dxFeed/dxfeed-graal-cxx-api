# Compare the complete captured byte stream and the domain result of identical replays.
foreach(variant IN ITEMS BEFORE AFTER)
  execute_process(COMMAND "${${variant}}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "${variant} form failed (${result}): ${error}")
  endif()
  set(${variant}_OUTPUT "${output}")
endforeach()

if(NOT BEFORE_OUTPUT STREQUAL AFTER_OUTPUT)
  message(FATAL_ERROR "Before/after state or complete terminal output differs")
endif()
message(STATUS "Before/after state, callbacks and complete terminal byte stream are identical")
