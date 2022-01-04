#pragma once // potentially faster compile-times if supported
#ifndef RENDERER_HPP_YBLYHOXN
#define RENDERER_HPP_YBLYHOXN

#include "MyTemplate/Common/aliases.hpp"
#include <memory>

// forward declarations:
class GlfwInstance;
class Window;
class Swapchain;
class Pipeline;
namespace vk::raii {
	class Context;
	class Instance;
	class DebugUtilsMessengerEXT;
	class PhysicalDevice;
	class Device;
	class Queue;
	class CommandPool;
	class CommandBuffers;
} // end-of-namespace: vk::raii

// NOTE: value corresponds to number of frames to allow for static casting
enum struct FramebufferingPriority: u32 {
	eSingle = 1,
	eDouble = 2,
	eTriple = 3 
}; // end-of-enum-struct: FramebufferingPriority

enum struct PresentationPriority {
	eMinimalLatency,        
	eMinimalStuttering,     
	eMinimalPowerConsumption
}; // end-of-enum-struct: PresentationPriority

struct QueueFamilyIndices {
	u32  present;
	u32  graphics;
	bool are_separate;
}; // end-of-struct: QueueFamilyIndices
		
class Renderer final {
	public:
		Renderer();
		Renderer( Renderer const &  ) = delete;
		Renderer( Renderer       && ) noexcept;
		~Renderer()                   noexcept;
		// TODO: update, draw, load_texture, etc?
		[[nodiscard]] Window const & get_window() const;
		[[nodiscard]] Window       & get_window();
	private:
		// NOTE: declaration order here is very important!
		std::unique_ptr<GlfwInstance>                         m_p_glfw_instance      ;
		std::unique_ptr<vk::raii::Context>                    m_p_vk_context         ;
		std::unique_ptr<vk::raii::Instance>                   m_p_vk_instance        ;
		#if !defined( NDEBUG )
			std::unique_ptr<vk::raii::DebugUtilsMessengerEXT>  m_p_debug_messenger    ;
		#endif
		std::unique_ptr<Window>                               m_p_window             ;
		std::unique_ptr<vk::raii::PhysicalDevice>             m_p_physical_device    ;
		QueueFamilyIndices                                    m_queue_family_indices ;
		std::unique_ptr<vk::raii::Device>                     m_p_logical_device     ;
		std::unique_ptr<vk::raii::Queue>                      m_p_graphics_queue     ;
		std::unique_ptr<vk::raii::Queue>                      m_p_present_queue      ;
		std::unique_ptr<vk::raii::CommandPool>                m_p_command_pool       ;
		std::unique_ptr<vk::raii::CommandBuffers>             m_p_command_buffers    ;
		std::unique_ptr<Swapchain>                            m_p_swapchain          ;
		std::unique_ptr<Pipeline>                             m_p_pipeline           ;
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

#endif // end-of-header-guard RENDERER_HPP_YBLYHOXN
// EOF
