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

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>

#include <iostream>
#include <array>
#include <optional>
#include <algorithm>
#include <ranges>
#include <algorithm>

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

// TODO: replace with `std::unreachable()` when it becomes available.
# ifdef __GNUC__ // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
	[[noreturn]] inline __attribute__((always_inline)) void unreachable() { __builtin_unreachable(); }
# elif defined(_MSC_VER) // MSVC
	[[noreturn]] __forceinline void unreachable() { __assume(false); }
# else
#    error Unsupported compiler! //	inline void unreachable() {}
# endif


bool constexpr g_is_debug_mode {
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
	enable_validation_layers( vk::raii::Context &context, vk::InstanceCreateInfo &instance_create_info )
		noexcept( not g_is_debug_mode )
	{
		if constexpr ( g_is_debug_mode ) { // logic is only required for debug builds
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
				throw std::runtime_error { "Failed to load required validation layers!" };
			else {
				instance_create_info.enabledLayerCount   = static_cast<u32>( required_validation_layers.size() );
				instance_create_info.ppEnabledLayerNames = required_validation_layers.data();
			}
		}
	} // end-of-function: enable_validation_layers
	
	std::vector<char const *> required_instance_extensions {
		#if !defined( NDEBUG )
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME ,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME
		#endif
	};
	
	void
	enable_instance_extensions( vk::raii::Context &context, vk::InstanceCreateInfo &instance_create_info )
	{
		spdlog::info( "Enabling instance extensions..." );
		auto const available_instance_extensions {
			context.enumerateInstanceExtensionProperties()
		};
		// get required GLFW instance extensions:
		u32  glfw_instance_extension_count;
		auto glfw_instance_extension_list {
			glfwGetRequiredInstanceExtensions( &glfw_instance_extension_count )
		};
		if ( glfw_instance_extension_list == nullptr )
			throw std::runtime_error { "Failed to get required Vulkan instance extensions!" };
		else {
			// add required GLFW instance extensions:
			required_instance_extensions.reserve( glfw_instance_extension_count );
			for ( i32 i=0; i<glfw_instance_extension_count; ++i )
				required_instance_extensions.push_back( glfw_instance_extension_list[i] );
			
			if constexpr ( g_is_debug_mode ) {
				// print required and available instance extensions:
				for ( auto const &required_instance_extension: required_instance_extensions )
					spdlog::info( "... required instance extension: `{}`", required_instance_extension );
				for ( auto const &available_instance_extension: available_instance_extensions )
					spdlog::info( "... available instance extension: `{}`", available_instance_extension.extensionName );
			}
			
			// ensure required instance extensions are available:
			bool is_adequate { true }; // assume true until proven otherwise
			for ( auto const &required_instance_extension: required_instance_extensions ) {
				bool is_supported { false }; // assume false until found
				for ( auto const &available_instance_extension: available_instance_extensions ) {
					if ( std::strcmp( required_instance_extension, available_instance_extension.extensionName ) == 0 ) {
						is_supported = true;
						break; // early exit
					}
				}
				if ( not is_supported ) {
					is_adequate = false;
					spdlog::error( "Missing required instance extension: `{}`", required_instance_extension );
				}
			}
			
			// handle success or failure:
			if ( is_adequate ) {
				// set create info fields:
				instance_create_info.enabledExtensionCount   = static_cast<u32>( required_instance_extensions.size() );
				instance_create_info.ppEnabledExtensionNames = required_instance_extensions.data();
			}
			else throw std::runtime_error { "Failed to load required instance extensions!" };
		}
	} // end-of-function: enable_instance_extensions
	
	std::array constexpr required_device_extensions {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	
	[[nodiscard]] bool
	check_device_extension_support( vk::raii::PhysicalDevice const &physical_device )
	{
		auto const &available_device_extensions {
			physical_device.enumerateDeviceExtensionProperties()
		};
		
		if constexpr ( g_is_debug_mode ) {
			// print required and available device extensions:
			for ( auto const &required_device_extension: required_device_extensions )
				spdlog::info( "... required device extension: `{}`", required_device_extension );
			for ( auto const &available_device_extension: available_device_extensions )
				spdlog::info( "... available device extension: `{}`", available_device_extension.extensionName );
		}	
		
		bool is_adequate { true }; // assume true until proven otherwise
		for ( auto const &required_device_extension: required_device_extensions ) {
			bool is_supported { false }; // assume false until found
			for ( auto const &available_device_extension: available_device_extensions ) {
				if ( std::strcmp( required_device_extension, available_device_extension.extensionName ) == 0 ) {
					is_supported = true;
					break; // early exit
				}
			}
			spdlog::info(
				"... support for required device extension `{}`: {}",
				required_device_extension, is_supported ? "found" : "missing!"
			);
			if ( not is_supported )
				is_adequate = false;
		}
		return is_adequate;
	} // end-of-function: check_device_extension_support
	
	[[nodiscard]] u32
	score_physical_device( vk::raii::PhysicalDevice const &physical_device )
	{
		auto const properties { physical_device.getProperties() }; // TODO: KHR2?
		auto const features   { physical_device.getFeatures()   }; // TODO: 2KHR?
		spdlog::info( "Scoring physical device `{}`...", properties.deviceName.data() );
		u32 score { 0 };
		
		// TODO: when more extensions are required, refactor extension check out into its own function
		bool const is_meeting_device_extension_requirements {
			check_device_extension_support( physical_device )
		};
		if ( not is_meeting_device_extension_requirements )
			spdlog::info( "... device extension support: insufficient!" );
		else if ( features.geometryShader == VK_FALSE )
			spdlog::info( "... geometry shader support: false" );
		else {
			spdlog::info( "... swapchain support: true" );
			spdlog::info( "... geometry shader support: true" );
			score += properties.limits.maxImageDimension2D;
			if ( properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ) {
				spdlog::info( "... type: discrete" );
				score += 10'000;
			}
			else {
				spdlog::info( "... type: integrated" );
			}
			// TODO: add more things if needed
			spdlog::info( "... final score: {}", score );
		}
		return score;
	} // end-of-function: score_physical_device
	
	vk::raii::PhysicalDevice
	select_physical_device( vk::raii::Instance &instance )
	{
		spdlog::info( "Selecting the most suitable physical device..." );
		vk::raii::PhysicalDevices physical_devices( instance );
		spdlog::info( "Found {} physical device(s)...", physical_devices.size() );
		if ( physical_devices.empty() )
			throw std::runtime_error { "Unable to find any physical devices!" };
		
		auto *p_best_match {
			&physical_devices.front()
		};
		
		auto best_score {
			score_physical_device( *p_best_match )
		};
		
		for ( auto &current_physical_device: physical_devices | std::views::drop(1) ) {
			auto const score {
				score_physical_device( current_physical_device )
			};
			if ( score > best_score) {
				best_score   =  score;
				p_best_match = &current_physical_device;
			}
		}
		
		if ( best_score > 0 ) {
			spdlog::info(
				"Selected physical device `{}` with a final score of: {}",
				p_best_match->getProperties().deviceName.data(), best_score
			);
			return std::move( *p_best_match );
		}
		else throw std::runtime_error { "Physical device does not support swapchains!" };
	} // end-of-function: select_physical_device
	
	#if !defined( NDEBUG )
		VKAPI_ATTR VkBool32 VKAPI_CALL
		debug_callback(
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
		} // end-of-function: debug_callback
		
		[[nodiscard]] auto
		create_debug_messenger( vk::raii::Context &context, vk::raii::Instance &instance )
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
				throw std::runtime_error { "Could not find " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " extension!" };
			
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
				.pfnUserCallback = &debug_callback
			};
			
			return vk::raii::DebugUtilsMessengerEXT( instance, create_info );
		} // end-of-function: create_debug_messenger
	#endif
	
} // end-of-namespace: <unnamed>

// NOTE: value corresponds to number of frames to allow for static casting
enum struct FramebufferingPriority: u32 {
	eSingle = 1,
	eDouble = 2,
	eTriple = 3 
}; // end-of-enum-struct: FramebufferingPriority

[[nodiscard]] auto constexpr
get_framebuffer_count(
	vk::SurfaceCapabilitiesKHR const &surface_capabilities,
	FramebufferingPriority     const  framebuffering_priority
) noexcept
{
	auto const ideal_framebuffer_count {
		static_cast<u32>( framebuffering_priority )
	};
	spdlog::info( "Ideal framebuffer count: {}", ideal_framebuffer_count );
	auto const minimum_framebuffer_count {
		surface_capabilities.minImageCount
	};
	auto const maximum_framebuffer_count {
		surface_capabilities.maxImageCount == 0 ? // handle special 0 (uncapped) case
			ideal_framebuffer_count : surface_capabilities.maxImageCount
	};
	auto result {
		std::clamp(
			ideal_framebuffer_count,
			minimum_framebuffer_count,
			maximum_framebuffer_count
		)
	};
	spdlog::info( "Nearest available framebuffer count: {}", result );
	return result;
} // end-of-function: get_framebuffer_count

enum struct PresentationPriority {
	eMinimalLatency,        
	eMinimalStuttering,     
	eMinimalPowerConsumption
};

[[nodiscard]] auto
get_present_mode(
	vk::raii::PhysicalDevice const &physical_device,
	vk::raii::SurfaceKHR     const &surface,
	PresentationPriority     const  presentation_priority
) noexcept
{
	// TODO: add support for ordered priorities instead of just ideal/fallback.
	auto const fallback_present_mode {
		vk::PresentModeKHR::eFifo
	};
	auto const available_present_modes {
		physical_device.getSurfacePresentModesKHR( *surface )
	};
	auto const ideal_present_mode {
		[presentation_priority] {
			switch ( presentation_priority ) {
				case PresentationPriority::eMinimalLatency          : return vk::PresentModeKHR::eMailbox;
				case PresentationPriority::eMinimalStuttering       : return vk::PresentModeKHR::eFifoRelaxed;
				case PresentationPriority::eMinimalPowerConsumption : return vk::PresentModeKHR::eFifo;
			}
			unreachable();
		}()
	};
	bool const has_support_for_ideal_mode {
		std::ranges::find( available_present_modes, ideal_present_mode )
		!= std::end( available_present_modes )
	};
	if ( has_support_for_ideal_mode ) {
		spdlog::info( "Ideal present mode is supported by device!" );
		return ideal_present_mode;
	}
	else {
		spdlog::warn( "Ideal present mode is not supported by device; using fallback present mode!" );
		return fallback_present_mode;
	}
} // end-of-function: get_present_mode

/*
// TODO: make generic or use existing vec type
struct V2i {
	int x,y;
}; // end-of-struct: V2i

class Renderer {
	struct Info {
		// TODO
	} info;
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
	
	if constexpr ( g_is_debug_mode ) {
		spdlog::set_level( spdlog::level::debug );
		spdlog::info( "Build: DEBUG" );
	}
	else
		spdlog::info( "Build: RELEASE" );
	
	spdlog::info( "Initializing GLFW..." );
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
	glfwWindowHint( GLFW_RESIZABLE,  GLFW_FALSE  ); // TODO: remove later
	
	try {
		// TODO: split engine/app versions and make customization point
		auto const version {
			VK_MAKE_VERSION(
				MYTEMPLATE_VERSION_MAJOR,
				MYTEMPLATE_VERSION_MINOR,
				MYTEMPLATE_VERSION_PATCH
			)
		};
		
	// Context:
		spdlog::info( "Creating Vulkan context..." );
		vk::raii::Context context {};
		
	// Instance:
		spdlog::info( "Creating instance..." );	
		
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
		enable_validation_layers(   context, instance_create_info );
		enable_instance_extensions( context, instance_create_info );
		vk::raii::Instance instance( context, instance_create_info );
		
	// Debug messenger:
		#if !defined( NDEBUG )
			auto debug_messenger = create_debug_messenger( context, instance ); // TODO: rename
		#endif
		
	// Physical device:	
		auto physical_device {
			select_physical_device( instance )
		};
		
	// Window:
		spdlog::info( "Creating GLFW window... ");
		auto *p_window = glfwCreateWindow( 640, 480, "MyTemplate", nullptr, nullptr ); // TODO: RAII wrapper
		VkSurfaceKHR surface_tmp;
		auto  result   = glfwCreateWindowSurface(
			*instance,
			 p_window,
			 nullptr,
			&surface_tmp
		);
		
		vk::raii::SurfaceKHR surface( instance, surface_tmp ); 
		
		if ( result != VkResult::VK_SUCCESS )
			throw std::runtime_error { "Unable to create GLFW window surface!" };
		
	// Queue family indices:
		auto const queue_family_indices {
			[&physical_device,&surface]() -> std::array<u32,2> { // TODO: refactor into free function(s)
				std::optional<u32> present_queue_family_index  {};
				std::optional<u32> graphics_queue_family_index {};
				auto const &queue_family_properties {
					physical_device.getQueueFamilyProperties()
				};
				spdlog::info( "Searching for graphics queue family..." );
				for ( u32 index{0}; index<std::size(queue_family_properties); ++index ) {
					bool const supports_graphics { queue_family_properties[index].queueFlags bitand vk::QueueFlagBits::eGraphics };
					bool const supports_present  { physical_device.getSurfaceSupportKHR( index, *surface ) == VK_TRUE };
					if ( supports_graphics and supports_present ) {
						spdlog::info( "Found queue family that supports both graphics and present!" );
						present_queue_family_index  = index;
						graphics_queue_family_index = index;
						break;
					}
					else if ( supports_graphics ) {
						spdlog::info( "Found queue family that supports graphics!" );
						graphics_queue_family_index = index;
					}
					else if ( supports_present ) {
						spdlog::info( "Found queue family that supports present!" );
						present_queue_family_index = index;
					}
				}
				if ( present_queue_family_index.has_value() and graphics_queue_family_index.has_value() ) {
					if ( present_queue_family_index.value() != graphics_queue_family_index.value() )
						spdlog::info( "Selected different queue families for graphics and present." );
					return { present_queue_family_index.value(), graphics_queue_family_index.value() };
				}
				else throw std::runtime_error { "Queue family support for either graphics or present missing!" };
			}()
		};
		
		auto const &present_queue_family_index {
			queue_family_indices[0]
		};
		
		auto const &graphics_queue_family_index {
			queue_family_indices[1]
		};
		
		bool const is_using_separate_queue_families {
			present_queue_family_index != graphics_queue_family_index
		};
		
	// Logical device:
		
		// TODO: refactor approach when more queues are needed
		std::vector<vk::DeviceQueueCreateInfo> device_queue_create_infos {};
		
		{
			f32 const present_queue_priority { 1.0f };
			device_queue_create_infos.push_back(
				vk::DeviceQueueCreateInfo {
					.queueFamilyIndex =  present_queue_family_index,
					.queueCount       =  1,
					.pQueuePriorities = &present_queue_priority
				}
			);
			
			if ( is_using_separate_queue_families ) {
				f32 const graphics_queue_priority { 1.0f };
				device_queue_create_infos.push_back(
					vk::DeviceQueueCreateInfo {
						.queueFamilyIndex =  graphics_queue_family_index,
						.queueCount       =  1,
						.pQueuePriorities = &graphics_queue_priority
					}
				);
			}
		}
		
		spdlog::info( "Creating logical device..." );
		vk::DeviceCreateInfo const graphics_device_create_info {
			.queueCreateInfoCount    = static_cast<u32>( device_queue_create_infos.size() ),
			.pQueueCreateInfos       = device_queue_create_infos.data(),
			.enabledLayerCount       = 0,       // no longer used; TODO: add conditionally to support older versions?
			.ppEnabledLayerNames     = nullptr, // ^ ditto
			.enabledExtensionCount   = static_cast<u32>( required_device_extensions.size() ),
			.ppEnabledExtensionNames = required_device_extensions.data()
		};
		vk::raii::Device device( physical_device, graphics_device_create_info );
	
	// Graphics queue:
		spdlog::info( "Getting graphics queue handle..." );
		auto graphics_queue {
			device.getQueue( graphics_queue_family_index, 0 ) // NOTE: 0 since we only use one queue for now
		};
 	
	 // Present queue:
		spdlog::info( "Getting present queue handle..." );
		auto present_queue {
			device.getQueue( present_queue_family_index, 0 ) // NOTE: 0 since we only use one queue for now
		};
		
	// Command buffer(s):
		spdlog::info( "Creating command buffer pool..." );
		vk::CommandPoolCreateInfo const command_pool_create_info {
			// NOTE: Flags can be set here to optimize for lifetime or enable resetability.
			//       Also, one pool would be needed for each queue family (if ever extended).
			.queueFamilyIndex = graphics_queue_family_index
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
		
	// Swapchain:

		// Swapchain image format:
		auto const surface_format {
		   // TODO: refactor into select_swapchain_surface_format function
			[&physical_device,&surface] {
				auto const available_surface_formats {
					physical_device.getSurfaceFormatsKHR( *surface ) // TODO: 2KHR?
				};
				for ( auto const &available_surface_format: available_surface_formats )
					if ( available_surface_format.format     == vk::Format::eB8G8R8A8Srgb // TODO: refactor out
					and  available_surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear ) // ^ ditto
						return available_surface_format;
				// TODO: add contingency decisions to fallback on
				throw std::runtime_error { "Unable to find the desired surface format!" };	
			}()
		};
			
		// Surface capabilities: (TODO: refactor wrap)
		auto const surface_capabilities {
			physical_device.getSurfaceCapabilitiesKHR( *surface ) // TODO: 2KHR?
		};
			
		// Swapchain image extent:
		vk::Extent2D const surface_extent {
			// TODO: refactor into get_image_extent function
			[&p_window,&surface_capabilities] {
				vk::Extent2D result {};
				if ( surface_capabilities.currentExtent.height == std::numeric_limits<u32>::max() )
					result = surface_capabilities.currentExtent;
				else {
					int width, height;
					glfwGetWindowSize( p_window, &width, &height );
					result.width =
						std::clamp(
							static_cast<u32>(width),
							surface_capabilities.minImageExtent.width,
							surface_capabilities.maxImageExtent.width
						);
					result.height =
						std::clamp(
							static_cast<u32>(height),
							surface_capabilities.minImageExtent.height,
							surface_capabilities.maxImageExtent.height
						);
				}
				return result;
			}()
		};
		
		auto const swapchain_present_mode {
			get_present_mode( physical_device, surface, PresentationPriority::eMinimalStuttering )
		};
		
		auto const swapchain_framebuffer_count {
			get_framebuffer_count( surface_capabilities, FramebufferingPriority::eTriple )	
		};
		
		spdlog::info( "Creating swapchain..." );
		// TODO: check present support	
		vk::SwapchainCreateInfoKHR const swapchain_create_info {
			.surface               = *surface,
			.minImageCount         =  swapchain_framebuffer_count, // TODO: config refactor
			.imageFormat           =  surface_format.format,       // TODO: config refactor
			.imageColorSpace       =  surface_format.colorSpace,   // TODO: config refactor
			.imageExtent           =  surface_extent,
			.imageArrayLayers      =  1, // non-stereoscopic
			.imageUsage            =  vk::ImageUsageFlagBits::eColorAttachment, // NOTE: change to eTransferDst if doing post-processing later
			.imageSharingMode      =  is_using_separate_queue_families ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
			.queueFamilyIndexCount =  is_using_separate_queue_families ?                           2u : 0u,
			.pQueueFamilyIndices   =  is_using_separate_queue_families ?  queue_family_indices.data() : nullptr,
			.preTransform          =  surface_capabilities.currentTransform,
			.compositeAlpha        =  vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode           =  swapchain_present_mode, // TODO: config refactor
			.clipped               =  VK_TRUE,
			.oldSwapchain          =  VK_NULL_HANDLE // TODO: revisit later after implementing resizing
		};
		vk::raii::SwapchainKHR swapchain( device, swapchain_create_info );
		
	// Image views:
		spdlog::info( "Creating swapchain framebuffer image views..." );
		auto swapchain_images { swapchain.getImages() };
		std::vector<vk::raii::ImageView> image_views;
		image_views.reserve( std::size( swapchain_images ) );
		vk::ImageViewCreateInfo image_view_create_info {
			.viewType          = vk::ImageViewType::e2D,
			.format            = surface_format.format, // TODO: verify
			.subresourceRange  = vk::ImageSubresourceRange {
				.aspectMask     = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel   = 0u,
				.levelCount     = 1u,
				.baseArrayLayer = 0u,
				.layerCount     = 1u
			}
		};
		for ( auto const &image: swapchain_images ) {
			image_view_create_info.image = static_cast<vk::Image>( image );
			image_views.emplace_back( device, image_view_create_info );
		}
		
		auto const find_memory_type_index {
			[](
				vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
				u32                                const  type_filter,
				vk::MemoryPropertyFlags            const  memory_property_flags
			) -> u32 {
				for ( u32 i{0}; i<physical_device_memory_properties.memoryTypeCount; ++i ) 
					if ( (type_filter bitand (1<<i)) and
						  (physical_device_memory_properties.memoryTypes[i].propertyFlags bitand memory_property_flags)
						      == memory_property_flags )
					{
						return i;
					}
				throw std::runtime_error { "Unable to find an index of a suitable memory type!" };
			}
		};
		
	// Depth buffer:
		spdlog::info( "Creating depth buffer image..." );
		
		vk::ImageCreateInfo const depth_buffer_image_create_info {
			.imageType             = vk::ImageType::e2D,
			.format                = vk::Format::eD16Unorm, // TODO: query support?
			.extent                = vk::Extent3D {
				.width              = surface_extent.width,
				.height             = surface_extent.height,
				.depth              = 1
			},
			.mipLevels             = 1, // no mipmapping
			.arrayLayers           = 1,
			.samples               = vk::SampleCountFlagBits::e1, // 1 sample per pixel (no MSAA; TODO: revisit later?)
			.tiling                = vk::ImageTiling::eOptimal,
			.usage                 = vk::ImageUsageFlagBits::eDepthStencilAttachment,
			.sharingMode           = vk::SharingMode::eExclusive,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices   = nullptr,
			.initialLayout         = vk::ImageLayout::eUndefined
		};
		
		vk::raii::Image depth_buffer_image( device, depth_buffer_image_create_info );
		
		auto const depth_buffer_image_memory_requirements {
			depth_buffer_image.getMemoryRequirements()
		};
		
		auto const physical_device_memory_properties {
			physical_device.getMemoryProperties()
		};
		
		auto const depth_buffer_image_memory_type_index {
			find_memory_type_index(
				physical_device_memory_properties,
				depth_buffer_image_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			)
		};
		
		vk::MemoryAllocateInfo const depth_buffer_image_memory_allocate_info {
			.allocationSize  = depth_buffer_image_memory_requirements.size,
			.memoryTypeIndex = depth_buffer_image_memory_type_index
		};
		
		vk::raii::DeviceMemory depth_buffer_image_device_memory(
			device,
			depth_buffer_image_memory_allocate_info
		);
		
	// Depth buffer image view:
		spdlog::info( "Creating depth buffer image view..." );
		vk::ImageViewCreateInfo depth_buffer_image_view_create_info {
			.image               = *depth_buffer_image,
			.viewType            =  vk::ImageViewType::e2D,
			.format              =  surface_format.format, // TODO: verify
			.subresourceRange    =  vk::ImageSubresourceRange {
				.aspectMask       =  vk::ImageAspectFlagBits::eDepth,
				.baseMipLevel     =  0u,
				.levelCount       =  1u,
				.baseArrayLayer   =  0u,
				.layerCount       =  1u
			}
		};
		
		vk::raii::ImageView depth_buffer_image_view(
			device,
			depth_buffer_image_view_create_info
		);
		
		// TODO: renderpass stuff + subpass stuff, etc ASAP
		
	// Uniform buffer:
		spdlog::info( "Creating uniform data buffer..." );
		
		auto const projection {
			glm::perspective( glm::radians(45.f), 1.0f, 0.1f, 100.0f )
		};
		
		auto const view {
			glm::lookAt(
				glm::vec3( -5,  3, -10 ), // world space camera position
				glm::vec3(  0,  0,   0 ), // look at the origin
				glm::vec3(  0, -1,   0 )  // head is up (remove minus?)
			)
		};
		
		auto const model {
			glm::mat4( 1.f )
		};
		
		auto const clip { // Vulkan clip space has inverted Y and half Z
			glm::mat4(
				1.0f,  0.0f,  0.0f,  0.0f,
				0.0f, -1.0f,  0.0f,  0.0f,
				0.0f,  0.0f,  0.5f,  0.0f,
				0.0f,  0.0f,  0.5f,  1.0f
			)
		};
		
		auto const mvp {
			clip * projection * view * model
		};
		
		vk::BufferCreateInfo const uniform_data_buffer_create_info {
			.size                  = sizeof( mvp ),
			.usage                 = vk::BufferUsageFlagBits::eUniformBuffer,
			.sharingMode           = vk::SharingMode::eExclusive,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices   = nullptr
			// NOTE: might need .flags
		};
		
		vk::raii::Buffer uniform_data_buffer( device, uniform_data_buffer_create_info );
		
		vk::MemoryRequirements const uniform_data_buffer_memory_requirements {
			uniform_data_buffer.getMemoryRequirements()
		};
		
		auto const uniform_data_buffer_memory_type_index {
			find_memory_type_index(
				physical_device_memory_properties,
				uniform_data_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eHostVisible bitor vk::MemoryPropertyFlagBits::eHostCoherent
			)
		};
		
		vk::MemoryAllocateInfo const uniform_data_buffer_memory_allocate_info {
			.allocationSize  = uniform_data_buffer_memory_requirements.size,
			.memoryTypeIndex = uniform_data_buffer_memory_type_index
		};
		
		vk::raii::DeviceMemory uniform_data_buffer_device_memory(
			device,
			uniform_data_buffer_memory_allocate_info
		);
		
		{ // TODO: verify block
			spdlog::info( "Mapping the uniform data buffer's device memory to the host device's memory so that the MVP can be uploaded..." );
			auto *p_mapped_memory {
				uniform_data_buffer_device_memory.mapMemory( 0, uniform_data_buffer_memory_requirements.size )
			};
			std::memcpy( p_mapped_memory, &mvp, sizeof(mvp) );
			uniform_data_buffer_device_memory.unmapMemory();
		}
		
	// Descriptor set binding & layout for uniform buffer:
		spdlog::info( "Creating descriptor set layout binding for the uniform data buffer..." );
		
		vk::DescriptorSetLayoutBinding const uniform_data_buffer_descriptor_set_layout_binding {
			.binding            = 0, // only making one set; index zero (TODO: read up on)
			.descriptorType     = vk::DescriptorType::eUniformBuffer,
			.descriptorCount    = 1, // only one descriptor in the set
			.stageFlags         = vk::ShaderStageFlagBits::eVertex,
			.pImmutableSamplers = nullptr // NOTE: ignore until later:w
		};
		
		spdlog::info( "Creating descriptor set layout for the uniform data buffer..." );
		
		vk::DescriptorSetLayoutCreateInfo const uniform_data_buffer_descriptor_set_layout_create_info {
			.bindingCount    = 1, // only one descriptor set (TODO: verify wording)
			.pBindings       = &uniform_data_buffer_descriptor_set_layout_binding
		};
		
		vk::raii::DescriptorSetLayout uniform_data_buffer_descriptor_set_layout(
			device,
			uniform_data_buffer_descriptor_set_layout_create_info
		);
		
	// Pipeline layout:
		spdlog::info( "Creating pipeline layout..." );
		
		vk::PipelineLayoutCreateInfo const pipeline_layout_create_info {
			.setLayoutCount         = 1,
			.pSetLayouts            = &*uniform_data_buffer_descriptor_set_layout, // TODO: verify &*
			.pushConstantRangeCount = 0,      // TODO: explain purpose
			.pPushConstantRanges    = nullptr //       ditto
		};
		
		vk::raii::PipelineLayout pipeline_layout( device, pipeline_layout_create_info ); 
		
		// NOTE (shader usage): layout (std140, binding = 0) uniform bufferVals {
		// NOTE (shader usage):     mat4 mvp;
		// NOTE (shader usage): } myBufferVals;
		
	// Descriptor pool:
		spdlog::info( "Creating descriptor pool..." );
		
		// TODO: std::array?
		vk::DescriptorPoolSize const descriptor_pool_sizes[1] {
			{
				.type            = vk::DescriptorType::eUniformBuffer,
				.descriptorCount = 1
			}
		};
		
		vk::DescriptorPoolCreateInfo const descriptor_pool_create_info {
			.maxSets       = 1, // TODO: comment properly
			.poolSizeCount = 1, // TODO: comment properly
			.pPoolSizes    = descriptor_pool_sizes
		};
		
		vk::raii::DescriptorPool descriptor_pool( device, descriptor_pool_create_info );
		
	// Descriptor set(s):
		spdlog::info( "Allocating descriptor set(s)..." );	
		
		vk::DescriptorSetAllocateInfo const descriptor_set_allocate_info {
			.descriptorPool     =  *descriptor_pool,
			.descriptorSetCount =   swapchain_framebuffer_count,
			.pSetLayouts        = &*uniform_data_buffer_descriptor_set_layout // TODO: array
		};
		
		auto descriptor_sets {
			device.allocateDescriptorSets( descriptor_set_allocate_info )
		};
		
		for ( u32 i{0}; i<swapchain_framebuffer_count; ++i ) {
			vk::DescriptorBufferInfo const descriptor_buffer_info {
				.buffer = uniform_data_buffer[i],
				.offset = 0,
				.range  = sizeof( glm::mat4 )
			};
		}

		// TODO: make recreate_swapchain function
		// TODO: make update_uniform_buffer function
		
///////////////////////////////////////////////////////////////////////////////////////
		// the big TODO
		
		// Swapchain
		// Image View
		// Depth Buffer
		// Uniform Buffer
		// Pipeline Layout
		// Descriptor Set
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
		while ( not glfwWindowShouldClose(p_window) ) {
			// TODO: handle input, update logic, render, draw window
			glfwSwapBuffers( p_window );
			glfwPollEvents();
		}
		spdlog::info( "Exiting MyTemplate..." );
		spdlog::info( "Destroying window..." );
		glfwDestroyWindow( p_window );
		spdlog::info( "Terminating GLFW..." );
		glfwTerminate();
	}
	catch ( vk::SystemError const &e ) {
		spdlog::critical( "vk::SystemError: {}", e.what() );
		std::exit( EXIT_FAILURE );
	}
	catch ( std::exception const &e ) {
		spdlog::critical( "std::exception: {}", e.what() );
		std::exit( EXIT_FAILURE );
	}
	catch (...) {
		spdlog::critical( "Unknown error encountered!" );
		std::exit( EXIT_FAILURE );
	}
	// exiting:
	std::exit( EXIT_SUCCESS );
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
