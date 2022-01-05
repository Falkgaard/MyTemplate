#include "MyTemplate/Renderer/Window.hpp"

#include "MyTemplate/Renderer/VkInstance.hpp"
#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <utility>
#include <stdexcept>

namespace gfx {
	// NOTE: the GlfwInstance reference is just to ensure that GLFW is initialized
	Window::Window( [[maybe_unused]] GlfwInstance const &, VkInstance const &vk_instance )
	{
		spdlog::info( "Constructing a Window instance..." );
		
		glfwDefaultWindowHints();
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		glfwWindowHint( GLFW_RESIZABLE,  GLFW_FALSE  ); // TODO: remove later
		
		spdlog::info( "... creating GLFW window");
		// TODO: refactor out construction args
		m_p_window = glfwCreateWindow( 640, 480, "MyTemplate", nullptr, nullptr );
		VkSurfaceKHR surface_tmp;
		
		auto const result {
			glfwCreateWindowSurface(
				*vk_instance.get_instance(),
				 m_p_window,
				 nullptr,
				&surface_tmp
			)
		};
		
		if ( result != VkResult::VK_SUCCESS ) [[unlikely]]
			throw std::runtime_error { "Unable to create GLFW window surface!" };
		
		m_p_surface = std::make_unique<vk::raii::SurfaceKHR>( vk_instance.get_instance(), surface_tmp );
	}

	Window::Window( Window &&other ) noexcept:
		m_p_surface ( std::exchange(other.m_p_surface, nullptr) ), // TODO: verify
		m_p_window  ( std::exchange(other.m_p_window,  nullptr) )  // TODO: verify
	{
		spdlog::info( "Moving a Window instance..." );
	}

	Window::~Window() noexcept
	{
		spdlog::info( "Destroying a Window instance..." );
		// NOTE: Vulkan-Hpp should take care of cleaning up the surface for us
		if ( m_p_window ) [[likely]] {
			spdlog::info( "... destroying GLFW window" );
			glfwDestroyWindow( m_p_window );
		}
	}

	[[nodiscard]] Window::Dimensions
	Window::get_dimensions() const
	{
		Dimensions dimensions;
		glfwGetWindowSize( m_p_window, &dimensions.width, &dimensions.height );
		return dimensions;
	}

	[[nodiscard]] vk::raii::SurfaceKHR const &
	Window::get_surface() const
	{
		return *m_p_surface;
	}

	[[nodiscard]] vk::raii::SurfaceKHR &
	Window::get_surface()
	{
		return *m_p_surface;
	}

	[[nodiscard]] bool
	Window::was_closed() const
	{
		return glfwWindowShouldClose( m_p_window );
	}

	void
	Window::update()
	{
		glfwSwapBuffers( m_p_window );
		glfwPollEvents(); // TODO: expose usage somehow
	}
} // end-of-namespace: gfx
// EOF
