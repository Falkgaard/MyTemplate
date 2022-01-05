#include "MyTemplate/Renderer/GlfwInstance.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace gfx {
	namespace { // private (file-scope)
		u32 fg_glfw_user_count { 0 }; // TODO: make atomic!
	} // end-of-unnamed-namespace
	
	GlfwInstance::GlfwInstance()
	{
		spdlog::info( "Constructing a GlfwInstance instance..." );
		if ( fg_glfw_user_count == 0 ) [[likely]] {
			spdlog::info( "... initializing GLFW" );
			
			if ( not glfwInit() ) [[unlikely]]
				throw std::runtime_error { "Unable to initialize GLFW!" };
			else if ( not glfwVulkanSupported() ) [[unlikely]]
				throw std::runtime_error { "Vulkan is unavailable!" };
		}
		++fg_glfw_user_count;
		spdlog::info( "... done!" );
	} // end-of-function: gfx::GlfwInstance::GlfwInstance
	
	GlfwInstance::GlfwInstance( [[maybe_unused]] GlfwInstance const & )
	{
		spdlog::info( "Copying a GlfwInstance instance..." );
		++fg_glfw_user_count;
	} // end-of-function: gfx::GlfwInstance::GlfwInstance
	
	GlfwInstance::GlfwInstance( [[maybe_unused]] GlfwInstance && ) noexcept
	{
		spdlog::info( "Moving a GlfwInstance instance..." );
	} // end-of-function: gfx::GlfwInstance::GlfwInstance
	
	GlfwInstance::~GlfwInstance() noexcept
	{
		spdlog::info( "Destroying a GlfwInstance instance..." );
		--fg_glfw_user_count;
		if ( fg_glfw_user_count == 0 ) [[likely]] {
			spdlog::info( "... terminating GLFW" );
			glfwTerminate();
		}
	} // end-of-function: gfx::GlfwInstance::~GlfwInstance
	
	std::span<char const *>
	GlfwInstance::get_required_extensions() const
	{
		u32          required_extensions_count;
		char const **required_extensions_list {
			glfwGetRequiredInstanceExtensions( &required_extensions_count )
		};
		
		if ( required_extensions_list == nullptr ) [[unlikely]]
			throw std::runtime_error { "Failed to get required Vulkan instance extensions!" };
		else [[likely]] return { required_extensions_list, required_extensions_count };
	} // end-of-function: gfx::GlfwInstance::get_required_extensions
} // end-of-namespace: gfx
// EOF
