#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include "MyTemplate/Common/aliases.hpp"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

GlfwInstance::GlfwInstance()
{
	spdlog::info( "Initializing GLFW..." );
	
	if ( not glfwInit() )
		throw std::runtime_error { "Unable to initialize GLFW!" };
	
	else if ( not glfwVulkanSupported() )
		throw std::runtime_error { "Vulkan is unavailable!" };
}

GlfwInstance::~GlfwInstance() noexcept
{
	spdlog::info( "Terminating GLFW..." );
	glfwTerminate();
}

std::span<char const *>
GlfwInstance::get_required_extensions() const
{
	u32  required_extensions_count;
	char const **required_extensions_list {
		glfwGetRequiredInstanceExtensions( &required_extensions_count )
	};
	
	if ( required_extensions_list == nullptr )
		throw std::runtime_error { "Failed to get required Vulkan instance extensions!" };
	
	return { required_extensions_list, required_extensions_count };
}

// EOF
