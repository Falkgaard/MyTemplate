#include "MyTemplate/Renderer/Window.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <utility>
#include <stdexcept>

Window::Window( [[maybe_unused]] GlfwInstance &, vk::raii::Instance &instance )
{
	glfwDefaultWindowHints();
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint( GLFW_RESIZABLE,  GLFW_FALSE  ); // TODO: remove later
	
	spdlog::info( "Creating GLFW window... ");
	m_p_window = glfwCreateWindow( 640, 480, "MyTemplate", nullptr, nullptr );
	VkSurfaceKHR surface_tmp;
	
	auto result = glfwCreateWindowSurface(
		*instance,
		 m_p_window,
		 nullptr,
		&surface_tmp
	);
	
	if ( result != VkResult::VK_SUCCESS )
		throw std::runtime_error { "Unable to create GLFW window surface!" };
	
	m_p_surface = std::make_unique<vk::raii::SurfaceKHR>( instance, surface_tmp );
}

Window::~Window() noexcept
{
	spdlog::info( "Destroying window..." );
	glfwDestroyWindow( m_p_window );
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

// EOF
