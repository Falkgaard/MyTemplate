#pragma once // potentially faster compile-times if supported
#ifndef WINDOW_HPP_ZGKG5SWR
#define WINDOW_HPP_ZGKG5SWR

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include <memory>

class GlfwInstance; // forward declaration

class Window final
{
public:
	struct Dimensions final { int width, height; };
	Window( GlfwInstance const &, vk::raii::Instance const &instance );
	Window(                ) = delete;	
	Window( Window const & ) = delete;
	Window( Window &&      ) noexcept;
	~Window() noexcept;
	// TODO: assignment operators?
	[[nodiscard]] Dimensions                   get_dimensions() const;
	[[nodiscard]] vk::raii::SurfaceKHR const & get_surface() const;
	[[nodiscard]] vk::raii::SurfaceKHR       & get_surface();
	[[nodiscard]] bool                         was_closed() const;
	void                                       update();
private:
	GLFWwindow                            *m_p_window;
	std::unique_ptr<vk::raii::SurfaceKHR>  m_p_surface;
};

#endif // end-of-header-guard WINDOW_HPP_ZGKG5SWR
// EOF
