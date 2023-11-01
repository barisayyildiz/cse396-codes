### Wiringpi çalıştırma

```
g++ main.cpp -o main -l wiringPi
sudo ./main
```

### opencv çalıtırma

```
cmake .
make
sudo ./main
```

### beraber link lemek için
```
cmake_minimum_required(VERSION 2.8)
project(Converter)

find_package(OpenCV REQUIRED)
find_library(WIRINGPI_LIBRARY wiringPi) # Find the WiringPi library

include_directories(${OpenCV_INCLUDE_DIRS})
add_executable(main main.cpp)

target_link_libraries(main ${OpenCV_LIBS} ${WIRINGPI_LIBRARY}) # Link both OpenCV and WiringPi

```
