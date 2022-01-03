#include "MyTemplate/Renderer/Pipeline.hpp"

#if 0 // TODO:
	// Graphics pipeline:
		auto const create_graphics_pipeline {
			[]( vk::raii::Device &device, vk::Extent2D const &surface_extent )
			{  // TODO: refactor into free function
				spdlog::info( "Creating graphics pipeline..." );
				
				auto const load_binary_from_file {
					// TODO: refactor into free function
					[]( std::string const &binary_filename )
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
					} // end-of-lambda-body
				}; // end-of-lambda: load_binary_from_file

				auto const create_shader_module_from_binary {
					[]( vk::raii::Device &device, std::vector<char> const &shader_binary )
					{  // TODO: refactor into free function
						spdlog::info( "Creating shader module from shader SPIR-V bytecode..." );
						vk::ShaderModuleCreateInfo const shader_module_create_info {
							.codeSize = shader_binary.size(),
							.pCode    = reinterpret_cast<u32 const *>( shader_binary.data() ) /// TODO: UB or US?
						};
						return vk::raii::ShaderModule(
							device,
							shader_module_create_info
						);
					} // end-of-lambda-body
				}; // end-of-lambda: create_shader_module_from_file
				
				auto const create_shader_module_from_file {
					[&load_binary_from_file, &create_shader_module_from_binary] (
						vk::raii::Device  &device,
						std::string const &shader_spirv_bytecode_filename
					)
					{ // TODO: refactor into free function
						spdlog::info( "Creating shader module from shader SPIR-V bytecode file..." );
						return create_shader_module_from_binary(
							device,
							load_binary_from_file( shader_spirv_bytecode_filename )
						);
					} // end-of-lambda-body
				}; // end-of-lambda: create_shader_module_from_file
				
				spdlog::info( "Creating shader modules..." );
				
				auto const vertex_shader_module {
					create_shader_module_from_file( device, "../dat/shaders/test.vert.spv" )
				};
				
				auto const fragment_shader_module {
					create_shader_module_from_file( device, "../dat/shaders/test.frag.spv" )
				};
				
			// Creating shader stage(s):
				
				spdlog::info( "Creating pipeline shader stages..." );
				
				vk::PipelineShaderStageCreateInfo const
				pipeline_shader_stages[]
				{
					{
						.stage  =  vk::ShaderStageFlagBits::eVertex,
						.module = *vertex_shader_module,
						.pName  =  "main" // shader program entry point
					// .pSpecializationInfo is unused (for now);
					//    but it allows for setting shader constants
					},
					{
						.stage  =  vk::ShaderStageFlagBits::eFragment,
						.module = *fragment_shader_module,
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
					// TODO: revisit later
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
				
			} // end-of-lambda-body
		}; // end-of-lambda: make_graphics_pipeline
#endif
// EOF
