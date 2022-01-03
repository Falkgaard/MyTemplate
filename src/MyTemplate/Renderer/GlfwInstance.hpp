#pragma once // potentially faster compile-times if supported
#ifndef GLFWINSTANCE_HPP_AU0HO3FK
#define GLFWINSTANCE_HPP_AU0HO3FK

#include <span>

class GlfwInstance final
{
public:
	GlfwInstance();
	~GlfwInstance() noexcept;
	std::span<char const *> get_required_extensions() const;
};

#endif // end-of-header-guard GLFWINSTANCE_HPP_AU0HO3FK
// EOF
