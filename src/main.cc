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
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
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

bool constexpr is_debug_mode {
	#if !defined( NDEBUG )
		true
	#else
		false
	#endif
};

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

namespace { // unnamed namespace for file scope
	std::array const required_validation_layers {
		"VK_LAYER_LUNARG_standard_validation" // TODO: add more, make customization point
	};
	
	void
	enableValidationLayers( vk::raii::Context &context, vk::InstanceCreateInfo &instance_create_info )
		noexcept( not is_debug_mode )
	{
		if constexpr ( is_debug_mode ) { // logic is only required for debug builds
			auto const available_validation_layers { context.enumerateInstanceLayerProperties() };
			// print required and available layers:
			for ( auto const &layer : required_validation_layers )
				spdlog::info( "Required layer: `{}`", layer );
			for ( auto const &layer : available_validation_layers )
				spdlog::info( "Available layer: `{}`", layer.layerName );
			bool is_missing_layer { false };
			// ensure required layers are available:
			for ( auto const &target : required_validation_layers ) {
				bool match_found { false };
				for ( auto const &layer : available_validation_layers ) {
					if ( std::strcmp( target, layer.layerName ) != 0 ) {
						match_found = true;
						break;
					}
				}
				if ( not match_found ) {
					is_missing_layer = true;
					spdlog::error( "Missing layer: `{}`!", target );
				}
			}
			// handle success or failure:
			if ( is_missing_layer )
				throw std::runtime_error( "Failed to load required validation layers!" );
			else {
				u32 const layer_count = static_cast<u32>( required_validation_layers.size() );
				instance_create_info.enabledLayerCount   = layer_count;
				instance_create_info.ppEnabledLayerNames = required_validation_layers.data();
			}
		}
	} // end-of-function: enableValidationLayers
	
	void
	enableInstanceExtensions( vk::InstanceCreateInfo &instance_create_info )
	{
		u32  extension_count;
		auto extension_data { glfwGetRequiredInstanceExtensions( &extension_count ) };
		if ( not extension_data )
			throw std::runtime_error( "Failed to get required Vulkan instance extensions!" );
		else {
			for ( i32 i=0; i<extension_count; ++i ) {
				spdlog::info( "Required extension: {}", extension_data[i] );
			}
			instance_create_info.enabledExtensionCount   = extension_count;
			instance_create_info.ppEnabledExtensionNames = extension_data;
		}
	} // end-of-function: enableInstanceExtensions
} // end-of-namespace: <unnamed>

int main() {
	spdlog::info(
		"Starting MyTemplate v{}.{}.{}...",
		MYTEMPLATE_VERSION_MAJOR, MYTEMPLATE_VERSION_MINOR, MYTEMPLATE_VERSION_PATCH
	);
	
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
	
	try {
		// TODO: split engine/app versions and make customization point
		auto const version {
			VK_MAKE_VERSION(
				MYTEMPLATE_VERSION_MAJOR,
				MYTEMPLATE_VERSION_MINOR,
				MYTEMPLATE_VERSION_PATCH
			)
		};
		vk::raii::Context   context  {};
		vk::ApplicationInfo app_info {
			.pApplicationName   = "MyTemplate App", // TODO: make customization point 
			.applicationVersion =  version,         // TODO: make customization point 
			.pEngineName        = "MyTemplate Engine",
			.engineVersion      =  version,
			.apiVersion         =  VK_API_VERSION_1_1
		};
		
		// vk::DebugUtilsMessengerCreateInfoEXT
		
		vk::InstanceCreateInfo instance_create_info { .pApplicationInfo = &app_info };
		enableValidationLayers( context, instance_create_info );
		enableInstanceExtensions( instance_create_info );
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
		auto *window_p = glfwCreateWindow( 640, 480, "MyTemplate", nullptr, nullptr );
		auto  result   = glfwCreateWindowSurface(               // SUS
			*instance,                                           // SUS
			 window_p,
			 nullptr,
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
