#include "MyTemplate/Renderer/Pipeline.hpp"
#include "MyTemplate/Renderer/Swapchain.hpp"
#include "MyTemplate/Common/aliases.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <array>
#include <vector>
#include <string>
#include <stdexcept>

namespace { // private (file-scope)

	[[nodiscard]] auto
	load_binary_from_file( std::string const &binary_filename )
	{  // TODO: null_terminated_string_view?
		spdlog::info( "Attempting to open binary file `{}`...", binary_filename );
		std::ifstream binary_file {
			binary_filename,
			std::ios::binary | std::ios::ate // start at the end of the binary
		};
		if ( binary_file ) {
			spdlog::info( "... successful!" );
			auto const binary_size {
				static_cast<std::size_t>( binary_file.tellg() )
			};
			spdlog::info( "... binary size: {}", binary_size );
			binary_file.seekg( 0 );
			std::vector<char> binary_buffer( binary_size );
			binary_file.read( binary_buffer.data(), binary_size );
			return binary_buffer;
		}
		else {
			spdlog::warn( "... failure!" );
			throw std::runtime_error { "Failed to load binary file!" };
		}
	} // end-of-function: load_binary_from_file
				
	[[nodiscard]] auto
	make_shader_module_from_binary(
		vk::raii::Device        &logical_device,
		std::vector<char> const &shader_binary
	)
	{
		spdlog::info( "Creating shader module from shader SPIR-V bytecode..." );
		return std::make_unique<vk::raii::ShaderModule>(
			logical_device,
			vk::ShaderModuleCreateInfo {
				.codeSize = shader_binary.size(),
				.pCode    = reinterpret_cast<u32 const *>( shader_binary.data() ) /// TODO: UB or US?
			}	
		);
	} // end-of-function: make_shader_module_from_file
	
	[[nodiscard]] auto
	make_shader_module_from_file(
		vk::raii::Device  &logical_device,
		std::string const &shader_spirv_bytecode_filename
	)
	{
		spdlog::info( "Creating shader module from shader SPIR-V bytecode file..." );
		return make_shader_module_from_binary(
			logical_device,
			load_binary_from_file( shader_spirv_bytecode_filename )
		);
	} // end-of-function: make_shader_module_from_file
	
	[[nodiscard]] auto
	make_pipeline_layout( vk::raii::Device &logical_device )
	{
		spdlog::info( "Creating pipeline layout..." );
		return std::make_unique<vk::raii::PipelineLayout>(
			logical_device,
			vk::PipelineLayoutCreateInfo {
				.setLayoutCount         = 0,       // TODO: explain
				.pSetLayouts            = nullptr, // TODO: explain
				.pushConstantRangeCount = 0,       // TODO: explain
				.pPushConstantRanges    = nullptr  // TODO: explain
			}
		);
	} // end-of-function: make_pipeline_layout
	
	[[nodiscard]] auto
	make_render_pass( vk::raii::Device &logical_device, Swapchain const &swapchain )
	{
		spdlog::info( "Creating render pass..." );
		vk::AttachmentDescription const
		color_attachment_description {
			.format         = swapchain.get_surface_format().format,
			.samples        = vk::SampleCountFlagBits::e1, // no MSAA yet
			.loadOp         = vk::AttachmentLoadOp::eLoad,
			.storeOp        = vk::AttachmentStoreOp::eStore,
			.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,  // no depth/stencil yet
			.stencilStoreOp = vk::AttachmentStoreOp::eDontCare, // no depth/stencil yet
			.initialLayout  = vk::ImageLayout::eUndefined,
			.finalLayout    = vk::ImageLayout::ePresentSrcKHR
		};
		
		vk::AttachmentReference const
		color_attachment_reference {
			.attachment = 0, // index 0; we only have one attachment at the moment
			.layout     = vk::ImageLayout::eColorAttachmentOptimal
		};
		
		vk::SubpassDescription const
		color_subpass_description {
			.pipelineBindPoint    =  vk::PipelineBindPoint::eGraphics,
			.colorAttachmentCount =  1,
			.pColorAttachments    = &color_attachment_reference
		};
		
		return std::make_unique<vk::raii::RenderPass>(
			logical_device,
			vk::RenderPassCreateInfo {
				.attachmentCount =  1,
				.pAttachments    = &color_attachment_description,
				.subpassCount    =  1,
				.pSubpasses      = &color_subpass_description
			}
		);
	} // end-of-function: make_render_pass

} // end-of-unnamed-namespace

// TODO: take swapchain as second arg?
Pipeline::Pipeline( vk::raii::Device &logical_device, Swapchain const &swapchain )
{
	spdlog::info( "Constructing Pipeline instance..." );
	
	spdlog::info( "Creating shader modules..." );
	m_p_vertex_shader_module = make_shader_module_from_file(
		logical_device,
		"../dat/shaders/test.vert.spv"
	);
	
	m_p_fragment_shader_module = make_shader_module_from_file(
		logical_device,
		"../dat/shaders/test.frag.spv"
	);
	
// Creating shader stage(s):
	
	spdlog::info( "Creating pipeline shader stages..." );
	
	vk::PipelineShaderStageCreateInfo const
	pipeline_shader_stages[]
	{
		{
			.stage  =   vk::ShaderStageFlagBits::eVertex,
			.module = **m_p_vertex_shader_module,
			.pName  =  "main" // shader program entry point
		// .pSpecializationInfo is unused (for now);
		//    but it allows for setting shader constants
		},
		{
			.stage  =   vk::ShaderStageFlagBits::eFragment,
			.module = **m_p_fragment_shader_module,
			.pName  =  "main" // shader program entry point
		// .pSpecializationInfo is unused (for now);
		//     but it allows for setting shader constants
		}
	};
	
	// WHAT: configures the vertex data format (spacing, instancing, loading...)
	vk::PipelineVertexInputStateCreateInfo const
	pipeline_vertex_input_state_create_info
	{  // NOTE: no data here since it's hardcoded (for now)
		.vertexBindingDescriptionCount   = 0,
		.pVertexBindingDescriptions      = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions    = nullptr
	};
	
	// WHAT: configures the primitive topology of the geometry
	vk::PipelineInputAssemblyStateCreateInfo const
	pipeline_input_assembly_state_create_info
	{
		.topology               = vk::PrimitiveTopology::eTriangleList,
		.primitiveRestartEnable = VK_FALSE // unused (for now);
	}; // otherwise it allows for designating gap indices in strips
	
// Create viewport state:
	auto const &surface_extent {
		swapchain.get_surface_extent()
	};
	
	vk::Viewport const viewport {
		.x        = 0.0f,
		.y        = 0.0f,
		.width    = static_cast<f32>( surface_extent.width  ),
		.height   = static_cast<f32>( surface_extent.height ),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	
	vk::Rect2D const scissor {
		.offset = { 0, 0 },
		.extent = surface_extent
	};
	
	vk::PipelineViewportStateCreateInfo const
	pipeline_viewport_state_create_info
	{
		.viewportCount =  1,
		.pViewports    = &viewport,
		.scissorCount  =  1,
		.pScissors     = &scissor,
	};
	
// Rasterizer:
	
	vk::PipelineRasterizationStateCreateInfo const
	pipeline_rasterization_state_create_info
	{
		.depthClampEnable        = VK_FALSE, // mostly just useful for shadow maps; requires GPU feature
		.rasterizerDiscardEnable = VK_FALSE, // seems pointless
		.polygonMode             = vk::PolygonMode::eFill, // wireframe and point requires GPU feature
		.cullMode                = vk::CullModeFlagBits::eBack, // backface culling
		.frontFace               = vk::FrontFace::eClockwise, // vertex winding order
		.depthBiasEnable         = VK_FALSE, // mostly just useful for shadow maps
		.depthBiasConstantFactor = 0.0f,     // mostly just useful for shadow maps
		.depthBiasClamp          = 0.0f,     // mostly just useful for shadow maps
		.depthBiasSlopeFactor    = 0.0f,     // mostly just useful for shadow maps
		.lineWidth               = 1.0f      // >1 requires wideLines GPU feature
	};
	
// MSAA:
	
	vk::PipelineMultisampleStateCreateInfo const
	pipeline_multisample_state_create_info
	{  // TODO: revisit later
		.rasterizationSamples  = vk::SampleCountFlagBits::e1,
		.sampleShadingEnable   = VK_FALSE, // unused; otherwise enables MSAA; requires GPU feature
		.minSampleShading      = 1.0f,
		.pSampleMask           = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable      = VK_FALSE 
	};
	
// Depth & stencil testing:
	
	vk::PipelineDepthStencilStateCreateInfo const
	pipeline_depth_stencil_state_create_info
	{
		// TODO: unused for now; revisit later
	};
	
// Color blending:
	
	vk::PipelineColorBlendAttachmentState const
	pipeline_color_blend_attachment_state
	{
		.blendEnable         = VK_FALSE,
		.srcColorBlendFactor = vk::BlendFactor::eOne,
		.dstColorBlendFactor = vk::BlendFactor::eZero,
		.colorBlendOp        = vk::BlendOp::eAdd,
		.srcAlphaBlendFactor = vk::BlendFactor::eOne,
		.dstAlphaBlendFactor = vk::BlendFactor::eZero,
		.alphaBlendOp        = vk::BlendOp::eAdd,
		.colorWriteMask      = vk::ColorComponentFlagBits::eR
									| vk::ColorComponentFlagBits::eG
									| vk::ColorComponentFlagBits::eB
									| vk::ColorComponentFlagBits::eA,
	};
	
	// NOTE: blendEnable and logicOpEnable are mutually exclusive!
	vk::PipelineColorBlendStateCreateInfo const
	color_blend_state_create_info
	{
		.logicOpEnable     =  VK_FALSE,
		.logicOp           =  vk::LogicOp::eCopy,
		.attachmentCount   =  1, // TODO: explain
		.pAttachments      = &pipeline_color_blend_attachment_state,
		.blendConstants    =  std::array<f32,4>{ 0.0f, 0.0f, 0.0f, 0.0f }
	};
	
// Dynamic state:
	
	// e.g: std::array<vk::DynamicState> const dynamic_states {
	//         vk::DynamicState::eLineWidth	
	//      };
	// 
	// vk::PipelineDynamicStateCreateInfo const
	// dynamic_state_create_info
	// { // NOTE: unused (for now)
	// 	.dynamicStateCount = 0,
	// 	.pDynamicStates    = nullptr
	// };
	
// Pipeline layout:
	m_p_pipeline_layout   = make_pipeline_layout( logical_device );
	m_p_render_pass       = make_render_pass( logical_device, swapchain );
	m_p_graphics_pipeline = std::make_unique<vk::raii::Pipeline>(
		logical_device,
		nullptr,
		vk::GraphicsPipelineCreateInfo {
			.stageCount          =   2,
			.pStages             =   pipeline_shader_stages,
			.pVertexInputState   =  &pipeline_vertex_input_state_create_info,
			.pInputAssemblyState =  &pipeline_input_assembly_state_create_info,
			.pViewportState      =  &pipeline_viewport_state_create_info,
			.pRasterizationState =  &pipeline_rasterization_state_create_info,
			.pMultisampleState   =  &pipeline_multisample_state_create_info,
			.pDepthStencilState  =   nullptr, // unused for now
			.pColorBlendState    =  &color_blend_state_create_info,
			.pDynamicState       =   nullptr, // unused for now
			.layout              = **m_p_pipeline_layout,
			.renderPass          = **m_p_render_pass,
			.subpass             =   0,
			.basePipelineHandle  =   VK_NULL_HANDLE,
			.basePipelineIndex   =   -1,
		}
	);
} // end-of-function: Pipeline::Pipeline

// EOF
