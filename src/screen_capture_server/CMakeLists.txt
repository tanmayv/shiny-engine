add_executable(screen_capture_server main.cc ffmpeg.cc)

if(WIN32)
	set(${PROJECT_NAME}_PLATFORM_LIBS Dwmapi)
	add_definitions(-DNOMINMAX)
elseif(APPLE)
    find_package(Threads REQUIRED)
    find_library(corefoundation_lib CoreFoundation REQUIRED)
    find_library(cocoa_lib Cocoa REQUIRED)
    find_library(coremedia_lib CoreMedia REQUIRED)
    find_library(avfoundation_lib AVFoundation REQUIRED)
    find_library(coregraphics_lib CoreGraphics REQUIRED)
    find_library(corevideo_lib CoreVideo REQUIRED)
   
	set(${PROJECT_NAME}_PLATFORM_LIBS
        ${CMAKE_THREAD_LIBS_INIT}
        ${corefoundation_lib}
        ${cocoa_lib}
        ${coremedia_lib}
        ${avfoundation_lib}
        ${coregraphics_lib}  
        ${corevideo_lib}
    ) 
else()
	find_package(X11 REQUIRED)
	if(!X11_XTest_FOUND)
 		message(FATAL_ERROR "X11 extensions are required, but not found!")
	endif()
	if(!X11_Xfixes_LIB)
 		message(FATAL_ERROR "X11 fixes extension is required, but not found!")
	endif()
	find_package(Threads REQUIRED)
	set(${PROJECT_NAME}_PLATFORM_LIBS
		${X11_LIBRARIES}
		${X11_Xfixes_LIB}
		${X11_XTest_LIB}
		${X11_Xinerama_LIB}
		${CMAKE_THREAD_LIBS_INIT}
	)
endif()


target_link_libraries(
  screen_capture_server
  PRIVATE shiny_engine::shiny_engine_options
          shiny_engine::shiny_engine_warnings
          )

target_link_system_libraries(
  screen_capture_server
  PRIVATE
          fmt::fmt
          absl::base
          absl::strings
          absl::status
          spdlog::spdlog
          FFMPEG::avcodec
          FFMPEG::avutil
          FFMPEG::avformat
          screen_capture_lite_static
          ${${PROJECT_NAME}_PLATFORM_LIBS}
          uvgrtp
          )
target_include_directories(screen_capture_server PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
target_include_directories(screen_capture_server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(screen_capture_server PRIVATE "${screen_capture_lite_build_SOURCE_DIR}/include")