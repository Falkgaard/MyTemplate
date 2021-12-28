#include "info.hh"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

#include "fRNG/core.hh"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
//#define  GLFW_INCLUDE_NONE
//#define  GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//#define  VULKAN_HPP_NO_CONSTRUCTORS
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
		"VK_LAYER_KHRONOS_validation"
	};
	
	// TODO: refactor common code shared by enableValidationLayers and enableInstanceExtensions.
	
	void
	enableValidationLayers( vk::raii::Context &context, vk::InstanceCreateInfo &instance_create_info )
		noexcept( not is_debug_mode )
	{
		if constexpr ( is_debug_mode ) { // logic is only required for debug builds
			spdlog::info( "Enabling validation layers..." );
			auto const available_validation_layers { context.enumerateInstanceLayerProperties() };
			// print required and available layers:
			for ( auto const &e : required_validation_layers )
				spdlog::info( "Required validation layer: `{}`", e );
			for ( auto const &e : available_validation_layers )
				spdlog::info( "Available validation layer: `{}`", e.layerName );
			bool is_missing_layer { false };
			// ensure required layers are available:
			for ( auto const &target : required_validation_layers ) {
				bool match_found { false };
				for ( auto const &layer : available_validation_layers ) {
					if ( std::strcmp( target, layer.layerName ) == 0 ) {
						match_found = true;
						break;
					}
				}
				if ( not match_found ) {
					is_missing_layer = true;
					spdlog::error( "Missing validation layer: `{}`!", target );
				}
			}
			
			// handle success or failure:
			if ( is_missing_layer )
				throw std::runtime_error( "Failed to load required validation layers!" );
			else {
				instance_create_info.enabledLayerCount   = static_cast<u32>( required_validation_layers.size() );
				instance_create_info.ppEnabledLayerNames = required_validation_layers.data();
			}
		}
	} // end-of-function: enableValidationLayers
	
	std::vector<char const *> required_extensions {
		#if !defined( NDEBUG )
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME ,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME
		#endif
	};
	
	void
	enableInstanceExtensions( vk::raii::Context &context, vk::InstanceCreateInfo &instance_create_info )
	{
		spdlog::info( "Enabling instance extensions..." );
		auto const available_extensions { context.enumerateInstanceExtensionProperties() };
		// get required GLFW extensions:
		u32  glfw_extension_count;
		auto glfw_extension_list { glfwGetRequiredInstanceExtensions( &glfw_extension_count ) };
		if ( glfw_extension_list == nullptr )
			throw std::runtime_error( "Failed to get required Vulkan instance extensions!" );
		else {
			// add required GLFW extensions:
			required_extensions.reserve( glfw_extension_count );
			for ( i32 i=0; i<glfw_extension_count; ++i )
				required_extensions.push_back( glfw_extension_list[i] );
			
			// print required and available extensions:
			for ( auto const &e : required_extensions )
				spdlog::info( "Required instance extension: `{}`", e );
			for ( auto const &e : available_extensions )
				spdlog::info( "Available instance extension: `{}`", e.extensionName );
			
			// ensure required extensions are available:
			bool is_missing_extension { false };
			for ( auto const &target : required_extensions ) {
				bool match_found { false };
				for ( auto const &extension : available_extensions ) {
					if ( std::strcmp( target, extension.extensionName ) == 0 ) {
						match_found = true;
						break;
					}
				}
				if ( not match_found ) {
					is_missing_extension = true;
					spdlog::error( "Missing instance extension: `{}`", target );
				}
			}
			
			// handle success or failure:
			if ( is_missing_extension )
				throw std::runtime_error( "Failed to load required instance extensions!" );
			else {
				instance_create_info.enabledExtensionCount   = static_cast<u32>( required_extensions.size() );
				instance_create_info.ppEnabledExtensionNames = required_extensions.data();
			}
			// set create info fields:
		}
	} // end-of-function: enableInstanceExtensions
	
	vk::raii::PhysicalDevice
	getPhysicalDevice( vk::raii::Instance &instance )
	{
		spdlog::info( "Selecting most suitable physical device..." );
		vk::raii::PhysicalDevices physical_devices( instance );
		if ( physical_devices.empty() )
			throw std::runtime_error( "Unable to find any physical devices!" );
		
		auto best_match {
			std::move( physical_devices.front() )
		};
		
		// replace best match with a discrete GPU (if found)
		if ( best_match.getProperties().deviceType != vk::PhysicalDeviceType::eDiscreteGpu )
			for ( auto &current_physical_device: physical_devices )
				if ( current_physical_device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu ) {
					best_match = std::move( current_physical_device );
					break;
				}
		
		if ( best_match.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
			spdlog::info( "Discrete GPU found!" );
		else 
			spdlog::info( "Only integrated GPU found!" );
		
		std::string const swapchain_extension_name {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		for ( auto const &extension: best_match.enumerateDeviceExtensionProperties() )
			if ( extension.extensionName == swapchain_extension_name )
				return best_match; // swapchain extension support found
		
		throw std::runtime_error( "Physical device does not support swapchains!" );
	} // end-of-function: pickPhysicalDevice
	
	#if !defined( NDEBUG )
		VKAPI_ATTR VkBool32 VKAPI_CALL
		debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT      severity_flags,
			VkDebugUtilsMessageTypeFlagsEXT             type_flags,
			VkDebugUtilsMessengerCallbackDataEXT const *callback_data_p,
			void * // unused for now
		)
		{
			// TODO: replace invocation duplication by using a function map?
			auto const  msg_type {
				vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(type_flags) )
			};
			auto const  msg_severity {
				static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity_flags)
			};
			auto const  msg_id        { callback_data_p->messageIdNumber };
			auto const &msg_id_name_p { callback_data_p->pMessageIdName  };
			auto const &msg_p         { callback_data_p->pMessage        };
			switch (msg_severity) {
				case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
					spdlog::error( "[{}] {} (#{}): {}", msg_type, msg_id_name_p, msg_id, msg_p );
					break;
				}
				case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
					spdlog::info( "[{}] {} (#{}): {}", msg_type, msg_id_name_p, msg_id, msg_p );
					break;
				}
				case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: { // TODO: find better fit?
					spdlog::info( "[{}] {} (#{}): {}", msg_type, msg_id_name_p, msg_id, msg_p );
					break;
				}
				case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
					spdlog::warn( "[{}] {} (#{}): {}", msg_type, msg_id_name_p, msg_id, msg_p );
					break;
				}
			}
			// TODO: expand with more info from callback_data_p
			return false; // TODO: why?
		} // end-of-function: debugCallback
		
		[[nodiscard]] auto
		createDebugMessenger( vk::raii::Context &context, vk::raii::Instance &instance )
		{
			spdlog::info( "Creating debug messenger..." );
			
			auto properties = context.enumerateInstanceExtensionProperties();
			
			// look for debug utils extension:
			auto search_result {
				std::find_if(
					std::begin( properties ),
					std::end(   properties ),
					[]( vk::ExtensionProperties const &p ) {
						return std::strcmp( p.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0;
					}
				)
			};
			if ( search_result == std::end( properties ) )
				throw std::runtime_error( "Could not find " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " extension!" );
			
			auto const severity_flags = vk::DebugUtilsMessageSeverityFlagsEXT(
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
			);
			
			auto const type_flags = vk::DebugUtilsMessageTypeFlagsEXT(
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral     |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
			);
			
			auto const create_info = vk::DebugUtilsMessengerCreateInfoEXT {
				.messageSeverity =  severity_flags,
				.messageType     =  type_flags,
				.pfnUserCallback = &debugCallback
			};
			
			return vk::raii::DebugUtilsMessengerEXT( instance, create_info );
		} // end-of-function: createDebugMessenger
	#endif
	
} // end-of-namespace: <unnamed>

// NOTE: value corresponds to number of frames to allow for static casting
enum struct FrameBuffering: u32 {
	eSingle = 1,
	eDouble = 2,
	eTriple = 3 
}; // end-of-enum-struct: Framebuffering

[[nodiscard]] auto constexpr
getFramebufferCount( FrameBuffering const fb ) noexcept
{
	return static_cast<u32>( fb );
}

enum struct PresentationPriority {
	eMinimalLatency,        
	eMinimalStuttering,     
	eMinimalPowerConsumption
};

[[nodiscard]] auto constexpr
getPresentMode( PresentationPriority const pp ) noexcept
{
	switch ( pp ) {
		case PresentationPriority::eMinimalLatency          : return vk::PresentModeKHR::eMailbox;
		case PresentationPriority::eMinimalStuttering       : return vk::PresentModeKHR::eFifoRelaxed;
		case PresentationPriority::eMinimalPowerConsumption : return vk::PresentModeKHR::eFifoRelaxed;
	}
}

/*
// TODO: make generic or use existing vec type
struct V2i {
	int x,y;
}; // end-of-struct: V2i

class Renderer {
	struct Config {
		V2i             internal_render_resolution;
		FrameBuffering  frame_buffering;
	}; // end-of-struct: Renderer::Config
	// Config
	// Context
	// Instance
	// Physical Device
	// Logical Device
	// Command Buffer Pool
	// Command Buffer(s)
	// Surface
	// Swapchain
}; // end-of-class: Renderer
*/

int
main()
{
	std::atexit( spdlog::shutdown );
	
	spdlog::info(
		"Starting MyTemplate v{}.{}.{}...",
		MYTEMPLATE_VERSION_MAJOR, MYTEMPLATE_VERSION_MINOR, MYTEMPLATE_VERSION_PATCH
	);
	
	if constexpr ( is_debug_mode ) {
		spdlog::set_level( spdlog::level::debug );
		spdlog::info( "Running debug build..." );
	}
	
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
		
	// Context & instance:
		vk::raii::Context context {};
		
		vk::ApplicationInfo app_info {
			.pApplicationName   = "MyTemplate App", // TODO: make customization point 
			.applicationVersion =  version,         // TODO: make customization point 
			.pEngineName        = "MyTemplate Engine",
			.engineVersion      =  version,
			.apiVersion         =  VK_API_VERSION_1_1
		};
		
		vk::InstanceCreateInfo instance_create_info {
			.pApplicationInfo = &app_info
		};
		enableValidationLayers(   context, instance_create_info );
		enableInstanceExtensions( context, instance_create_info );
		spdlog::info( "Creating instance..." );	
		vk::raii::Instance instance( context, instance_create_info );
		
	// Debug messenger:
		#if !defined( NDEBUG )
			auto debug_messenger = createDebugMessenger( context, instance ); // TODO: rename
		#endif
		
	// Physical device:	
		auto physical_device {
			getPhysicalDevice( instance )
		};
		
	// Logical device:
		// TODO: refactor
		u32 const graphics_device_queue_family_index {
			[&physical_device]() -> u32 {
				auto const &queue_family_properties {
					physical_device.getQueueFamilyProperties()
				};
				spdlog::info( "Searching for graphics queue family..." );
				auto const it {
					std::find_if(
						std::begin( queue_family_properties ),
						std::end(   queue_family_properties ),
						[]( vk::QueueFamilyProperties const &queue_family_properties ) {
							return bool { queue_family_properties.queueFlags bitand vk::QueueFlagBits::eGraphics };
						}
					)
				};
				if ( it != std::end(queue_family_properties) ) {
					spdlog::info( "Graphics queue family found!" );
					return static_cast<u32>( std::distance( std::begin(queue_family_properties), it ) );
				}
				else throw std::runtime_error( "Graphics queue family not found!" );
			}()
		};
		u32 const graphics_device_queue_count    {  1  };
		f32 const graphics_device_queue_priority { .0f };
		vk::DeviceQueueCreateInfo const graphics_device_queue_create_info {
			.queueFamilyIndex =  graphics_device_queue_family_index,
			.queueCount       =  graphics_device_queue_count,
			.pQueuePriorities = &graphics_device_queue_priority
		};
		spdlog::info( "Creating logical device..." );
		vk::DeviceCreateInfo const graphics_device_create_info {
			.queueCreateInfoCount =  1,
			.pQueueCreateInfos    = &graphics_device_queue_create_info
			// TODO: add more?
		};
		vk::raii::Device device( physical_device, graphics_device_create_info );
		
	// Command buffer(s):
		spdlog::info( "Creating command buffer pool..." );
		vk::CommandPoolCreateInfo const command_pool_create_info {
			// NOTE: Flags can be set here to optimize for lifetime or enable resetability.
			//       Also, one pool would be needed for each queue family (if ever extended).
			.queueFamilyIndex = graphics_device_queue_family_index
		};
		vk::raii::CommandPool command_pool( device, command_pool_create_info );
		u32 const command_buffer_count { 1 };
		spdlog::info( "Creating {} command buffer(s)...", command_buffer_count );
		vk::CommandBufferAllocateInfo const command_buffer_allocate_info {
			.commandPool        = *command_pool,
			.level              =  vk::CommandBufferLevel::ePrimary, // TODO: comment
			.commandBufferCount =  command_buffer_count              // NOTE: extend here if needed
		};
		vk::raii::CommandBuffers command_buffers( device, command_buffer_allocate_info );
		
	// Window & Surface:
		spdlog::info( "Creating GLFW window surface... ");
		VkSurfaceKHR surface_tmp;
		auto *window_p = glfwCreateWindow( 640, 480, "MyTemplate", nullptr, nullptr ); // TODO: RAII wrapper
		auto  result   = glfwCreateWindowSurface(
			*instance,
			 window_p,
			 nullptr,
			&surface_tmp
		);
		
		vk::raii::SurfaceKHR surface( instance, surface_tmp ); 
		
		if ( result != VkResult::VK_SUCCESS )
			throw std::runtime_error( "Unable to create GLFW window surface!" );
		
	// Swapchain
		spdlog::info( "Creating swapchain..." );	
		// TODO: check present support	
		vk::SwapchainCreateInfoKHR const swapchain_create_info {
			.surface               = *surface,
			.minImageCount         =  getFramebufferCount( FrameBuffering::eTriple ),             // TODO: capabilities query & config refactor
			.imageFormat           =  vk::Format::eA8B8G8R8UnormPack32,                           // TODO: capabilities query & config refactor
			.imageColorSpace       =  vk::ColorSpaceKHR::eExtendedSrgbLinearEXT,
			.imageExtent           =  {}, // TODO
			.imageArrayLayers      =  1, // not-stereoscopic
			.imageUsage            =  vk::ImageUsageFlagBits::eColorAttachment, // change to eTransferDst if doing post-processing
			.imageSharingMode      =  {},
			.preTransform          =  {},
			.compositeAlpha        =  vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode           =  getPresentMode( PresentationPriority::eMinimalStuttering ), // TODO: capabilities query & config refactor
			.clipped               =  VK_TRUE,
			.oldSwapchain          =  VK_NULL_HANDLE // TODO: revisit
			// TODO: queues?
		};
		vk::raii::SwapchainKHR swapchain( device, swapchain_create_info );
		//
	
///////////////////////////////////////////////////////////////////////////////////////
		// the big TODO
		
		// Swapchain
		// Image View
		// Depth Buffer
		// Uniform Buffer
		// Pipeline Layout
		// Desciptor Set
		// Render Pass
		// Shaders
		// Framebuffers
		// Vertex buffer
		// Pipeline
		
		// ...
		
		// vkBeginCommandBuffer()
		// vkCmd???()
		// vkEndCommandBuffer()
		// vkQueueSubmit()
	
///////////////////////////////////////////////////////////////////////////////////////






		
		// main loop:
		while ( not glfwWindowShouldClose(window_p) ) {
			// TODO: handle input, update logic, render, draw window
			glfwSwapBuffers( window_p );
			glfwPollEvents();
		}
		spdlog::info( "Exiting MyTemplate..." );
		spdlog::info( "Destroying window..." );
		glfwDestroyWindow( window_p );
		spdlog::info( "Terminating GLFW..." );
		glfwTerminate();
	}
	catch ( vk::SystemError const &e ) {
		spdlog::critical( "std::exception: {}", e.what() );
		std::exit(EXIT_FAILURE);
	}
	catch ( std::exception const &e ) {
		spdlog::critical( "vk::SystemError: {}", e.what() );
		std::exit(EXIT_FAILURE);
	}
	catch (...) {
		spdlog::critical( "Unknown error encountered!" );
		std::exit(EXIT_FAILURE);
	}
	// exiting:
	std::exit(EXIT_SUCCESS);
} // end-of-function: main

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
