#pragma once // potentially faster compile-times if supported
#ifndef PIPELINE_HPP_K7YBFDM9
#define PIPELINE_HPP_K7YBFDM9

#include <memory>

// forward declarations:
class Swapchain;
namespace vk::raii {
	class Device;
	class ShaderModule;
	class PipelineLayout;
	class RenderPass;
	class Pipeline;
} // end-of-namespace: vk::raii

class Pipeline {
	public:
		Pipeline( vk::raii::Device const &logical_device, Swapchain const &swapchain );
		~Pipeline() noexcept;
	private:
		std::unique_ptr<vk::raii::ShaderModule>    m_p_vertex_shader_module;
		std::unique_ptr<vk::raii::ShaderModule>    m_p_fragment_shader_module;
		std::unique_ptr<vk::raii::PipelineLayout>  m_p_pipeline_layout;
		std::unique_ptr<vk::raii::RenderPass>      m_p_render_pass;
		std::unique_ptr<vk::raii::Pipeline>        m_p_graphics_pipeline;
}; // end-of-class: Pipeline

#endif // end-of-header-guard: PIPELINE_HPP_K7YBFDM9
// EOF
