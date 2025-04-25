cmake_minimum_required(VERSION 3.10)
project(imgui)

file(GLOB IMGUI_SOURCES "imgui/imgui*.cpp")

set(SRCS 
	imgui/backends/imgui_impl_opengl3.cpp
	imgui/backends/imgui_impl_sdl3.cpp
	${IMGUI_SOURCES}
)

add_library(imgui ${SRCS})

target_include_directories(imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
target_include_directories(imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../SDL/include)