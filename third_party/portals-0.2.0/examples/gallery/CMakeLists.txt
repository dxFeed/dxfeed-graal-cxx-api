add_executable(portals_gallery main.cpp)
target_link_libraries(portals_gallery PRIVATE portals::core)
portals_enable_warnings(portals_gallery)
