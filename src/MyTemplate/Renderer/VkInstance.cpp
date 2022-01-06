#include "MyTemplate/Renderer/VkInstance.hpp"

#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <spdlog/spdlog.h>

namespace gfx {
	namespace { // private (file-scope)
		
		// TODO: refactor out as customization point
		std::array const required_validation_layers {
			"VK_LAYER_KHRONOS_validation"
		};
		
		// TODO: refactor common code shared by enableValidationLayers and enableInstanceExtensions.
		
		void
		enable_validation_layers(
			vk::raii::Context const &vk_context,
			vk::InstanceCreateInfo  &vk_instance_create_info
		)
		{
			if constexpr ( g_is_debug_mode ) { // logic is only required for debug builds
				spdlog::info( "... enabling validation layers:" );
				auto const available_validation_layers { vk_context.enumerateInstanceLayerProperties() };
				// print required and available layers:
				for ( auto const &e : required_validation_layers )
					spdlog::info( "      required  : `{}`", e );
				for ( auto const &e : available_validation_layers )
					spdlog::info( "      available : `{}`", e.layerName );
				bool is_missing_layer { false };
				// ensure required layers are available:
				for ( auto const &target : required_validation_layers ) {
					bool match_found { false };
					for ( auto const &layer : available_validation_layers ) {
						if ( std::strcmp( target, layer.layerName ) == 0 ) [[unlikely]] {
							match_found = true;
							break;
						}
					}
					if ( not match_found ) [[unlikely]] {
						is_missing_layer = true;
						spdlog::error( "    ! MISSING   : `{}`!", target );
					}
				}
				
				// handle success or failure:
				if ( is_missing_layer ) [[unlikely]]
					throw std::runtime_error { "Failed to load required validation layers!" };
				else {
					vk_instance_create_info.enabledLayerCount   = static_cast<u32>( required_validation_layers.size() );
					vk_instance_create_info.ppEnabledLayerNames = required_validation_layers.data();
				}
			}
		} // end-of-function: enable_validation_layers
		
		// TODO: refactor out as customization point
		std::vector<char const *> required_instance_extensions {
			#if !defined( NDEBUG )
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME ,
				VK_EXT_DEBUG_REPORT_EXTENSION_NAME
			#endif
		};
		
		void
		enable_instance_extensions(
			GlfwInstance      const &glfw_instance,
			vk::raii::Context const &vk_context,
			vk::InstanceCreateInfo  &vk_instance_create_info
		)
		{
			spdlog::info( "... enabling instance extensions:" );
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
					spdlog::info( "      required  : `{}`", required_instance_extension );
				for ( auto const &available_instance_extension: available_instance_extensions )
					spdlog::info( "      available : `{}`", available_instance_extension.extensionName );
			}
			
			// ensure required instance extensions are available:
			bool is_adequate { true }; // assume true until proven otherwise
			for ( auto const &required_instance_extension: required_instance_extensions ) {
				bool is_supported { false }; // assume false until found
				for ( auto const &available_instance_extension: available_instance_extensions ) {
					if ( std::strcmp( required_instance_extension, available_instance_extension.extensionName ) == 0 ) [[unlikely]] {
						is_supported = true;
						break; // early exit
					}
				}
				if ( not is_supported ) [[unlikely]] {
					is_adequate = false;
					spdlog::error( "   ! MISSING   : `{}`", required_instance_extension );
				}
			}
			
			// handle success or failure:
			if ( is_adequate ) [[likely]] {
				// set create info fields:
				vk_instance_create_info.enabledExtensionCount   = static_cast<u32>( required_instance_extensions.size() );
				vk_instance_create_info.ppEnabledExtensionNames = required_instance_extensions.data();
			}
			else [[unlikely]] throw std::runtime_error { "Failed to load required instance extensions!" };
		} // end-of-function: enable_instance_extensions
		
	} // end-of-unnamed-namespace
	
	VkInstance::VkInstance( GlfwInstance const &glfw_instance )
	{
		spdlog::info( "Constructing a VkInstance..." );
		
		spdlog::info( "... creating a Vulkan context" );
		m_p_context = std::make_unique<vk::raii::Context>();
		
		spdlog::info( "... creating a Vulkan instance" );
		
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
		
		vk::InstanceCreateInfo create_info {
			.pApplicationInfo = &app_info
		};
		enable_validation_layers( *m_p_context, create_info );
		enable_instance_extensions( glfw_instance, *m_p_context, create_info );
		m_p_instance = std::make_unique<vk::raii::Instance>( *m_p_context, create_info );
		spdlog::info( "... done!" );
	} // end-of-function: VkInstance::VkInstance
	
	VkInstance::VkInstance( VkInstance &&other ) noexcept
	{
		spdlog::info( "Move constructing a VkInstance..." );
		m_p_context  = std::move( other.m_p_context  );
		m_p_instance = std::move( other.m_p_instance );
	}
	
	VkInstance::~VkInstance() noexcept
	{
		spdlog::info( "Destructing a VkInstance..." );
	}
	
	VkInstance &
	VkInstance::operator=( VkInstance &&other ) noexcept
	{
		spdlog::info( "Move assigning a VkInstance..." );
		m_p_context  = std::move( other.m_p_context  );
		m_p_instance = std::move( other.m_p_instance );
		return *this;
	}
	
	vk::raii::Context const &
	VkInstance::get_context() const
	{
		spdlog::debug( "Accessing Vulkan context..." );
		return *m_p_context;
	}
	
	vk::raii::Context &
	VkInstance::get_context()
	{
		spdlog::debug( "Accessing Vulkan context..." );
		return *m_p_context;
	}
	
	vk::raii::Instance const &
	VkInstance::get_instance() const
	{
		spdlog::debug( "Accessing Vulkan instance..." );
		return *m_p_instance;
	}
	
	vk::raii::Instance &
	VkInstance::get_instance() {
		spdlog::debug( "Accessing Vulkan instance..." );
		return *m_p_instance;
	}
	
} // end-of-namespace: gfx
// EOF
