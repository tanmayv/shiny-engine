add_executable(screen_capture_server main.cpp)

target_link_libraries(
  screen_capture_server
  PRIVATE shiny_engine::shiny_engine_options
          shiny_engine::shiny_engine_warnings)

target_link_system_libraries(
  screen_capture_server
  PRIVATE
          fmt::fmt
          absl::base
          spdlog::spdlog
          )

target_include_directories(intro PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
