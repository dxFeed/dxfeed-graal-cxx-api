# Compile intentionally invalid public API usage through the installed target.
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --config "${CONFIG}" --target "${TARGET}"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
set(diagnostic "${output}\n${error}")
if(result EQUAL 0)
  message(FATAL_ERROR "Invalid typed column unexpectedly compiled")
endif()
if(NOT diagnostic MATCHES "ColumnFormatter|ColumnProjection")
  message(FATAL_ERROR "Compilation failed without a typed-column constraint diagnostic:\n${diagnostic}")
endif()
message(STATUS "Rejected invalid typed column at its public constraint:\n${diagnostic}")
