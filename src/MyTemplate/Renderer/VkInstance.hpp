#pragma once // potentially faster compile-times if supported
#ifndef VKINSTANCE_HPP_6GSYQOH2
#define VKINSTANCE_HPP_6GSYQOH2

#include <memory>

// forward declarations:
class GlfwInstance;
namespace vk::raii {
	class Instance;
}; // end-of-namespace: vk::raii

class VkInstance {
	public:
		VkInstance(           GlfwInstance const  & )         ;
		VkInstance(             VkInstance const  & ) = delete;
		VkInstance(             VkInstance       && ) noexcept;
		~VkInstance(                                ) noexcept;
		VkInstance & operator=( VkInstance const  & ) = delete;
		VkInstance & operator=( VkInstance       && ) noexcept;
	private:
		std::unique_ptr<vk::raii::Instance> m_p_instance;
}; // end-of-class: VkInstance

#endif // end-of-header-guard: VKINSTANCE_HPP_6GSYQOH2
// EOF
