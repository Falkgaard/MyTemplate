#pragma once
cmake_minimum_required ( VERSION 3.21 FATAL_ERROR ) # TODO: expand version range
if ( PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR )
	message( FATAL_ERROR "In-source builds are disabled. Please make a build directory and run CMake from there." )
endif ()

# Configure project name and version here:
project (
	MyTemplate    # <- project name
	VERSION       0.1.0
	DESCRIPTION   "A project template."
	LANGUAGES     CXX
)

configure_file ( # for exporting info to C++ (e.g. project version)
	"${CMAKE_CURRENT_SOURCE_DIR}/src/info.hh.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/info.hh"
)

set          ( CMAKE_EXPORT_COMPILE_COMMANDS       ON ) # generate compile_commands.json
set          ( CMAKE_POSITION_INDEPENDENT_CODE     ON ) # -fPIC
set          ( CMAKE_INTERPROCEDURAL_OPTIMIZATION  ON )
cmake_policy ( SET                  CMP0069       NEW )
set          ( CMAKE_POLICY_DEFAULT_CMP0069       NEW )

add_executable (
	${PROJECT_NAME}
	src/main.cc
)

target_compile_definitions( ${PROJECT_NAME} PUBLIC VULKAN_HPP_NO_CONSTRUCTORS )
target_compile_definitions( ${PROJECT_NAME} PUBLIC GLFW_INCLUDE_NONE )
target_compile_definitions( ${PROJECT_NAME} PUBLIC GLFW_INCLUDE_VULKAN )
#add_compile_definitions( ${PROJECT_NAME} PUBLIC VULKAN_HPP_NO_CONSTRUCTORS            )
#add_compile_definitions( ${PROJECT_NAME} PUBLIC GLFW_INCLUDE_NONE GLFW_INCLUDE_VULKAN )

target_compile_features    ( ${PROJECT_NAME} PUBLIC cxx_std_20                                        ) # Set C++ standard to C++20
# TODO: target_compile_options     ( ${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->" ) # To help ensure cross-platform compatibility
target_include_directories ( ${PROJECT_NAME} PUBLIC "src/"                                            ) # Source code
set_target_properties      ( ${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF                            ) # For cross-platform compatibility

#------------------------ External Dependencies: -------------------------#
set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} )
include ( cmake/CPM.cmake )
CPMAddPackage( "gh:fmtlib/fmt#d141cdbeb0fb422a3fb7173b285fd38e0d1772dc"                  ) # {fmt}   v8.0.1
CPMAddPackage( "gh:onqtam/doctest#4d8716f1efc1d14aa736ef52ee727bd4204f4c40"              ) # DocTest v2.4.6
CPMAddPackage( "gh:gabime/spdlog#eb3220622e73a4889eee355ffa37972b3cac3df5"               ) # SpdLog  v1.9.2
CPMAddPackage( "gh:glfw/glfw#d3b73abba0cab8cbb2a638151477f54d8502a07e"                   ) # GLFW    v3.3.5
CPMAddPackage( "gh/g-truc/glm#bf71a834948186f4097caa076cd2663c69a10e1e"                  ) # GLM     v0.9.9.8
CPMAddPackage( "gh:KhronosGroup/Vulkan-Headers#a15237165443ba1ef430ed332745f9a99ec509ad" ) # Vulkan  v1.2.200
CPMAddPackage( "gh:falkgaard/fHash@0.1"                                                  ) # fRNG    v0.1.0
CPMAddPackage( "gh:falkgaard/fRNG@0.1"                                                   ) # fHash   v0.1.0
# ImGui is directly in src/
target_link_libraries( ${PROJECT_NAME} PUBLIC
	doctest::doctest
	spdlog::spdlog
	fmt::fmt
	glfw
	glm::glm
	Vulkan::Headers
	# ^ add library dependency targets here
)
target_link_libraries( ${PROJECT_NAME} PUBLIC
	fHash
	fRNG
	# ^ add header only library dependency targets here
)

# TODO: OpenMP, intrinsics, vulkan, imgui...
# TODO: Presets
# TODO: Pre-compiled headers
