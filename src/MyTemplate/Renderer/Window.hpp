#pragma once // potentially faster compile-times if supported
#ifndef WINDOW_HPP_ZGKG5SWR
#define WINDOW_HPP_ZGKG5SWR

#include <memory>

// forward declarations:
class GLFWwindow;
namespace vk::raii {
	class Instance;
	class SurfaceKHR;
}

namespace gfx {
	class Window final {
		public:
			struct Dimensions final { int width, height; }; // TODO: refactor later
			Window( GlfwInstance const &, vk::raii::Instance const &, bool &setOnResize );
			Window(                ) = delete;
			Window( Window const & ) = delete;
			Window( Window &&      ) noexcept;
			~Window()                noexcept;
			// TODO: assignment operators?
			[[nodiscard]] Dimensions                   getDimensions() const;
			[[nodiscard]] vk::raii::SurfaceKHR const & getSurface()    const;
			[[nodiscard]] vk::raii::SurfaceKHR       & getSurface()         ;
			[[nodiscard]] bool                         wasClosed()     const;
			void                                       update()             ;
			void                                       waitResize()         ;
		private:
			static void onResizeCallback( GLFWwindow *, [[maybe_unused]] int width, [[maybe_unused]] int height );
			
			GLFWwindow                            *mpWindow;
			std::unique_ptr<vk::raii::SurfaceKHR>  mpSurface;
			bool                                  *mpSetOnResize;
	}; // end-of-class: Window
} // end-of-namespace: gfx

#endif // end-of-header-guard WINDOW_HPP_ZGKG5SWR
// EOF
