#include "MyTemplate/Renderer/Renderer.hpp"

#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include "MyTemplate/Renderer/VkInstance.hpp"
#include "MyTemplate/Renderer/Window.hpp"
#include "MyTemplate/Renderer/Swapchain.hpp"
#include "MyTemplate/Renderer/Pipeline.hpp"
#include "MyTemplate/Renderer/Framebuffers.hpp"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <ranges>
#include <algorithm>
#include <optional>
#include <array>
#include <vector>

namespace gfx {
	namespace { // private (file-scope)
		
		// TODO: refactor out
		std::array constexpr required_device_extensions {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		
		[[nodiscard]] bool
		is_supporting_extensions(
			vk::raii::PhysicalDevice const &physical_device
		)
		{
			auto const &available_device_extensions {
				physical_device.enumerateDeviceExtensionProperties()
			};
			
			if constexpr ( g_is_debug_mode ) {
				spdlog::info( "... checking device extension support:" );
				// print required and available device extensions:
				for ( auto const &required_device_extension: required_device_extensions )
					spdlog::info( "      required  : `{}`", required_device_extension );
				for ( auto const &available_device_extension: available_device_extensions )
					spdlog::info( "      available : `{}`", available_device_extension.extensionName );
			}	
			
			bool is_adequate { true }; // assume true until proven otherwise
			for ( auto const &required_device_extension: required_device_extensions ) {
				bool is_supported { false }; // assume false until found
				for ( auto const &available_device_extension: available_device_extensions ) {
					if ( std::strcmp( required_device_extension, available_device_extension.extensionName ) == 0 ) [[unlikely]] {
						is_supported = true;
						break; // early exit
					}
				}
				spdlog::info(
					"      support for required device extension `{}`: {}",
					required_device_extension, is_supported ? "OK" : "missing!"
				);
				if ( not is_supported ) [[unlikely]]
					is_adequate = false;
			}
			return is_adequate;
		} // end-of-function: gfx::<unnamed>::is_supporting_extensions
		
		[[nodiscard]] u32
		score_physical_device(
			vk::raii::PhysicalDevice const &physical_device
		)
		{
			auto const properties { physical_device.getProperties() }; // TODO: KHR2?
			auto const features   { physical_device.getFeatures()   }; // TODO: 2KHR?
			spdlog::info( "Scoring physical device `{}`...", properties.deviceName.data() );
			u32 score { 0 };
			
			if ( not is_supporting_extensions( physical_device ) ) [[unlikely]]
				spdlog::info( "... device extension support: insufficient!" );
			else if ( features.geometryShader == VK_FALSE ) [[unlikely]]
				spdlog::info( "... geometry shader support: false" );
			else {
				spdlog::info( "... swapchain support: true" );
				spdlog::info( "... geometry shader support: true" );
				score += properties.limits.maxImageDimension2D;
				if ( properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ) [[likely]] {
					spdlog::info( "... type: discrete" );
					score += 10'000;
				}
				else [[unlikely]] {
					spdlog::info( "... type: integrated" );
				}
				// TODO: add more things if needed
				spdlog::info( "... final score: {}", score );
			}
			return score;
		} // end-of-function: gfx::<unnamed>::score_physical_device
		
		[[nodiscard]] auto
		select_physical_device(
			VkInstance const &vk_instance
		)
		{
			spdlog::info( "Selecting the most suitable physical device..." );
			vk::raii::PhysicalDevices physical_devices( vk_instance.get_instance() );
			spdlog::info( "... found {} physical device(s)...", physical_devices.size() );
			if ( physical_devices.empty() ) [[unlikely]]
				throw std::runtime_error { "Unable to find any physical devices!" };
			
			auto *p_best_match {
				&physical_devices.front()
			};
			
			auto best_score {
				score_physical_device( *p_best_match )
			};
			
			for ( auto &current_physical_device: physical_devices | std::views::drop(1) ) {
				auto const score {
					score_physical_device( current_physical_device )
				};
				if ( score > best_score) {
					best_score   =  score;
					p_best_match = &current_physical_device;
				}
			}
			
			if ( best_score > 0 ) [[likely]] {
				spdlog::info(
					"... selected physical device `{}` with a final score of: {}",
					p_best_match->getProperties().deviceName.data(), best_score
				);
				return std::make_unique<vk::raii::PhysicalDevice>( std::move( *p_best_match ) );
			}
			else [[unlikely]] throw std::runtime_error { "Physical device does not support swapchains!" };
		} // end-of-function: gfx::<unnamed>::select_physical_device
		
		#if !defined( NDEBUG )
			VKAPI_ATTR VkBool32 VKAPI_CALL
			debug_callback(
				VkDebugUtilsMessageSeverityFlagBitsEXT      severity_flags,
				VkDebugUtilsMessageTypeFlagsEXT             type_flags,
				VkDebugUtilsMessengerCallbackDataEXT const *callback_data_p,
				void * // unused for now
			)
			{
				// TODO: replace invocation duplication by using a function map?
				auto const  msg_type {
					vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(type_flags) )
				};
				auto const  msg_severity {
					static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity_flags)
				};
				auto const  msg_id        { callback_data_p->messageIdNumber };
				auto const &msg_id_name_p { callback_data_p->pMessageIdName  };
				auto const &msg_p         { callback_data_p->pMessage        };
				switch (msg_severity) {
					[[unlikely]] case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
						spdlog::error( "[{}] {} (#{}): {}", msg_type, msg_id_name_p, msg_id, msg_p );
						break;
					}
					[[likely]] case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
						spdlog::info( "[{}] {} (#{}): {}", msg_type, msg_id_name_p, msg_id, msg_p );
						break;
					}
					case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: { // TODO: find better fit?
						spdlog::info( "[{}] {} (#{}): {}", msg_type, msg_id_name_p, msg_id, msg_p );
						break;
					}
					[[unlikely]] case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
						spdlog::warn( "[{}] {} (#{}): {}", msg_type, msg_id_name_p, msg_id, msg_p );
						break;
					}
				}
				// TODO: expand with more info from callback_data_p
				return false; // TODO: why?
			} // end-of-function: gfx::<unnamed>::debug_callback
		#endif	
		
		#if !defined( NDEBUG )
			[[nodiscard]] auto
			make_debug_messenger(
				vk::raii::Context  const &vk_context,
				vk::raii::Instance const &vk_instance
			)
			{
				spdlog::info( "Creating debug messenger..." );
				
				auto const extension_properties {
					vk_context.enumerateInstanceExtensionProperties()
				};
				
				// look for debug utils extension:
				auto const search_result {
					std::find_if(
						std::begin( extension_properties ),
						std::end(   extension_properties ),
						[]( auto const &extension_properties ) {
							return std::strcmp( extension_properties.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0;
						}
					)
				};
				if ( search_result == std::end( extension_properties ) ) [[unlikely]]
					throw std::runtime_error { "Could not find " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " extension!" };
				
				vk::DebugUtilsMessageSeverityFlagsEXT const severity_flags {
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
				};
				
				vk::DebugUtilsMessageTypeFlagsEXT const type_flags {
					vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral     |
					vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
					vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
				};
				
				return std::make_unique<vk::raii::DebugUtilsMessengerEXT>(
					vk_instance,
					vk::DebugUtilsMessengerCreateInfoEXT {
						.messageSeverity =  severity_flags,
						.messageType     =  type_flags,
						.pfnUserCallback = &debug_callback
					}
				);
			} // end-of-function: gfx::<unnamed>::make_debug_messenger
		#endif
		
		[[nodiscard]] auto
		select_queue_family_indices(
			vk::raii::PhysicalDevice const &physical_device,
			Window                   const &window
		)
		{
			std::optional<u32> maybe_present_index  {};
			std::optional<u32> maybe_graphics_index {};
			auto const &queue_family_properties {
				physical_device.getQueueFamilyProperties()
			};
			spdlog::info( "Searching for graphics queue family..." );
			for ( u32 index{0}; index<std::size(queue_family_properties); ++index ) {
				bool const supports_graphics {
					queue_family_properties[index].queueFlags & vk::QueueFlagBits::eGraphics
				};
				bool const supports_present {
					physical_device.getSurfaceSupportKHR( index, *window.get_surface() ) == VK_TRUE
				};
				
				spdlog::info( "... Evaluating queue family index {}:", index );
				spdlog::info( "    ...  present support: {}", supports_present  ? "OK!" : "missing!" );
				spdlog::info( "    ... graphics support: {}", supports_graphics ? "OK!" : "missing!" );
				if ( supports_graphics and supports_present ) {
					maybe_present_index  = index;
					maybe_graphics_index = index;
					break;
				}
				else if ( supports_graphics )
					maybe_graphics_index = index;
				else if ( supports_present )
					maybe_present_index = index;
			}
			if ( maybe_present_index.has_value() and maybe_graphics_index.has_value() ) [[likely]] {
				bool const are_separate {
					maybe_present_index.value() != maybe_graphics_index.value()
				};
				if ( are_separate ) [[unlikely]]
					spdlog::info( "... selected different queue families for graphics and present." );
				else
					spdlog::info( "... ideal queue family was found!" );
				return QueueFamilyIndices {
					.present      = maybe_present_index.value(),
					.graphics     = maybe_graphics_index.value(),
					.are_separate = are_separate
				};
			}
			else [[unlikely]] throw std::runtime_error { "Queue family support for either graphics or present missing!" };
		} // end-of-function: gfx::<unnamed>::select_queue_family_indices
		
		[[nodiscard]] auto
		make_logical_device(
			vk::raii::PhysicalDevice const &physical_device,
			QueueFamilyIndices       const &queue_family_indices
		)
		{
			spdlog::info( "Creating logical device..." );
			
			// TODO: refactor approach when more queues are needed
			std::vector<vk::DeviceQueueCreateInfo> queue_create_infos {};
			
			f32 const present_queue_priority  { 1.0f }; // TODO: verify value
			f32 const graphics_queue_priority { 1.0f }; // TODO: verify value
			
			queue_create_infos.push_back(
				vk::DeviceQueueCreateInfo {
					.queueFamilyIndex =  queue_family_indices.present,
					.queueCount       =  1,
					.pQueuePriorities = &present_queue_priority
				}
			);
			
			if ( queue_family_indices.are_separate ) [[unlikely]] {
				queue_create_infos.push_back(
					vk::DeviceQueueCreateInfo {
						.queueFamilyIndex =  queue_family_indices.graphics,
						.queueCount       =  1,
						.pQueuePriorities = &graphics_queue_priority
					}
				);
			}
			
			return std::make_unique<vk::raii::Device>(
				physical_device,
				vk::DeviceCreateInfo {
					.queueCreateInfoCount    = static_cast<u32>( queue_create_infos.size() ),
					.pQueueCreateInfos       = queue_create_infos.data(),
					.enabledLayerCount       = 0,       // no longer used; TODO: add conditionally to support older versions?
					.ppEnabledLayerNames     = nullptr, // ^ ditto
					.enabledExtensionCount   = static_cast<u32>( required_device_extensions.size() ),
					.ppEnabledExtensionNames = required_device_extensions.data()
				}
			);
		} // end-of-function: gfx::<unnamed>::make_logical_device
		
		[[nodiscard]] auto
		make_queue(
			vk::raii::Device const &logical_device,
			u32              const  queue_family_index,
			u32              const  queue_index
		)
		{ // TODO: refactor away third arg and use automated tracking + exceptions
			spdlog::info( "Creating handle for queue #{} of queue family #{}...", queue_index, queue_family_index );
			return std::make_unique<vk::raii::Queue>(
				logical_device.getQueue( queue_family_index, queue_index )
			);
		} // end-of-function: gfx::<unnamed>::make_queue
		
		[[nodiscard]] auto
		make_command_pool(
			vk::raii::Device const &logical_device,
			u32              const  graphics_queue_family_index
		)
		{
			spdlog::info( "Creating command buffer pool..." );
			return std::make_unique<vk::raii::CommandPool>(
				logical_device,
				vk::CommandPoolCreateInfo {
					// NOTE: Flags can be set here to optimize for lifetime or enable resetability.
					//       Also, one pool would be needed for each queue family (if ever extended).
					.queueFamilyIndex = graphics_queue_family_index
				}
			);
		} // end-of-function: gfx::<unnamed>::make_command_pool
		
		[[nodiscard]] auto
		make_command_buffers(
			vk::raii::Device       const &logical_device,
			vk::raii::CommandPool  const &command_pool,
			vk::CommandBufferLevel const  level,
			u32                    const  command_buffer_count
		)
		{
			spdlog::info( "Creating {} command buffer(s)...", command_buffer_count );
			return std::make_unique<vk::raii::CommandBuffers>(
				logical_device,
				vk::CommandBufferAllocateInfo {
					.commandPool        = *command_pool,
					.level              =  level,
					.commandBufferCount =  command_buffer_count
				}
			);
		} // end-of-function: gfx::<unnamed>::make_command_buffers
		
		void
		recreate_swapchain(
			std::unique_ptr<Swapchain>     &p_swapchain,
			vk::raii::PhysicalDevice const &physical_device,
			vk::raii::Device         const &logical_device,
			Window                   const &window,
			QueueFamilyIndices       const &queue_family_indices
		)
		{
			spdlog::debug( "Remaking swapchain!" );
			p_swapchain.reset( nullptr );
			std::make_unique<Swapchain>(
				physical_device, logical_device, window, queue_family_indices
			);	
		} // end-of-function: gfx::<unnamed>::recreate_swapchain
	} // end-of-unnamed-namespace
	
	Renderer::Renderer()
	{
		spdlog::info( "Constructing a Renderer instance..." );
		m_should_remake_swapchain = false;
		m_p_glfw_instance         = std::make_unique<GlfwInstance>();
		m_p_vk_instance           = std::make_unique<VkInstance>( *m_p_glfw_instance );
		#if !defined( NDEBUG )
		//	m_p_debug_messenger    = std::make_unique<DebugMessenger>( *m_p_vk_instance ); // TODO: make into its own class
		#endif
		m_p_window                = std::make_unique<Window>( *m_p_glfw_instance, *m_p_vk_instance, m_should_remake_swapchain );
// TODO: refactor block below   
		m_p_physical_device       = select_physical_device( *m_p_vk_instance );
		m_queue_family_indices    = select_queue_family_indices( *m_p_physical_device, *m_p_window );
		m_p_logical_device        = make_logical_device( *m_p_physical_device, m_queue_family_indices );
		m_p_graphics_queue        = make_queue( *m_p_logical_device, m_queue_family_indices.graphics, 0 ); // NOTE: 0 since we
		m_p_present_queue         = make_queue( *m_p_logical_device, m_queue_family_indices.present,  0 ); // only use 1 queue
// TODO: refactor block above   
		m_p_swapchain             = std::make_unique<Swapchain>( *m_p_physical_device, *m_p_logical_device, *m_p_window, m_queue_family_indices ); // TODO: refactor params
		m_p_pipeline              = std::make_unique<Pipeline>( *m_p_logical_device, *m_p_swapchain );
		m_p_framebuffers          = std::make_unique<Framebuffers>( *m_p_logical_device, *m_p_swapchain, *m_p_pipeline );
// TODO: refactor block below
		m_p_command_pool          = make_command_pool( *m_p_logical_device, m_queue_family_indices.graphics );
		m_p_command_buffers       = make_command_buffers(
			*m_p_logical_device,
			*m_p_command_pool,
			 vk::CommandBufferLevel::ePrimary,
			 m_p_swapchain->get_image_views().size() // one per framebuffer frame
		);
		
		vk::ClearValue const clear_value {
			.color = {{{ 0.02f, 0.02f, 0.02f, 1.0f }}}
		};
		
		for ( u32 index{0}; index < m_p_swapchain->get_image_views().size(); ++index ) {
			auto &command_buffer = (*m_p_command_buffers)[index];
			command_buffer.begin( {} );
			command_buffer.beginRenderPass(
				vk::RenderPassBeginInfo {
					.renderPass      = *( m_p_pipeline->get_render_pass()   ),
					.framebuffer     = *( m_p_framebuffers->access()[index] ),
					.renderArea      =  vk::Rect2D {
												 .extent = m_p_swapchain->get_surface_extent(),
										  },
					.clearValueCount =  1, // TODO: explain
					.pClearValues    = &clear_value
				},
				vk::SubpassContents::eInline
			);
			command_buffer.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				*( m_p_pipeline->access() )
			);
			//command_buffer.bindDescriptorSets()
			//command_buffer.setViewport()
			//command_buffer.setScissor()
			command_buffer.draw(
				3, // vertex count
				1, // instance count
				0, // first vertex
				0  // first instance
			);
			command_buffer.endRenderPass();
			command_buffer.end();
		}

#if 0 // TODO:
	// initialize a vk::RenderPassBeginInfo with the current imageIndex and some appropriate renderArea and clearValues
	vk::RenderPassBeginInfo renderPassBeginInfo( *renderPass, *framebuffers[imageIndex], renderArea, clearValues );

	// begin the render pass with an inlined subpass; no secondary command buffers allowed
	commandBuffer.beginRenderPass( renderPassBeginInfo, vk::SubpassContents::eInline );

	// bind the graphics pipeline
	commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *graphicsPipeline );

	// bind an appropriate descriptor set
	commandBuffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, { *descriptorSet }, nullptr );

	// bind the vertex buffer
	commandBuffer.bindVertexBuffers( 0, { *vertexBuffer }, { 0 } );

	// set viewport and scissor
	commandBuffer.setViewport( 0, viewport );
	commandBuffer.setScissor( renderArea );

	// draw the 12 * 3 vertices once, starting with vertex 0 and instance 0
	commandBuffer.draw( 12 * 3, 1, 0, 0 );

	// end the render pass and stop recording
	commandBuffer.endRenderPass();
	commandBuffer.end();
#endif
	} // end-of-function: gfx::Renderer::Renderer
	
	Renderer::Renderer( Renderer &&other ) noexcept:
		m_p_glfw_instance      { std::move( other.m_p_glfw_instance      ) },
		m_p_vk_instance        { std::move( other.m_p_vk_instance        ) },
		#if !defined( NDEBUG )
		m_p_debug_messenger    { std::move( other.m_p_debug_messenger    ) },
		#endif
		m_p_window             { std::move( other.m_p_window             ) },
		m_p_physical_device    { std::move( other.m_p_physical_device    ) },
		m_queue_family_indices { std::move( other.m_queue_family_indices ) },
		m_p_logical_device     { std::move( other.m_p_logical_device     ) },
		m_p_graphics_queue     { std::move( other.m_p_graphics_queue     ) },
		m_p_present_queue      { std::move( other.m_p_present_queue      ) },
		m_p_command_pool       { std::move( other.m_p_command_pool       ) },
		m_p_command_buffers    { std::move( other.m_p_command_buffers    ) },
		m_p_framebuffers       { std::move( other.m_p_framebuffers       ) },
		m_p_swapchain          { std::move( other.m_p_swapchain          ) },
		m_p_pipeline           { std::move( other.m_p_pipeline           ) }
	{
		spdlog::info( "Moving a Renderer instance..." );
	} // end-of-function: gfx::Renderer::Renderer
	
	Renderer::~Renderer() noexcept
	{
		spdlog::info( "Destroying a Renderer instance..." );
	} // end-of-function: gfx::Renderer::~Renderer
	
	[[nodiscard]] Window const &
	Renderer::get_window() const
	{
		return *m_p_window;
	} // end-of-function: gfx::Renderer::get_window
	
	[[nodiscard]] Window &
	Renderer::get_window()
	{
		return *m_p_window;
	} // end-of-function: gfx::Renderer::get_window
	
	void
	Renderer::operator()()
	{
		#define TIMEOUT 5000 // TODO!
		
		vk::raii::Semaphore const image_acquired_semaphore(
			*m_p_logical_device,
			vk::SemaphoreCreateInfo{}
		);
		
		auto const [acquire_result, image_index] {
			m_p_swapchain->access().acquireNextImage(
				TIMEOUT,
				*image_acquired_semaphore
			)
		};
		
		switch ( acquire_result ) {
			case vk::Result::eTimeout           : throw "TODO"; break; // TODO
			case vk::Result::eNotReady          : throw "TODO"; break; // TODO
			case vk::Result::eSuboptimalKHR     : [[fallthrough]];
			case vk::Result::eErrorOutOfDateKHR : {
				m_should_remake_swapchain = false;
				recreate_swapchain(
					m_p_swapchain,
					*m_p_physical_device,
					*m_p_logical_device,
					*m_p_window,
					m_queue_family_indices
				);
				return;
			}
			default: break; // do nothing
		}
		
		vk::raii::Fence const fence( *m_p_logical_device, vk::FenceCreateInfo{} );
		
		vk::PipelineStageFlags const wait_destination_stage_mask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput
		);
		
		m_p_graphics_queue->submit(
			vk::SubmitInfo {
				.pWaitDstStageMask  = &wait_destination_stage_mask,
				.commandBufferCount =  1,
				.pCommandBuffers    = &*(*m_p_command_buffers)[image_index],
				.pSignalSemaphores  = &*image_acquired_semaphore
			},
			*fence
		);
		
		while ( m_p_logical_device->waitForFences( { *fence }, VK_TRUE, TIMEOUT ) == vk::Result::eTimeout );
		
		auto const present_result {
			m_p_present_queue->presentKHR(
				vk::PresentInfoKHR {
					.waitSemaphoreCount = 0,
					.pWaitSemaphores    = nullptr, // TODO: comment
					.swapchainCount     = 1,
					.pSwapchains        = &*(m_p_swapchain->access()),
					.pImageIndices      = &image_index,
				}
			)
		};
		
		switch ( present_result ) {
			case vk::Result::eErrorOutOfDateKHR : [[fallthrough]];
			case vk::Result::eSuboptimalKHR     : {
				m_should_remake_swapchain = false;
				recreate_swapchain(
					m_p_swapchain,
					*m_p_physical_device,
					*m_p_logical_device,
					*m_p_window,
					m_queue_family_indices
				);
				break;
			}
			case vk::Result::eSuccess : break;
			default: throw "TODO";
		}

		if ( m_should_remake_swapchain ) {
			m_should_remake_swapchain = false;
			recreate_swapchain(
				m_p_swapchain,
				*m_p_physical_device,
				*m_p_logical_device,
				*m_p_window,
				m_queue_family_indices
			);
		}
	} // end-of-function: gfx::Renderer::operator()
} // end-of-namespace: gfx
// EOF
