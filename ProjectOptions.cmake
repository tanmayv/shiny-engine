include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


macro(shiny_engine_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
    set(SUPPORTS_UBSAN ON)
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    set(SUPPORTS_ASAN ON)
  endif()
endmacro()

macro(shiny_engine_setup_options)
  option(shiny_engine_ENABLE_HARDENING "Enable hardening" ON)
  option(shiny_engine_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    shiny_engine_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    shiny_engine_ENABLE_HARDENING
    OFF)

  shiny_engine_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR shiny_engine_PACKAGING_MAINTAINER_MODE)
    option(shiny_engine_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(shiny_engine_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(shiny_engine_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(shiny_engine_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(shiny_engine_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(shiny_engine_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(shiny_engine_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(shiny_engine_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(shiny_engine_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(shiny_engine_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(shiny_engine_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(shiny_engine_ENABLE_PCH "Enable precompiled headers" OFF)
    option(shiny_engine_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(shiny_engine_ENABLE_IPO "Enable IPO/LTO" ON)
    option(shiny_engine_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(shiny_engine_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(shiny_engine_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(shiny_engine_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(shiny_engine_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(shiny_engine_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(shiny_engine_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(shiny_engine_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(shiny_engine_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(shiny_engine_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(shiny_engine_ENABLE_PCH "Enable precompiled headers" OFF)
    option(shiny_engine_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      shiny_engine_ENABLE_IPO
      shiny_engine_WARNINGS_AS_ERRORS
      shiny_engine_ENABLE_USER_LINKER
      shiny_engine_ENABLE_SANITIZER_ADDRESS
      shiny_engine_ENABLE_SANITIZER_LEAK
      shiny_engine_ENABLE_SANITIZER_UNDEFINED
      shiny_engine_ENABLE_SANITIZER_THREAD
      shiny_engine_ENABLE_SANITIZER_MEMORY
      shiny_engine_ENABLE_UNITY_BUILD
      shiny_engine_ENABLE_CLANG_TIDY
      shiny_engine_ENABLE_CPPCHECK
      shiny_engine_ENABLE_COVERAGE
      shiny_engine_ENABLE_PCH
      shiny_engine_ENABLE_CACHE)
  endif()

  shiny_engine_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (shiny_engine_ENABLE_SANITIZER_ADDRESS OR shiny_engine_ENABLE_SANITIZER_THREAD OR shiny_engine_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(shiny_engine_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

endmacro()

macro(shiny_engine_global_options)
  if(shiny_engine_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    shiny_engine_enable_ipo()
  endif()

  shiny_engine_supports_sanitizers()

  if(shiny_engine_ENABLE_HARDENING AND shiny_engine_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR shiny_engine_ENABLE_SANITIZER_UNDEFINED
       OR shiny_engine_ENABLE_SANITIZER_ADDRESS
       OR shiny_engine_ENABLE_SANITIZER_THREAD
       OR shiny_engine_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${shiny_engine_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${shiny_engine_ENABLE_SANITIZER_UNDEFINED}")
    shiny_engine_enable_hardening(shiny_engine_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(shiny_engine_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(shiny_engine_warnings INTERFACE)
  add_library(shiny_engine_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  shiny_engine_set_project_warnings(
    shiny_engine_warnings
    ${shiny_engine_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(shiny_engine_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    configure_linker(shiny_engine_options)
  endif()

  include(cmake/Sanitizers.cmake)
  shiny_engine_enable_sanitizers(
    shiny_engine_options
    ${shiny_engine_ENABLE_SANITIZER_ADDRESS}
    ${shiny_engine_ENABLE_SANITIZER_LEAK}
    ${shiny_engine_ENABLE_SANITIZER_UNDEFINED}
    ${shiny_engine_ENABLE_SANITIZER_THREAD}
    ${shiny_engine_ENABLE_SANITIZER_MEMORY})

  set_target_properties(shiny_engine_options PROPERTIES UNITY_BUILD ${shiny_engine_ENABLE_UNITY_BUILD})

  if(shiny_engine_ENABLE_PCH)
    target_precompile_headers(
      shiny_engine_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(shiny_engine_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    shiny_engine_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(shiny_engine_ENABLE_CLANG_TIDY)
    shiny_engine_enable_clang_tidy(shiny_engine_options ${shiny_engine_WARNINGS_AS_ERRORS})
  endif()

  if(shiny_engine_ENABLE_CPPCHECK)
    shiny_engine_enable_cppcheck(${shiny_engine_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(shiny_engine_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    shiny_engine_enable_coverage(shiny_engine_options)
  endif()

  if(shiny_engine_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(shiny_engine_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(shiny_engine_ENABLE_HARDENING AND NOT shiny_engine_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR shiny_engine_ENABLE_SANITIZER_UNDEFINED
       OR shiny_engine_ENABLE_SANITIZER_ADDRESS
       OR shiny_engine_ENABLE_SANITIZER_THREAD
       OR shiny_engine_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    shiny_engine_enable_hardening(shiny_engine_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
