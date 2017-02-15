project(shadow CXX)
enable_testing()

SET(CMAKE_CXX_FLAGS "-std=c++11 -fpermissive -Wall")

cmake_minimum_required(VERSION 2.6)

set (ENV{OpenCV_DIR} "/usr/local/opencv")

find_package( OpenCV REQUIRED )

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
endif()

add_executable(shadow
shadow.cpp
        FlowCalculator.hpp)

target_link_libraries(shadow glut GL GLU ${OpenCV_LIBS} )
