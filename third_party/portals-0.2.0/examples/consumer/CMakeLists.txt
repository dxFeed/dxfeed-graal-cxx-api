cmake_minimum_required(VERSION 3.24)
project(PortalsConsumer LANGUAGES CXX)

if(NOT TARGET portals::core)
  set(PORTALS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH "Path to the Portals source tree")
  add_subdirectory("${PORTALS_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/portals")
endif()

add_executable(portals_consumer main.cpp)
target_link_libraries(portals_consumer PRIVATE portals::core)
