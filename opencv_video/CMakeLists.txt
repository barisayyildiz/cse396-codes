cmake_minimum_required(VERSION 2.8)
project(DisplayImage)

find_package(OpenCV REQUIRED)
find_library(WIRINGPI_LIBRARY wiringPi) # Find the WiringPi library

include_directories(${OpenCV_INCLUDE_DIRS})
add_executable(main main.cpp)

target_link_libraries(main ${OpenCV_LIBS} ${WIRINGPI_LIBRARY}) # Link both OpenCV and WiringPi

# cmake_minimum_required(VERSION 2.8)
# project( DisplayImage )
# find_package( OpenCV REQUIRED )
# include_directories( ${OpenCV_INCLUDE_DIRS} )
# add_executable( main main.cpp )
# target_link_libraries( main ${OpenCV_LIBS} )
