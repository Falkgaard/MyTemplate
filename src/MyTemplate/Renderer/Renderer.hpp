#pragma once // potentially faster compile-times if supported
#ifndef RENDERER_HPP_YBLYHOXN
#define RENDERER_HPP_YBLYHOXN

#include "MyTemplate/Renderer/common.hpp"	
#include <memory>

namespace gfx {
	class Renderer final {
		public:
			Renderer();
			Renderer( Renderer const &  ) = delete;
			Renderer( Renderer       && ) noexcept;
			~Renderer()                   noexcept;
			// TODO: update, draw, load_texture, etc?
			[[nodiscard]] Window const & get_window() const;
			[[nodiscard]] Window       & get_window()      ;
		private:
			// NOTE: declaration order here is very important!
			std::unique_ptr<GlfwInstance>                         m_p_glfw_instance      ;
			std::unique_ptr<VkInstance>                           m_p_vk_instance        ;
			#if !defined( NDEBUG )
				std::unique_ptr<vk::raii::DebugUtilsMessengerEXT>  m_p_debug_messenger    ; // TODO: refactor into own class?
			#endif
			std::unique_ptr<Window>                               m_p_window             ;
			std::unique_ptr<vk::raii::PhysicalDevice>             m_p_physical_device    ; // TODO: refactor into LogicalDevice
			QueueFamilyIndices                                    m_queue_family_indices ; // TODO: refactor into LogicalDevice
			std::unique_ptr<vk::raii::Device>                     m_p_logical_device     ; // TODO: refactor into own class
			std::unique_ptr<vk::raii::Queue>                      m_p_graphics_queue     ;
			std::unique_ptr<vk::raii::Queue>                      m_p_present_queue      ;
			std::unique_ptr<vk::raii::CommandPool>                m_p_command_pool       ;
			std::unique_ptr<vk::raii::CommandBuffers>             m_p_command_buffers    ;
			std::unique_ptr<Swapchain>                            m_p_swapchain          ;
			std::unique_ptr<Pipeline>                             m_p_pipeline           ;
			std::unique_ptr<Framebuffers>                         m_p_framebuffers       ;
			// TODO: queue_family_indices as well? (array)
			#if 0
				// LONG TODO:
				Depth Buffer(s) + related
				Uniform Buffer(s) + related
				MVP
				DecriptorPool + DescriptorSet(s)
				PipelineLayout
				ShaderModule(s)
				ShaderStages
				PipelineStates
				Viewport
				Rasterizer
				MSAA
				DepthStencil
				ColorBlend
				+ more
			#endif
	}; // end-of-class: Renderer
} // end-of-namespace: gfx

#endif // end-of-header-guard RENDERER_HPP_YBLYHOXN
// EOF
