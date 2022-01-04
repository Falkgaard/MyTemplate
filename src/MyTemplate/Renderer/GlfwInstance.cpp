#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include "MyTemplate/Common/aliases.hpp"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

GlfwInstance::GlfwInstance()
{
	spdlog::info( "Constructing a GlfwInstance instance..." );
	if ( sm_glfw_user_count == 0 ) [[likely]] {
		spdlog::info( "... initializing GLFW" );
		
		if ( not glfwInit() ) [[unlikely]]
			throw std::runtime_error { "Unable to initialize GLFW!" };
		
		else if ( not glfwVulkanSupported() ) [[unlikely]]
			throw std::runtime_error { "Vulkan is unavailable!" };
	}
	++sm_glfw_user_count;
}

GlfwInstance::GlfwInstance( [[maybe_unused]] GlfwInstance const & )
{
	spdlog::info( "Copying a GlfwInstance instance..." );
	++sm_glfw_user_count;
}

GlfwInstance::GlfwInstance( [[maybe_unused]] GlfwInstance && ) noexcept
{
	spdlog::info( "Moving a GlfwInstance instance..." );
}

GlfwInstance::~GlfwInstance() noexcept
{
	spdlog::info( "Destroying a GlfwInstance instance..." );
	--sm_glfw_user_count;
	if ( sm_glfw_user_count == 0 ) [[likely]] {
		spdlog::info( "... terminating GLFW" );
		glfwTerminate();
	}
}

std::span<char const *>
GlfwInstance::get_required_extensions() const
{
	u32          required_extensions_count;
	char const **required_extensions_list {
		glfwGetRequiredInstanceExtensions( &required_extensions_count )
	};
	
	if ( required_extensions_list == nullptr ) [[unlikely]]
		throw std::runtime_error { "Failed to get required Vulkan instance extensions!" };
	
	return { required_extensions_list, required_extensions_count };
}

// EOF
