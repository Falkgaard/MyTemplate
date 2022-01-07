#pragma once // potentially faster compile-times if supported
#ifndef GLFWINSTANCE_HPP_AU0HO3FK
#define GLFWINSTANCE_HPP_AU0HO3FK

#include <span>

namespace gfx {
	class GlfwInstance final {
		public:
			GlfwInstance();
			GlfwInstance( GlfwInstance const &  );
			GlfwInstance( GlfwInstance       && ) noexcept = default;
			~GlfwInstance() noexcept;
			// TODO: assignment ops?
			[[nodiscard]] std::span<char const *> getRequiredExtensions() const;
	}; // end-of-class: GlfwInstance
} // end-of-namespace: gfx

#endif // end-of-header-guard GLFWINSTANCE_HPP_AU0HO3FK
// EOF
