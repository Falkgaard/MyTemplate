#pragma once // potentially faster compile-times if supported
#ifndef GLFWINSTANCE_HPP_AU0HO3FK
#define GLFWINSTANCE_HPP_AU0HO3FK

#include <span>
#include "MyTemplate/Common/aliases.hpp"

class GlfwInstance final
{
public:
	GlfwInstance();
	GlfwInstance( GlfwInstance const &  );
	GlfwInstance( GlfwInstance       && ) noexcept;
	~GlfwInstance() noexcept;
	// TODO: assignment ops?
	[[nodiscard]] std::span<char const *> get_required_extensions() const;
private:
	inline static u32 sm_glfw_user_count { 0 }; // TODO: make atomic!
};

#endif // end-of-header-guard GLFWINSTANCE_HPP_AU0HO3FK
// EOF
