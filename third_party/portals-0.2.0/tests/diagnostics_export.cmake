if(NOT DEFINED PROGRAM OR NOT DEFINED OUTPUT_DIRECTORY)
  message(FATAL_ERROR "PROGRAM and OUTPUT_DIRECTORY are required")
endif()
# Opening a directory as an output file must fail on each supported platform.
execute_process(COMMAND "${PROGRAM}" "${OUTPUT_DIRECTORY}" RESULT_VARIABLE result)
if(NOT result EQUAL 2)
  message(FATAL_ERROR "Expected observable export error 2, got ${result}")
endif()
