#pragma once // potentially faster compile-times if supported
#ifndef WINDOW_HPP_ZGKG5SWR
#define WINDOW_HPP_ZGKG5SWR

#include "MyTemplate/Renderer/common.hpp"
#include <memory>

namespace gfx {
	class Window final {
		public:
			struct Dimensions final { int width, height; }; // TODO: refactor later
			Window( GlfwInstance const &, VkInstance const & );
			Window(                ) = delete;
			Window( Window const & ) = delete;
			Window( Window &&      ) noexcept;
			~Window()                noexcept;
			// TODO: assignment operators?
			[[nodiscard]] Dimensions                   get_dimensions() const;
			[[nodiscard]] vk::raii::SurfaceKHR const & get_surface()    const;
			[[nodiscard]] vk::raii::SurfaceKHR       & get_surface()         ;
			[[nodiscard]] bool                         was_closed()     const;
			void                                       update()              ;
		private:
			GLFWwindow                            *m_p_window;
			std::unique_ptr<vk::raii::SurfaceKHR>  m_p_surface;
	}; // end-of-class: Window
} // end-of-namespace: gfx

#endif // end-of-header-guard WINDOW_HPP_ZGKG5SWR
// EOF
