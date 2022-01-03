#include "MyTemplate/Renderer/Renderer.hpp"
#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include "MyTemplate/Renderer/Window.hpp"
#include "MyTemplate/Renderer/Swapchain.hpp"
#include "MyTemplate/Common/utility.hpp"
#include "MyTemplate/info.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <ranges>
#include <algorithm>

namespace // private (file-scope)
{
	std::array const required_validation_layers {
		"VK_LAYER_KHRONOS_validation"
	};
	
	// TODO: refactor common code shared by enableValidationLayers and enableInstanceExtensions.
	
	void
	enable_validation_layers(
		vk::raii::Context      &vk_context,
		vk::InstanceCreateInfo &vk_instance_create_info
	)
	{
		if constexpr ( g_is_debug_mode ) { // logic is only required for debug builds
			spdlog::info( "Enabling validation layers..." );
			auto const available_validation_layers { vk_context.enumerateInstanceLayerProperties() };
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
				vk_instance_create_info.enabledLayerCount   = static_cast<u32>( required_validation_layers.size() );
				vk_instance_create_info.ppEnabledLayerNames = required_validation_layers.data();
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
	enable_instance_extensions(
		GlfwInstance           &glfw_instance,
		vk::raii::Context      &vk_context,
		vk::InstanceCreateInfo &vk_instance_create_info
	)
	{
		spdlog::info( "Enabling instance extensions..." );
		auto const available_instance_extensions {
			vk_context.enumerateInstanceExtensionProperties()
		};
		
		// get required GLFW instance extensions:
		auto const glfw_required_extensions {
			glfw_instance.get_required_extensions()
		};
		// add required GLFW instance extensions:
		required_instance_extensions.reserve( std::size(glfw_required_extensions) );
		for ( i32 i=0; i<std::size(glfw_required_extensions); ++i )
			required_instance_extensions.push_back( glfw_required_extensions[i] );
		
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
			vk_instance_create_info.enabledExtensionCount   = static_cast<u32>( required_instance_extensions.size() );
			vk_instance_create_info.ppEnabledExtensionNames = required_instance_extensions.data();
		}
		else throw std::runtime_error { "Failed to load required instance extensions!" };
	} // end-of-function: enable_instance_extensions
	
	std::array constexpr required_device_extensions {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	
	[[nodiscard]] bool
	is_supporting_extensions( vk::raii::PhysicalDevice const &physical_device )
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
	} // end-of-function: is_supporting_extensions
	
	[[nodiscard]] u32
	score_physical_device( vk::raii::PhysicalDevice const &physical_device )
	{
		auto const properties { physical_device.getProperties() }; // TODO: KHR2?
		auto const features   { physical_device.getFeatures()   }; // TODO: 2KHR?
		spdlog::info( "Scoring physical device `{}`...", properties.deviceName.data() );
		u32 score { 0 };
		
		if ( not is_supporting_extensions( physical_device ) )
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
	
	[[nodiscard]] auto
	select_physical_device( vk::raii::Instance &vk_instance )
	{
		spdlog::info( "Selecting the most suitable physical device..." );
		vk::raii::PhysicalDevices physical_devices( vk_instance );
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
			return std::make_unique<vk::raii::PhysicalDevice>( std::move( *p_best_match ) );
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
	#endif	
	
	[[nodiscard]] auto
	make_vk_context()
	{
		spdlog::info( "Making Vulkan context..." );
		return std::make_unique<vk::raii::Context>();
	} // end-of-function: make_vk_context
	
	[[nodiscard]] auto
	make_glfw_instance()
	{
		spdlog::info( "Making GLFW instance..." );
		return std::make_unique<GlfwInstance>();
	} // end-of-function: make_glfw_instance
	
	[[nodiscard]] auto
	make_vk_instance( GlfwInstance &glfw_instance, vk::raii::Context &vk_context )
	{
		spdlog::info( "Making Vulkan instance..." );
		
		// TODO: split engine/app versions and make customization point
		// TODO: move into some ApplicationInfo class created in main?
		auto const version {
			VK_MAKE_VERSION(
				MYTEMPLATE_VERSION_MAJOR,
				MYTEMPLATE_VERSION_MINOR,
				MYTEMPLATE_VERSION_PATCH
			)
		};
		vk::ApplicationInfo const app_info {
			.pApplicationName   = "MyTemplate App", // TODO: make customization point 
			.applicationVersion =  version,         // TODO: make customization point 
			.pEngineName        = "MyTemplate Engine",
			.engineVersion      =  version,
			.apiVersion         =  VK_API_VERSION_1_1
		};
		
		vk::InstanceCreateInfo vk_instance_create_info {
			.pApplicationInfo = &app_info
		};
		
		enable_validation_layers(                    vk_context, vk_instance_create_info );
		enable_instance_extensions(   glfw_instance, vk_context, vk_instance_create_info );
		return std::make_unique<vk::raii::Instance>( vk_context, vk_instance_create_info );
	} // end-of-function: make_vk_instance
	
	#if !defined( NDEBUG )
		[[nodiscard]] auto
		make_debug_messenger( vk::raii::Context &vk_context, vk::raii::Instance &vk_instance )
		{
			spdlog::info( "Making debug messenger..." );
			
			auto const extension_properties {
				vk_context.enumerateInstanceExtensionProperties()
			};
			
			// look for debug utils extension:
			auto const search_result {
				std::find_if(
					std::begin( extension_properties ),
					std::end(   extension_properties ),
					[]( auto const &extension_properties ) {
						return std::strcmp( extension_properties.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0;
					}
				)
			};
			if ( search_result == std::end( extension_properties ) )
				throw std::runtime_error { "Could not find " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " extension!" };
			
			vk::DebugUtilsMessageSeverityFlagsEXT const severity_flags {
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
			};
			
			vk::DebugUtilsMessageTypeFlagsEXT const type_flags {
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral     |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
			};
			
			vk::DebugUtilsMessengerCreateInfoEXT const create_info {
				.messageSeverity =  severity_flags,
				.messageType     =  type_flags,
				.pfnUserCallback = &debug_callback
			};
			
			return std::make_unique<vk::raii::DebugUtilsMessengerEXT>( vk_instance, create_info );
		} // end-of-function: make_debug_messenger
	#endif
	
	[[nodiscard]] auto
	make_window( GlfwInstance &glfw_instance, vk::raii::Instance &vk_instance )
	{
		spdlog::info( "Making window..." );
		return std::make_unique<Window>( glfw_instance, vk_instance );
	} // end-of-function: make_window
	
	[[nodiscard]] auto
	select_queue_family_indices( vk::raii::PhysicalDevice &physical_device, Window &window )
	{
		std::optional<u32> maybe_present_index  {};
		std::optional<u32> maybe_graphics_index {};
		auto const &queue_family_properties {
			physical_device.getQueueFamilyProperties()
		};
		spdlog::info( "Searching for graphics queue family..." );
		for ( u32 index{0}; index<std::size(queue_family_properties); ++index ) {
			bool const supports_graphics {
				queue_family_properties[index].queueFlags & vk::QueueFlagBits::eGraphics
			};
			bool const supports_present {
				physical_device.getSurfaceSupportKHR( index, *window.get_surface() ) == VK_TRUE
			};
			
			if ( supports_graphics and supports_present ) {
				spdlog::info( "Found queue family that supports both graphics and present!" );
				maybe_present_index  = index;
				maybe_graphics_index = index;
				break;
			}
			else if ( supports_graphics ) {
				spdlog::info( "Found queue family that supports graphics!" );
				maybe_graphics_index = index;
			}
			else if ( supports_present ) {
				spdlog::info( "Found queue family that supports present!" );
				maybe_present_index = index;
			}
		}
		if ( maybe_present_index.has_value() and maybe_graphics_index.has_value() ) {
			bool const are_separate {
				maybe_present_index.value() != maybe_graphics_index.value()
			};
			if ( are_separate )
				spdlog::info( "Selected different queue families for graphics and present." );
			return QueueFamilyIndices {
				.present      = maybe_present_index.value(),
				.graphics     = maybe_graphics_index.value(),
				.are_separate = are_separate
			};
		}
		else throw std::runtime_error { "Queue family support for either graphics or present missing!" };
	} // end-of-function: select_queue_family_indices
	
	[[nodiscard]] auto
	make_logical_device(
		vk::raii::PhysicalDevice &physical_device,
		QueueFamilyIndices const &queue_family_indices
	)
	{
		spdlog::info( "Making logical device..." );
		
		// TODO: refactor approach when more queues are needed
		std::vector<vk::DeviceQueueCreateInfo> queue_create_infos {};
		
		f32 const present_queue_priority  { 1.0f }; // TODO: verify value
		f32 const graphics_queue_priority { 1.0f }; // TODO: verify value
		
		queue_create_infos.push_back(
			vk::DeviceQueueCreateInfo {
				.queueFamilyIndex =  queue_family_indices.present,
				.queueCount       =  1,
				.pQueuePriorities = &present_queue_priority
			}
		);
		
		if ( queue_family_indices.are_separate ) {
			queue_create_infos.push_back(
				vk::DeviceQueueCreateInfo {
					.queueFamilyIndex =  queue_family_indices.graphics,
					.queueCount       =  1,
					.pQueuePriorities = &graphics_queue_priority
				}
			);
		}
		
		vk::DeviceCreateInfo const create_info {
			.queueCreateInfoCount    = static_cast<u32>( queue_create_infos.size() ),
			.pQueueCreateInfos       = queue_create_infos.data(),
			.enabledLayerCount       = 0,       // no longer used; TODO: add conditionally to support older versions?
			.ppEnabledLayerNames     = nullptr, // ^ ditto
			.enabledExtensionCount   = static_cast<u32>( required_device_extensions.size() ),
			.ppEnabledExtensionNames = required_device_extensions.data()
		};
		
		return std::make_unique<vk::raii::Device>( physical_device, create_info );
	} // end-of-function: make_logical_device
	
	[[nodiscard]] auto
	make_queue( vk::raii::Device &logical_device, u32 const queue_family_index, u32 const queue_index )
	{ // TODO: refactor away third arg and use automated tracking + exceptions
		spdlog::info( "Making handle for queue #{} of queue family #{}...", queue_index, queue_family_index );
		auto queue {
			logical_device.getQueue( queue_family_index, queue_index )
		};
		return std::make_unique<vk::raii::Queue>( std::move(queue) );
	} // end-of-function: make_queue
	
	[[nodiscard]] auto
	make_command_pool( vk::raii::Device &logical_device, u32 const graphics_queue_family_index )
	{
		spdlog::info( "Making command buffer pool..." );
		vk::CommandPoolCreateInfo const create_info {
			// NOTE: Flags can be set here to optimize for lifetime or enable resetability.
			//       Also, one pool would be needed for each queue family (if ever extended).
			.queueFamilyIndex = graphics_queue_family_index
		};
		return std::make_unique<vk::raii::CommandPool>( logical_device, create_info );
	} // end-of-function: make_command_pool
	
	[[nodiscard]] auto
	make_command_buffers(
		vk::raii::Device       &logical_device,
		vk::raii::CommandPool  &command_pool,
		vk::CommandBufferLevel  level,
		u32 const               command_buffer_count
	)
	{
		spdlog::info( "Making {} command buffer(s)...", command_buffer_count );
			vk::CommandBufferAllocateInfo const allocate_info {
			.commandPool        = *command_pool,
			.level              =  level,
			.commandBufferCount =  command_buffer_count
		};
		return std::make_unique<vk::raii::CommandBuffers>( logical_device, allocate_info );
	} // end-of-function: make_command_buffers
	
	[[nodiscard]] auto
	make_swapchain(
		vk::raii::PhysicalDevice &physical_device,
		vk::raii::Device         &logical_device,
		Window                   &window,
		QueueFamilyIndices       &queue_family_indices
	)
	{
		spdlog::info( "Making swapchain..." );
		return std::make_unique<Swapchain>( physical_device, logical_device, window, queue_family_indices );
	} // end-of-function: make_swapchain
	
} // end-of-unnamed-namespace

Renderer::Renderer()
{
	spdlog::info( "Creating Renderer instance..." );
	m_p_glfw_instance      = make_glfw_instance();
	m_p_vk_context         = make_vk_context();
	m_p_vk_instance        = make_vk_instance( *m_p_glfw_instance, *m_p_vk_context );
	#if !defined( NDEBUG )
		m_p_debug_messenger = make_debug_messenger( *m_p_vk_context, *m_p_vk_instance );
	#endif
	m_p_window             = make_window( *m_p_glfw_instance, *m_p_vk_instance );
	m_p_physical_device    = select_physical_device( *m_p_vk_instance );
	m_queue_family_indices = select_queue_family_indices( *m_p_physical_device, *m_p_window );
	m_p_logical_device     = make_logical_device( *m_p_physical_device, m_queue_family_indices );
	m_p_graphics_queue     = make_queue( *m_p_logical_device, m_queue_family_indices.graphics, 0 ); // NOTE: 0 since we
	m_p_present_queue      = make_queue( *m_p_logical_device, m_queue_family_indices.present,  0 ); // only use 1 queue
	m_p_command_pool       = make_command_pool( *m_p_logical_device, m_queue_family_indices.graphics );
	m_p_command_buffers    = make_command_buffers( *m_p_logical_device, *m_p_command_pool, vk::CommandBufferLevel::ePrimary, 1 );
	m_p_swapchain          = make_swapchain( *m_p_physical_device, *m_p_logical_device, *m_p_window, m_queue_family_indices );
}

Renderer::Renderer( Renderer &&other ) noexcept:
	m_p_glfw_instance      { std::exchange( other.m_p_glfw_instance,      nullptr ) },
	m_p_vk_context         { std::exchange( other.m_p_vk_context,         nullptr ) },
	m_p_vk_instance        { std::exchange( other.m_p_vk_instance,        nullptr ) },
	#if !defined( NDEBUG )                   
	m_p_debug_messenger    { std::exchange( other.m_p_debug_messenger,    nullptr ) },
	#endif
	m_p_window             { std::exchange( other.m_p_window,             nullptr ) },
	m_p_physical_device    { std::exchange( other.m_p_physical_device,    nullptr ) },
	m_queue_family_indices { other.m_queue_family_indices                           },
	m_p_logical_device     { std::exchange( other.m_p_logical_device,     nullptr ) },
	m_p_graphics_queue     { std::exchange( other.m_p_graphics_queue,     nullptr ) },
	m_p_present_queue      { std::exchange( other.m_p_present_queue,      nullptr ) },
	m_p_command_pool       { std::exchange( other.m_p_command_pool,       nullptr ) },
	m_p_command_buffers    { std::exchange( other.m_p_command_buffers,    nullptr ) },
	m_p_swapchain          { std::exchange( other.m_p_swapchain,          nullptr ) }
{
	spdlog::info( "Moving Renderer instance..." );
}

Renderer::~Renderer() noexcept
{
	spdlog::info( "Destroying Renderer instance..." );
}

[[nodiscard]] Window &
Renderer::get_window()
{
	return *m_p_window;
}


// EOF
