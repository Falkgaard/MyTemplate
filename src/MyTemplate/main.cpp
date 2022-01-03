#include "MyTemplate/info.hpp"

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <fRNG/core.hh>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>

#include <fstream>
#include <cstdlib>

#include "MyTemplate/Common/utility.hpp"
#include "MyTemplate/Common/aliases.hpp"
#include "MyTemplate/Renderer/Renderer.hpp"
#include "MyTemplate/Renderer/Window.hpp"

	int
main()
{
	std::atexit( spdlog::shutdown );
	
	spdlog::info(
		"Starting MyTemplate v{}.{}.{}...",
		MYTEMPLATE_VERSION_MAJOR, MYTEMPLATE_VERSION_MINOR, MYTEMPLATE_VERSION_PATCH
	);
	
	if constexpr ( g_is_debug_mode ) {
		spdlog::set_level( spdlog::level::debug );
		spdlog::info( "Build: DEBUG" );
	}
	else {
		spdlog::info( "Build: RELEASE" );
	}
	
	try {
		Renderer renderer {};
		
#if 0
		auto const find_memory_type_index {
			[](
				vk::PhysicalDeviceMemoryProperties const &physical_device_memory_properties,
				u32                                const  type_filter,
				vk::MemoryPropertyFlags            const  memory_property_flags
			) -> u32 {
				for ( u32 i{0}; i<physical_device_memory_properties.memoryTypeCount; ++i ) 
					if ( (type_filter & (1<<i)) and
						  (physical_device_memory_properties.memoryTypes[i].propertyFlags & memory_property_flags)
						      == memory_property_flags )
					{
						return i;
					}
				throw std::runtime_error { "Unable to find an index of a suitable memory type!" };
			}
		};
		
	// Depth buffer:
		spdlog::info( "Creating depth buffer image(s)..." );
		// TODO: Look into more efficient allocation (batch allocation)
		// TODO: unify image creation with a helper function
		// NOTE: The vectors in this section are so that each framebuffer will have it's own depth buffer.
		// NOTE: If the memory overhead ends up too expensive,
		//       the renderpass can be made to use only a single depth buffer
		//       for all framebuffers, however this would likely affect performance.
		
		vk::ImageCreateInfo const depth_buffer_image_create_info {
			.imageType             = vk::ImageType::e2D,
			.format                = vk::Format::eD16Unorm, // TODO: query support
			.extent                = vk::Extent3D {
			                          .width  = surface_extent.width,
			                          .height = surface_extent.height,
			                          .depth  = 1
			                       },
			.mipLevels             = 1, // no mipmapping
			.arrayLayers           = 1,
			.samples               = vk::SampleCountFlagBits::e1, // 1 sample per pixel (no MSAA; TODO: revisit later?)
			.tiling                = vk::ImageTiling::eOptimal,
			.usage                 = vk::ImageUsageFlagBits::eDepthStencilAttachment,
			.sharingMode           = vk::SharingMode::eExclusive,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices   = nullptr,
			.initialLayout         = vk::ImageLayout::eUndefined
		};
		
		std::vector<vk::raii::Image> depth_buffer_images {};
		// NOTE: one depth buffer per swapchain framebuffer image
		depth_buffer_images.reserve( swapchain_framebuffer_count );
		for ( u32 i{0}; i<swapchain_framebuffer_count; ++i )
			depth_buffer_images.emplace_back( device, depth_buffer_image_create_info );
		
		// NOTE: since they're all identical, we just use the front image here	
		auto const depth_buffer_image_memory_requirements {
			depth_buffer_images.front().getMemoryRequirements() 
		};
		
		auto const physical_device_memory_properties {
			physical_device.getMemoryProperties()
		};
		
		// NOTE: this will be used for all depth buffer allocations
		auto const depth_buffer_image_memory_type_index {
			find_memory_type_index(
				physical_device_memory_properties,
				depth_buffer_image_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			)
		};
		
		// NOTE: this will be used for all depth buffer allocations
		vk::MemoryAllocateInfo const depth_buffer_image_memory_allocate_info {
			.allocationSize  = depth_buffer_image_memory_requirements.size,
			.memoryTypeIndex = depth_buffer_image_memory_type_index
		};
		
		std::vector<vk::raii::DeviceMemory> depth_buffer_image_device_memories {};
		// NOTE: one device memory per depth buffer
		// TODO: unify depth buffer data in some POD struct
		depth_buffer_image_device_memories.reserve( swapchain_framebuffer_count );
		// allocate and bind memory for each depth buffer image:
		for ( u32 i{0}; i<swapchain_framebuffer_count; ++i ) {
			depth_buffer_image_device_memories.emplace_back( device, depth_buffer_image_memory_allocate_info );
			depth_buffer_images[i].bindMemory( *depth_buffer_image_device_memories[i], 0 );
		}
		
		// TODO: descriptor sets stuff?
		
	// Depth buffer image view:
		spdlog::info( "Creating depth buffer image view(s)..." );
		vk::ImageViewCreateInfo depth_buffer_image_view_create_info {
		//	.image will be set afterwards in a for-loop
			.viewType         = vk::ImageViewType::e2D,
			.format           = vk::Format::eD16Unorm, // TODO: query support
			.subresourceRange = vk::ImageSubresourceRange {
			                       .aspectMask     = vk::ImageAspectFlagBits::eDepth,
			                       .baseMipLevel   = 0u,
			                       .levelCount     = 1u,
			                       .baseArrayLayer = 0u,
			                       .layerCount     = 1u
			                    }
		};
		
		std::vector<vk::raii::ImageView> depth_buffer_image_views {};
		// NOTE: one image view per depth buffer
		depth_buffer_image_views.reserve( swapchain_framebuffer_count );
		for ( u32 i{0}; i<swapchain_framebuffer_count; ++i ) {
			depth_buffer_image_view_create_info.image = *depth_buffer_images[i];
			depth_buffer_image_views.emplace_back( device, depth_buffer_image_view_create_info );
		}	
		
		// TODO: renderpass stuff + subpass stuff, etc ASAP
		
	// Uniform buffer:
		spdlog::info( "Creating uniform data buffer..." );
		
		// TODO: allocate one for each swapchain buffer?	
		
		auto const projection {
			glm::perspective( glm::radians(45.f), 1.0f, 0.1f, 100.0f )
		};
		
		auto const view {
			glm::lookAt(
				glm::vec3( -5,  3, -10 ), // world space camera position
				glm::vec3(  0,  0,   0 ), // look at the origin
				glm::vec3(  0, -1,   0 )  // head is up (remove minus?)
			)
		};
		
		auto const model {
			glm::mat4( 1.f )
		};
		
		auto const clip { // Vulkan clip space has inverted Y and half Z
			glm::mat4(
				1.0f,  0.0f,  0.0f,  0.0f,
				0.0f, -1.0f,  0.0f,  0.0f,
				0.0f,  0.0f,  0.5f,  0.0f,
				0.0f,  0.0f,  0.5f,  1.0f
			)
		};
		
		auto const mvp {
			clip * projection * view * model
		};
		
		vk::BufferCreateInfo const uniform_data_buffer_create_info {
			.size                  = sizeof( mvp ),
			.usage                 = vk::BufferUsageFlagBits::eUniformBuffer,
			.sharingMode           = vk::SharingMode::eExclusive,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices   = nullptr
			// NOTE: might need .flags
		};
		
		vk::raii::Buffer uniform_data_buffer( device, uniform_data_buffer_create_info );
		
		vk::MemoryRequirements const uniform_data_buffer_memory_requirements {
			uniform_data_buffer.getMemoryRequirements()
		};
		
		auto const uniform_data_buffer_memory_type_index {
			find_memory_type_index(
				physical_device_memory_properties,
				uniform_data_buffer_memory_requirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
			)
		};
		
		vk::MemoryAllocateInfo const uniform_data_buffer_memory_allocate_info {
			.allocationSize  = uniform_data_buffer_memory_requirements.size,
			.memoryTypeIndex = uniform_data_buffer_memory_type_index
		};
		
		vk::raii::DeviceMemory uniform_data_buffer_device_memory(
			device,
			uniform_data_buffer_memory_allocate_info
		);
		
		{ // TODO: verify block
			spdlog::info( "Mapping the uniform data buffer's device memory to the host device's memory..." );
			auto *p_mapped_memory {
				uniform_data_buffer_device_memory.mapMemory( 0, uniform_data_buffer_memory_requirements.size )
			};
			spdlog::info( "Uploading the MVP matrix to the mapped uniform data buffer memory..." );
			std::memcpy( p_mapped_memory, &mvp, sizeof(mvp) );
			spdlog::info( "Unmapping memory..." );
			uniform_data_buffer_device_memory.unmapMemory(); // TODO: remove when uniform buffer is dynamic?
		}
			
	// Descriptor pool:
		spdlog::info( "Creating descriptor pool..." );
		
		vk::DescriptorPoolSize const descriptor_pool_size {
			.type            = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = swapchain_framebuffer_count // one for each frame
		};
		
		vk::DescriptorPoolCreateInfo const descriptor_pool_create_info {
			.maxSets       =  swapchain_framebuffer_count, // at most one per frame
			.poolSizeCount =  1, // TODO: comment properly
			.pPoolSizes    = &descriptor_pool_size
		// .flag is unused by us
		};
		
		vk::raii::DescriptorPool descriptor_pool( device, descriptor_pool_create_info );
		
		auto const generate_swapchain {
			[] { // TODO!!!
				create_uniform_buffers();
				create_descriptor_pool();
				create_command_buffers();
			}
		};
			
	// Descriptor set binding & layout for uniform buffer:
		spdlog::info( "Creating descriptor set layout binding for the uniform data buffer..." );
		
		vk::DescriptorSetLayoutBinding const uniform_data_buffer_descriptor_set_layout_binding {
			.binding            = 0, // only making one set; index zero (TODO: read up on)
			.descriptorType     = vk::DescriptorType::eUniformBuffer,
			.descriptorCount    = 1, // only one descriptor in the set
			.stageFlags         = vk::ShaderStageFlagBits::eVertex,
			.pImmutableSamplers = nullptr // NOTE: ignore until later
		};
		
		spdlog::info( "Creating descriptor set layout for the uniform data buffer..." );
		
		vk::DescriptorSetLayoutCreateInfo const uniform_data_buffer_descriptor_set_layout_create_info {
			.bindingCount    =  1, // only one descriptor set (TODO: verify wording)
			.pBindings       = &uniform_data_buffer_descriptor_set_layout_binding,
		};
		
		vk::raii::DescriptorSetLayout uniform_data_buffer_descriptor_set_layout(
			device,
			uniform_data_buffer_descriptor_set_layout_create_info
		);
		
	// Pipeline layout:
		spdlog::info( "Creating pipeline layout..." );
		
		vk::PipelineLayoutCreateInfo const pipeline_layout_create_info {
			.setLayoutCount         =   1,
			.pSetLayouts            = &*uniform_data_buffer_descriptor_set_layout, // TODO: verify &*
			.pushConstantRangeCount =   0,             // TODO: explain purpose
			.pPushConstantRanges    =   VK_NULL_HANDLE //       ditto
		};
		
		vk::raii::PipelineLayout pipeline_layout( device, pipeline_layout_create_info ); 
		
		// NOTE (shader usage): layout (std140, binding = 0) uniform bufferVals {
		// NOTE (shader usage):     mat4 mvp;
		// NOTE (shader usage): } myBufferVals;

	// Descriptor set(s):
		spdlog::info( "Allocating descriptor set(s)..." );	
		
		vk::DescriptorSetAllocateInfo const descriptor_set_allocate_info {
			.descriptorPool     =  *descriptor_pool,
			.descriptorSetCount =   swapchain_framebuffer_count,
			.pSetLayouts        = &*uniform_data_buffer_descriptor_set_layout // TODO: array
		};
		
		auto descriptor_sets {
			device.allocateDescriptorSets( descriptor_set_allocate_info )
		};
		
		for ( u32 i{0}; i<swapchain_framebuffer_count; ++i ) {
			vk::DescriptorBufferInfo const descriptor_buffer_info {
				.buffer = *uniform_data_buffer, // TODO!!!! should be array
				.offset =  0,
				.range  =  sizeof( glm::mat4 )
			};
		}
#endif
		// TODO: make update_uniform_buffer function
			
///////////////////////////////////////////////////////////////////////////////////////
		// main loop:
		auto &window {
			renderer.get_window()
		};
		while ( window.was_closed() == false ) {
			window.update();
			// TODO: handle input, update logic, render, draw window
		}
		spdlog::info( "Exiting MyTemplate..." );
	}
	catch ( vk::SystemError const &e ) {
		spdlog::critical( "vk::SystemError: {}", e.what() );
		std::exit( EXIT_FAILURE );
	}
	catch ( std::exception const &e ) {
		spdlog::critical( "std::exception: {}", e.what() );
		std::exit( EXIT_FAILURE );
	}
	catch (...) {
		spdlog::critical( "Unknown error encountered!" );
		std::exit( EXIT_FAILURE );
	}
	std::exit( EXIT_SUCCESS );
} // end-of-function: main

/*
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
TEST_CASE ("Test function") {
	CHECK( f(6)    ==   69 );
	CHECK( f(9)    ==   69 );
	CHECK( f(123)  ==  169 );
	CHECK( f(420)  ==  469 );
	CHECK( f(3234) == 3269 );
}
*/
