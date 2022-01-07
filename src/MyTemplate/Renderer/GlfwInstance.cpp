#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include "MyTemplate/Common/aliases.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace gfx {
	namespace { // private (file-scope)
		u32 gGlfwUserCount { 0 }; // TODO: make atomic!
	} // end-of-unnamed-namespace
	
	GlfwInstance::GlfwInstance()
	{
		spdlog::info( "Constructing a GlfwInstance instance..." );
		if ( gGlfwUserCount == 0 ) [[likely]] {
			spdlog::info( "... initializing GLFW" );
			
			if ( not glfwInit() ) [[unlikely]]
				throw std::runtime_error { "Unable to initialize GLFW!" };
			else if ( not glfwVulkanSupported() ) [[unlikely]]
				throw std::runtime_error { "Vulkan is unavailable!" };
		}
		++gGlfwUserCount;
		spdlog::info( "... done!" );
	} // end-of-function: GlfwInstance::GlfwInstance
	
	GlfwInstance::GlfwInstance( [[maybe_unused]] GlfwInstance const & )
	{
		spdlog::info( "Copying a GlfwInstance instance..." );
		++gGlfwUserCount;
	} // end-of-function: GlfwInstance::GlfwInstance
	
	GlfwInstance::~GlfwInstance() noexcept
	{
		spdlog::info( "Destroying a GlfwInstance instance..." );
		--gGlfwUserCount;
		if ( gGlfwUserCount == 0 ) [[likely]] {
			spdlog::info( "... terminating GLFW" );
			glfwTerminate();
		}
	} // end-of-function: GlfwInstance::~GlfwInstance
	
	std::span<char const *>
	GlfwInstance::getRequiredExtensions() const
	{
		u32          requiredExtensionsCount;
		char const **requiredExtensionsList {
			glfwGetRequiredInstanceExtensions( &requiredExtensionsCount )
		};
		
		if ( requiredExtensionsList == nullptr ) [[unlikely]]
			throw std::runtime_error { "Failed to get required Vulkan instance extensions!" };
		else [[likely]]
			return { requiredExtensionsList, requiredExtensionsCount };
	} // end-of-function: GlfwInstance::getRequiredExtensions
} // end-of-namespace: gfx
// EOF
