cmake_minimum_required(VERSION 3.10)
project(Label)

include_directories("${PROJECT_SOURCE_DIR}/includes")
include_directories(${CMAKE_SOURCE_DIR}/includes/json)
include_directories(${CMAKE_SOURCE_DIR}/includes/imgui)
include_directories(${CMAKE_SOURCE_DIR}/includes/imgui/backends/)
link_directories(".")

set (CMAKE_CXX_COMPILER "/usr/bin/clang++-10")
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)



set(CMAKE_CXX_STANDARD 17)

add_subdirectory(./includes/imgui)


add_executable(Label main.cpp)

target_link_libraries(Label ${OpenCV_LIBRARIES} OpenGL::GL glfw imgui_lib ${CMAKE_DL_LIBS} libjsoncpp.a)