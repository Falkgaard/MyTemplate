#include "MyTemplate/Renderer/Window.hpp"

#include "MyTemplate/Renderer/VkInstance.hpp"
#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <utility>
#include <stdexcept>

// TODO: wrap mpWindow with a unique_ptr instead

namespace gfx {
	// NOTE: the GlfwInstance reference is just to ensure that GLFW is initialized
	Window::Window(
		[[maybe_unused]] GlfwInstance const &,
		vk::raii::Instance            const &vkInstance,
		bool                                &setOnResize
	):
		mpSetOnResize { &setOnResize }
	{
		spdlog::info( "Constructing a Window instance..." );
		
		glfwDefaultWindowHints();
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		
		spdlog::info( "... creating GLFW window");
		// TODO: refactor out construction args
		mpWindow = glfwCreateWindow( 640, 480, "MyTemplate", nullptr, nullptr );
		VkSurfaceKHR surfaceTemp;
		
		auto const result {
			glfwCreateWindowSurface(
				*vkInstance.get_instance(),
				 mpWindow,
				 nullptr,
				&surfaceTemp
			)
		};
		
		if ( result != VkResult::VK_SUCCESS ) [[unlikely]]
			throw std::runtime_error { "Unable to create GLFW window surface!" };
		
		mpSurface = std::make_unique<vk::raii::SurfaceKHR>( vkInstance, surfaceTemp );
		
		// setup callback(s):
		glfwSetWindowUserPointer(  mpWindow, this );
		glfwSetWindowSizeCallback( mpWindow, onResizeCallback );
	} // end-of-function: Window::Window
	
	Window::Window( Window &&other ) noexcept:
		mpSurface       ( std::move(     other.mpSurface              ) ),
		mpWindow        ( std::exchange( other.mpWindow,      nullptr ) ), // TODO: verify
		mpSet_on_resize ( std::exchange( other.mpSetOnResize, nullptr ) )  // TODO: verify
	{
		spdlog::info( "Moving a Window instance..." );
	} // end-of-function: Window::Window
	
	Window::~Window() noexcept
	{
		spdlog::info( "Destroying a Window instance..." );
		// NOTE: Vulkan-Hpp should take care of cleaning up the surface for us
		if ( mpWindow ) [[likely]] {
			spdlog::info( "... destroying GLFW window" );
			glfwDestroyWindow( mpWindow );
		}
	} // end-of-function: Window::~Window
	
	[[nodiscard]] Window::Dimensions
	Window::getDimensions() const
	{
		spdlog::debug( "Accessing window dimensions..." );
		Dimensions dimensions;
		glfwGetWindowSize( mpWindow, &dimensions.width, &dimensions.height );
		return dimensions;
	} // end-of-function: Window::getDimensions
	
	[[nodiscard]] vk::raii::SurfaceKHR const &
	Window::getSurface() const
	{
		spdlog::debug( "Accessing window surface..." );
		return *mpSurface;
	} // end-of-function: Window::getSurface
	
	[[nodiscard]] vk::raii::SurfaceKHR &
	Window::getSurface()
	{
		spdlog::debug( "Accessing window surface..." );
		return *mpSurface;
	} // end-of-function: Window::getSurface
	
	[[nodiscard]] bool
	Window::wasClosed() const
	{
		return glfwWindowShouldClose( mpWindow );
	} // end-of-function: Window::wasClosed

	void
	Window::update()
	{
		glfwSwapBuffers( mpWindow );
		glfwPollEvents(); // TODO: expose usage somehow
	} // end-of-function: Window::update
	
	void
	Window::waitResize()
	{
		i32 width  { 0 };
		i32 height { 0 };
		glfwGetFramebufferSize( mpWindow, &width, &height );
		while ( width == 0 or height == 0 ) {
		    glfwGetFramebufferSize( mpWindow, &width, &height );
		    glfwWaitEvents();
		}
	} // end-of-function: Window::waitResize
	
	void
	Window::onResizeCallback( GLFWwindow *pWindow, [[maybe_unused]] int width, [[maybe_unused]] int height )
	{
		*(static_cast<Window*>( glfwGetWindowUserPointer(pWindow) )->mpSetOnResize) = true;
	} // end-of-function: Window::onResizeCallback
} // end-of-namespace: gfx
// EOF
