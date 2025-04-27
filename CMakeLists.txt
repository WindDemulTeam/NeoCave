cmake_minimum_required(VERSION 3.10)
project(NeoCave VERSION 7.0 LANGUAGES CXX)

find_package(OpenGL REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -S -fverbose-asm")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -S -fverbose-asm")
elseif(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Fa")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Fa")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(COMMON_BINARY_DIR ${CMAKE_BINARY_DIR}/build)

set(SDL_STATIC ON CACHE BOOL "Build SDL3 as a static library")
set(SDL_SHARED OFF CACHE BOOL "Disable SDL3 shared library")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/ ${COMMON_BINARY_DIR}/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/ ${COMMON_BINARY_DIR}/imgui)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL/ ${COMMON_BINARY_DIR}/SDL)

set(CAVE
	app.cpp
	app.h
	cave.cpp
	cave.h
)

set(SH3
	sh3.cpp
	sh3.h
	sh3_onchip.cpp
	sh3_onchip.h
	sh3_mmu.cpp
	sh3_mmu.h
	sh3_interpreter.cpp
	sh3_interpreter.h
)

set(YMZ770
	ymz770.cpp
	ymz770.h
	amms2.cpp
	amms2.h
	amm2_table.h
)

set(RTC9701
	rtc9701.cpp
	rtc9701.h
)

set(ROMS
	roms.cpp
	roms.h
)

set(NAND
	nand.cpp
	nand.h
)

set(BLITTER
	blitter.cpp
	blitter.h
)

set(COUNTERS
	counters.cpp
	counters.h
)

set(UI
	ui.cpp
	ui.h
	ui_input.cpp
	ui_input.h
	ui_file_dialog.cpp
	ui_file_dialog.h
	ui_game_list.cpp
	ui_game_list.h
)

set(CONFIG
	config.cpp
	config.h
	iconfig.h
	input_manager.cpp
	input_manager.h
)

source_group("CAVE" FILES ${CAVE})
source_group("SH3" FILES ${SH3})
source_group("YMZ770" FILES ${YMZ770})
source_group("RTC9701" FILES ${RTC9701})
source_group("ROMS" FILES ${ROMS})
source_group("NAND" FILES ${NAND})
source_group("BLITTER" FILES ${BLITTER})
source_group("COUNTERS" FILES ${COUNTERS})
source_group("UI" FILES ${UI})
source_group("CONFIG" FILES ${CONFIG})

add_executable(NeoCave ${CAVE} ${SH3} ${YMZ770} ${RTC9701} ${ROMS} ${NAND} ${BLITTER} ${COUNTERS} ${UI} ${CONFIG})

set_target_properties(NeoCave PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)

target_include_directories(NeoCave PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/stb)
target_include_directories(NeoCave PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/toml11/include)
target_include_directories(NeoCave PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui)
target_include_directories(NeoCave PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui/backends)

target_link_libraries(NeoCave PRIVATE glad imgui SDL3::SDL3 OpenGL::GL)
