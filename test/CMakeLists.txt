cmake_minimum_required(VERSION 3.1)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 14) # C++14...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

option(GTF_Test_ENABLE_COVERAGE "enable coverage" OFF)
# find_package(Threads REQUIRED)

## Set our project name
project(GTF_Test)

set(gtf_test_src
	test.cpp
)
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
  #for coverage
  if(GTF_Test_ENABLE_COVERAGE)
    message(STATUS "coverage enabled")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../cmake/modules)
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    set(COVERAGE_EXCLUDES "*iutest*" "*/usr/include/*" "*mingw*")
    SETUP_TARGET_FOR_COVERAGE(
      NAME GTF_Test_coverage                    # New target name
      EXECUTABLE GTF_Test# Executable in PROJECT_BINARY_DIR
      DEPENDENCIES GTF_Test                     # Dependencies to build first
    )
  endif()
endif()
## Define the executable
add_executable(GTF_Test ${gtf_test_src})
if(WIN32)
  target_link_libraries(GTF_Test ws2_32)
endif()
# target_link_libraries(GTF_Test Threads::Threads)
