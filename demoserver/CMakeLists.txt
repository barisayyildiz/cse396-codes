cmake_minimum_required(VERSION 2.8)
project(DemoServer)
find_package(OpenCV REQUIRED)

# find_library(WIRINGPI_LIBRARY wiringPi) # Find the WiringPi library

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(header)
add_executable(test test.cpp source/scanner.cpp source/communication_layer.cpp)
target_link_libraries( test ${OpenCV_LIBS} pthread)
