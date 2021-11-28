#include "info.hh"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

#include "fRNG/core.hh"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#define  GLFW_INCLUDE_NONE
#define  GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define  VULKAN_HPP_NO_CONSTRUCTORS
//#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cinttypes>

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

/*
int f( int n ) {
	rng::engine e;
	auto const rng_val = rng::get( 0, 100, e );
	spdlog::info("Running f() ...");
	auto result = (n / 100 * 100) + 69;
	fmt::print( "(fmt): App v{}.{}, testing f({}) ... result: {} ... random value: {}\n",
		MYTEMPLATE_VERSION_MAJOR,
		MYTEMPLATE_VERSION_MINOR,
		n,
		result,
		rng_val
	);
	return result;
}
*/

int main() {
	spdlog::info( "Starting MyTemplate..." );
	spdlog::info( "Creating window..." );
	if ( not glfwInit() ) {
		spdlog::critical( "Unable to initialize GLFW!" );
		abort();
	}
	
	if ( not glfwVulkanSupported() ) {
		spdlog::critical( "Vulkan is unavailable!" );
		abort();
	}
	
	glfwDefaultWindowHints();
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	
	u32  ext_count;
	auto ext_array = glfwGetRequiredInstanceExtensions( &ext_count );
	if ( not ext_array ) {
		spdlog::critical( "Unable to get Vulkan extensions!" );
		abort();
	}
	
	for ( i32 i=0; i<ext_count; ++i ) {
		spdlog::info( "Required extension: {}", ext_array[i] );
	}
	
	try {
		vk::raii::Context   context  {};
		vk::ApplicationInfo app_info {
			.pApplicationName   = "MyTemplate App",
			.applicationVersion =  1,
			.pEngineName        = "MyTemplate Engine",
			.engineVersion      =  1,
			.apiVersion         =  VK_API_VERSION_1_2
		};
		vk::InstanceCreateInfo instance_create_info {
			.pApplicationInfo        = &app_info,
			.enabledExtensionCount   =  ext_count,
			.ppEnabledExtensionNames =  ext_array
		};
		vk::raii::Instance instance( context, instance_create_info );
		#if !defined( NDEBUG )
			 // TODO: vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger
		#endif
		// enumerate physical devices:
///////////////////////////////////////////////////////////////////////////////////////
		// the big TODO
///////////////////////////////////////////////////////////////////////////////////////
		vk::raii::PhysicalDevices physical_devices( instance ); // SUS
		auto surface   = vk::SurfaceKHR();                      // SUS
		auto surface_c = static_cast<VkSurfaceKHR>( surface );  // SUS
		auto *window_p = glfwCreateWindow( 640, 480, "MyTemplate", NULL, NULL );
		auto  result   = glfwCreateWindowSurface(               // SUS
			*instance,
			 window_p,
			 NULL,
			&surface_c
		);
		if ( result != VkResult::VK_SUCCESS ) {
			spdlog::critical( "Unable to create GLFW window surface!" );
			abort();
		}
		
		// main loop:
		while ( not glfwWindowShouldClose(window_p) ) {
			// TODO: handle input, update logic, render, draw window
			glfwSwapBuffers( window_p );
			glfwPollEvents();
		}
		
		// exiting:
		spdlog::info( "Exiting MyTemplate..." );
		spdlog::info( "Destroying window..." );
		glfwDestroyWindow( window_p );
		spdlog::info( "Terminating GLFW..." );
		glfwTerminate();
		return EXIT_SUCCESS;
	}
	catch ( vk::SystemError const &e ) {
		spdlog::critical( "std::exception: {}", e.what() );
		abort();
	}
	catch ( std::exception const &e ) {
		spdlog::critical( "vk::SystemError: {}", e.what() );
		abort();
	}
	catch (...) {
		spdlog::critical( "Unknown error encountered!" );
		abort();
	}
}

/*
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
TEST_CASE ("Test function") {
	CHECK( f(6)    ==   69 );
	CHECK( f(9)    ==   69 );
	CHECK( f(123)  ==  169 );
	CHECK( f(420)  ==  469 );
	CHECK( f(3234) == 3269 );
}
*/
