#pragma once // potentially faster compile-times if supported
#ifndef VKINSTANCE_HPP_6GSYQOH2
#define VKINSTANCE_HPP_6GSYQOH2

#include "MyTemplate/Renderer/common.hpp"
#include <memory>

namespace gfx {
	class VkInstance {
		public:
			VkInstance(           GlfwInstance const  & )         ;
			VkInstance(             VkInstance const  & ) = delete;
			VkInstance(             VkInstance       && ) noexcept;
			~VkInstance(                                ) noexcept;
			VkInstance & operator=( VkInstance const  & ) = delete;
			VkInstance & operator=( VkInstance       && ) noexcept;
			vk::raii::Context  const & get_context()  const;
			vk::raii::Context        & get_context()       ;
			vk::raii::Instance const & get_instance() const;
			vk::raii::Instance       & get_instance()      ;
		private:
			std::unique_ptr<vk::raii::Context>  m_p_context;
			std::unique_ptr<vk::raii::Instance> m_p_instance;
	}; // end-of-class: VkInstance
} // end-of-namespace: gfx

#endif // end-of-header-guard: VKINSTANCE_HPP_6GSYQOH2
// EOF
