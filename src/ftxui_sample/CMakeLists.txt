add_executable(intro main.cpp)

target_link_libraries(
  intro
  PRIVATE shiny_engine::shiny_engine_options
          shiny_engine::shiny_engine_warnings)

target_link_system_libraries(
  intro
  PRIVATE
          CLI11::CLI11
          fmt::fmt
          spdlog::spdlog
          lefticus::tools
          shiny_engine::sample_library
          ftxui::screen
          ftxui::dom
          ftxui::component)

target_include_directories(intro PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
