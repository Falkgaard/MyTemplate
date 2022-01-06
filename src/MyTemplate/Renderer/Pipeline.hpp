#pragma once // potentially faster compile-times if supported
#ifndef PIPELINE_HPP_K7YBFDM9
#define PIPELINE_HPP_K7YBFDM9

#include "MyTemplate/Renderer/common.hpp"
#include <memory>

namespace gfx {
	class Pipeline {
		public:
			Pipeline( vk::raii::Device const &, Swapchain const &); // TODO
			Pipeline(             Pipeline const &  ) = delete;
			Pipeline(             Pipeline       && ) noexcept;
			~Pipeline(                              ) noexcept;
			Pipeline & operator=( Pipeline const &  ) = delete;
			Pipeline & operator=( Pipeline       && ) noexcept;
			vk::raii::RenderPass const & get_render_pass() const;
			vk::raii::RenderPass       & get_render_pass()      ;
		private:
			std::unique_ptr<vk::raii::ShaderModule>    m_p_vertex_shader_module   ;
			std::unique_ptr<vk::raii::ShaderModule>    m_p_fragment_shader_module ;
			std::unique_ptr<vk::raii::PipelineLayout>  m_p_pipeline_layout        ;
			std::unique_ptr<vk::raii::RenderPass>      m_p_render_pass            ;
			std::unique_ptr<vk::raii::Pipeline>        m_p_graphics_pipeline      ;
	}; // end-of-class: Pipeline
} // end-of-namespace: gfx

#endif // end-of-header-guard: PIPELINE_HPP_K7YBFDM9
// EOF
