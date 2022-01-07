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
	struct Renderer::State final {
		// NOTE: declaration order here is very important!
		std::unique_ptr<GlfwInstance>                         pGlfwInstance     ;
		std::unique_ptr<VkInstance>                           pVkInstance       ;
		#if !defined( NDEBUG )
			std::unique_ptr<vk::raii::DebugUtilsMessengerEXT>  pDebugMessenger   ; // TODO: refactor into own class?
		#endif
		std::unique_ptr<Window>                               pWindow           ;
		std::unique_ptr<vk::raii::PhysicalDevice>             pPhysicalDevice   ; // TODO: refactor into LogicalDevice
		QueueFamilyIndices                                    queueFamilies     ; // TODO: refactor into LogicalDevice
		std::unique_ptr<vk::raii::Device>                     pDevice           ; // TODO: refactor into own class
		std::unique_ptr<vk::raii::Queue>                      pGraphicsQueue    ;
		std::unique_ptr<vk::raii::Queue>                      pPresentQueue     ;
		std::unique_ptr<vk::raii::CommandPool>                pCommandPool      ;
		// recreating part follows:
		std::unique_ptr<Swapchain>                            pSwapchain        ;
		std::unique_ptr<vk::raii::CommandBuffers>             pCommandBuffers   ; // NOTE: Must be deleted before command pool!
		std::unique_ptr<Pipeline>                             pGraphicsPipeline ;
		std::unique_ptr<Framebuffers>                         pFramebuffers     ; // NOTE: Must be deleted before swapchain!
		bool                                                  bBadSwapchain     ;
		// TODO: refactor into a struct of per-frame data (synchro, buffers, etc)
		std::vector<vk::raii::Semaphore>                      imageAvailable    ; // synchronization
		std::vector<vk::raii::Semaphore>                      imagePresentable  ; // synchronization
		std::vector<vk::raii::Fence>                          images_in_flight  ; // synchronization
		std::vector<vk::raii::Fence>                          fences_in_flight  ; // synchronization
		u64                                                   currentFrame      ;
	}; // end-of-struct: gfx::Renderer::State	
	
	namespace { // private (file-scope)
		u32 constexpr gMaxConcurrentFrames { 2 }; // TODO: refactor
		
		// TODO: refactor out
		std::array constexpr gRequiredDeviceExtensions {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		
		[[nodiscard]] bool
		meetsExtensionRequirements( vk::raii::PhysicalDevice const &physicalDevice )
		{
			// NOTE: all extensions in this function are device extensions
			auto const &availableExtensions { physicalDevice.enumerateDeviceExtensionProperties() };
			
			if constexpr ( gIsDebugMode ) {
				spdlog::info( "... checking device extension support:" );
				// print required and available device extensions:
				for ( auto const &requiredExtension: gRequiredDeviceExtensions )
					spdlog::info( "      required  : `{}`", requiredExtension );
				for ( auto const &availableExtension: availableExtensions )
					spdlog::info( "      available : `{}`", availableExtension.extensionName );
			}	
			
			bool bIsAdequate { true }; // assume true until proven otherwise
			for ( auto const &requiredExtension: gRequiredDeviceExtensions ) {
				bool bIsSupported { false }; // assume false until found
				for ( auto const &availableExtension: availableExtensions ) {
					if ( std::strcmp( requiredExtension, availableExtension.extensionName ) == 0 ) [[unlikely]] {
						bIsSupported = true;
						break; // early exit
					}
				}
				spdlog::info(
					"      support for required device extension `{}`: {}",
					requiredExtension, bIsSupported ? "OK" : "missing!"
				);
				if ( not bIsSupported ) [[unlikely]]
					bIsAdequate = false;
			}
			return bIsAdequate;
		} // end-of-function: gfx::<unnamed>::meetsExtensionRequirements
		
		[[nodiscard]] u32
		calculateScore( vk::raii::PhysicalDevice const &physicalDevice )
		{
			auto const properties { physicalDevice.getProperties() }; // TODO: KHR2?
			auto const features   { physicalDevice.getFeatures()   }; // TODO: 2KHR?
			spdlog::info( "Scoring physical device `{}`...", properties.deviceName.data() );
			u32 score { 0 };
			
			if ( not meetsExtensionRequirements( physicalDevice ) ) [[unlikely]]
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
				// TODO: add additional score factors later on (if needed)
				spdlog::info( "... final score: {}", score );
			}
			return score;
		} // end-of-function: gfx::<unnamed>::calculateScore
		
		// NOTE: pre-conditions(s): `pContext` and `pVkInstance` are valid.
		void Renderer::selectPhysicalDevice()
		{
			spdlog::info( "Selecting the most suitable physical device..." );
			vk::raii::PhysicalDevices physicalDevices( pVkInstance );
			spdlog::info( "... found {} physical device(s)...", physicalDevices.size() );
			if ( physicalDevices.empty() ) [[unlikely]]
				throw std::runtime_error { "Unable to find any physical devices!" };
			
			auto *pBestMatch { &physicalDevices.front() };
			auto  bestScore  {  score( *pBestMatch )    };
			
			for ( auto &currentPhysicalDevice: physicalDevices | std::views::drop(1) ) {
				auto const score { calculateScore( currentPhysicalDevice ) };
				if ( score > bestScore) {
					bestScore  =  score;
					pBestMatch = &currentPhysicalDevice;
				}
			}
			
			if ( bestScore > 0 ) [[likely]] {
				spdlog::info(
					"... selected physical device `{}` with a final score of: {}",
					pBestMatch->getProperties().deviceName.data(), bestScore
				);
				pPhysicalDevice = std::make_unique<vk::raii::PhysicalDevice>( std::move( *pBestMatch ) );
			}
			else [[unlikely]] throw std::runtime_error { "Physical device does not support swapchains!" };
		} // end-of-function: Renderer::selectPhysicalDevice
		
		#if !defined( NDEBUG )
			VKAPI_ATTR VkBool32 VKAPI_CALL
			debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT      msgSeverity,
				VkDebugUtilsMessageTypeFlagsEXT             msgType,
				VkDebugUtilsMessengerCallbackDataEXT const *fpCallbackData,
				void * // unused for now, TODO
			)
			{
				// TODO: replace invocation duplication by using a function map?
				auto const msgTypeName {
					vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(msgType) )
				};
				auto const msgSeverityLevel {
					static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(msgSeverity)
				};
				auto const  msgId      { fpCallbackData->messageIdNumber };
				auto const &pMsgIdName { fpCallbackData->pMessageIdName  };
				auto const &pMsg       { fpCallbackData->pMessage        };
				switch (msgSeverityLevel) {
					[[unlikely]] case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
						spdlog::error( "[{}] {} (#{}): {}", msgTypeName, pMsgIdName, msgId, pMsg );
						break;
					}
					[[likely]] case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
						spdlog::info( "[{}] {} (#{}): {}", msgTypeName, pMsgIdName, msgId, pMsg );
						break;
					}
					case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: { // TODO: find a better fit?
						spdlog::info( "[{}] {} (#{}): {}", msgTypeName, pMsgIdName, msgId, pMsg );
						break;
					}
					[[unlikely]] case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
						spdlog::warn( "[{}] {} (#{}): {}", msgTypeName, pMsgIdName, msgId, pMsg );
						break;
					}
				}
				// TODO: expand with more info from pCallbackData
				return false; // TODO: explain why
			} // end-of-function: gfx::<unnamed>::debugCallback
		#endif	
		
		#if !defined( NDEBUG )
			[[nodiscard]] auto
			// NOTE: pre-conditions(s): `state.pContext` and `state.pVkInstance` are valid.
			makeDebugMessenger( Render::State const &state )
			{
				spdlog::info( "Creating debug messenger..." );
				
				auto const extensionProperties { pContext->enumerateInstanceExtensionProperties() };
				
				// look for debug utils extension:
				auto const searchResultIterator {
					std::find_if(
						std::begin( extensionProperties ),
						std::end(   extensionProperties ),
						[]( auto const &extensionProperties ) {
							return std::strcmp( extensionProperties.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0;
						}
					)
				};
				if ( searchResultIterator == std::end( extensionProperties ) ) [[unlikely]]
					throw std::runtime_error { "Could not find " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " extension!" };
				
				vk::DebugUtilsMessageSeverityFlagsEXT const severityFlags {
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
				};
				
				vk::DebugUtilsMessageTypeFlagsEXT const typeFlags {
					vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral     |
					vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
					vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
				};
				
				return std::make_unique<vk::raii::DebugUtilsMessengerEXT>(
					*pVkInstance,
					vk::DebugUtilsMessengerCreateInfoEXT {
						.messageSeverity =  severityFlags,
						.messageType     =  typeFlags,
						.pfnUserCallback = &debugCallback
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
					.enabledExtensionCount   = static_cast<u32>( gRequiredDeviceExtensions.size() ),
					.ppEnabledExtensionNames = gRequiredDeviceExtensions.data()
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
			Renderer::State        const &state,
			vk::CommandBufferLevel const  level,
			u32                    const  command_buffer_count
		)
		{
			spdlog::info( "Creating {} command buffer(s)...", command_buffer_count );
			return std::make_unique<vk::raii::CommandBuffers>(
				*state.pDevice,
				vk::CommandBufferAllocateInfo {
					.commandPool        = **state.pCommandPool,
					.level              =   level,
					.commandBufferCount =   command_buffer_count
				}
			);
		} // end-of-function: gfx::<unnamed>::make_command_buffers
		
		void
		make_synch_primitives(
			Renderer::State &state
		)
		{
			spdlog::info( "Creating synchronization primitives..." );
			// TODO: refactor into array of struct?
			state.imagePresentable .clear();
			state.imageAvailable   .clear();
			state.fences_in_flight  .clear();
			state.images_in_flight  .clear();

			state.imagePresentable .reserve( gMaxConcurrentFrames );
			state.imageAvailable   .reserve( gMaxConcurrentFrames );
			state.fences_in_flight  .reserve( gMaxConcurrentFrames );
//			state.images_in_flight  .resize(
//				state.p_swapchain->get_image_views().size() // one per frame
//			);
			for ( auto i{0}; i<gMaxConcurrentFrames; ++i ) {
				state.imagePresentable .emplace_back( *state.pDevice, vk::SemaphoreCreateInfo {} );
				state.imageAvailable   .emplace_back( *state.pDevice, vk::SemaphoreCreateInfo {} );
				state.fences_in_flight  .emplace_back( *state.pDevice, vk::FenceCreateInfo     {} );
			}
		} // end-of-function: gfx::<unnamed>::make_synch_primitives
		
		void
		make_swapchain( Renderer::State &state )
		{
			spdlog::debug( "Creating swapchain and necessary state!" );
			
			state.pWindow->wait_for_resize();
			state.pDevice->waitIdle();
			
			// delete previous state (if any) in the right order:
			if ( state.pFramebuffers ) {
				state.pFramebuffers.reset();
			}
			if ( state.pCommandBuffers ) {
				state.pCommandBuffers->clear();
				state.pCommandBuffers.reset();
			}
			if ( state.pGraphicsPipeline ) {
				state.pGraphicsPipeline.reset();
		   }
			if ( state.pSwapchain ) {
				state.pSwapchain.reset();
			}
			
			state.pSwapchain = std::make_unique<Swapchain>(
				*state.pPhysicalDevice,
				*state.pDevice,
				*state.pWindow,
				 state.queueFamilies
			);
			
			state.pGraphicsPipeline = std::make_unique<Pipeline>(
				*state.pDevice,
				*state.pSwapchain
			);
			
			state.pFramebuffers = std::make_unique<Framebuffers>(
				*state.pDevice,
				*state.pSwapchain,
				*state.pGraphicsPipeline
			);
			
			state.pCommandBuffers = make_command_buffers(
				state,
				vk::CommandBufferLevel::ePrimary,
				state.pSwapchain->get_image_views().size() // one per framebuffer frame
			);
			
			// TODO: refactor
			vk::ClearValue const clear_value {
				.color = {{{ 0.02f, 0.02f, 0.02f, 1.0f }}}
			};
			
			for ( u32 index{0}; index < state.pSwapchain->get_image_views().size(); ++index ) {
				auto &command_buffer = (*state.pCommandBuffers)[index];
				command_buffer.begin( {} );
				command_buffer.beginRenderPass(
					vk::RenderPassBeginInfo {
						.renderPass      = *( state.pGraphicsPipeline->get_render_pass()   ),
						.framebuffer     = *( state.pFramebuffers->access()[index] ),
						.renderArea      = vk::Rect2D {
						                    .extent = state.pSwapchain->get_surface_extent(),
						                 },
						.clearValueCount =  1, // TODO: explain
						.pClearValues    = &clear_value
					},
					vk::SubpassContents::eInline
				);
				command_buffer.bindPipeline(
					vk::PipelineBindPoint::eGraphics,
					*( state.pGraphicsPipeline->access() )
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
		} // end-of-function: gfx::<unnamed>::recreate_swapchain
	} // end-of-unnamed-namespace
	
	Renderer::Renderer()
	{
		// INIT:
		//		Context
		//		Instances
		//		Window
		//		PhysicalDevice
		//		QueueFamilyIndices
		//		LogicalDevice
		//		Queues
		//		CommandBufferPool
		//	RECREATE:
		//		Swapchain
		//		ImageViews
		//		RenderPass
		//		GraphicsPipeline
		//		CommandBuffers
		spdlog::info( "Constructing a Renderer instance..." );
		pState = std::make_unique<Renderer::State>();
		pState->should_remake_swapchain = false;
		pState->frame_number            = 0;
		pState->p_glfw_instance         = std::make_unique<GlfwInstance>();
		pState->p_vk_instance           = std::make_unique<VkInstance>( *pState->p_glfw_instance );
		#if !defined( NDEBUG )
		//	m_p_debug_messenger    = std::make_unique<DebugMessenger>( *m_p_vk_instance ); // TODO: make into its own class
		#endif
		pState->p_window                = std::make_unique<Window>( *pState->p_glfw_instance, *pState->p_vk_instance, pState->should_remake_swapchain );
// TODO: refactor block below   
		pState->p_physical_device       = select_physical_device( *pState->p_vk_instance );
		pState->queue_family_indices    = select_queue_family_indices( *pState->p_physical_device, *pState->p_window );
		pState->p_logical_device        = make_logical_device( *pState->p_physical_device, pState->queue_family_indices );
		pState->p_graphics_queue        = make_queue( *pState->p_logical_device, pState->queue_family_indices.graphics, 0 ); // NOTE: 0 since we
		pState->p_present_queue         = make_queue( *pState->p_logical_device, pState->queue_family_indices.present,  0 ); // only use 1 queue
		pState->p_command_pool          = make_command_pool( *pState->p_logical_device, pState->queue_family_indices.graphics );
		make_swapchain( *pState );
		make_synch_primitives( *pState ); // TODO: re-run whenever swapchain image count changes!
   
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
		pState      { std::move( other.pState ) }
	{
		spdlog::info( "Moving a Renderer instance..." );
	} // end-of-function: gfx::Renderer::Renderer
	
	Renderer::~Renderer() noexcept
	{
		spdlog::info( "Destroying a Renderer instance..." );
		pState->p_logical_device->waitIdle();
		if ( pState and pState->p_command_buffers )
			pState->p_command_buffers->clear();
	} // end-of-function: gfx::Renderer::~Renderer
	
	[[nodiscard]] Window const &
	Renderer::get_window() const
	{
		return *pState->p_window;
	} // end-of-function: gfx::Renderer::get_window
	
	[[nodiscard]] Window &
	Renderer::get_window()
	{
		return *pState->p_window;
	} // end-of-function: gfx::Renderer::get_window
   
	void
	Renderer::operator()()
	{
		#define TIMEOUT 5000000 // TEMP! TODO // UINT64_MAX? (or limits)
		auto const frame = pState->frame_number % g_max_concurrent_frames;
//
//		if ( m_p_state->p_logical_device->waitForFences(
//				*(m_p_state->fences_in_flight[frame]),
//				VK_TRUE,
//				TIMEOUT
//			) != vk::Result::eSuccess // i.e. eTimeout
//		)                            // the rest will raise exceptions
//		{
//			// TODO: decide on what to do...
//			spdlog::warn( "Timeout in draw..." ); // TODO: better message
//		}
//		
		u32        acquired_index;
		vk::Result acquired_result;
		try {
			auto const [result, index] {
				pState->p_swapchain->access().acquireNextImage(
					TIMEOUT,
					*(pState->image_available[frame])
				)
			};
			acquired_index  = index;
			acquired_result = result;
		}
		catch( vk::OutOfDateKHRError const &e ) {}
//      // vk::Result::eSuccess
//      // vk::Result::eTimeout
//      // vk::Result::eNotReady
//      // vk::Result::eSuboptimalKHR
//		
//		if ( acquired_result == vk::Result::eSuboptimalKHR
//		or   acquired_result == vk::Result::eErrorOutOfDateKHR ) {
//			make_swapchain( *m_p_state );
//			return;
//		}
//		else if ( acquired_result != vk::Result::eSuccess)
//			throw std::runtime_error { "Failed to acquire swapchain image!" };
//		
//		if ( (m_p_state->images_in_flight[acquired_index]).getStatus )
//			if ( m_p_state->p_logical_device->waitForFences(
//					*m_p_state->images_in_flight[acquired_index],
//					VK_TRUE,
//					TIMEOUT
//				) != vk::Result::eSuccess // i.e. eTimeout
//			)                            // the rest will raise exceptions
//			{
//				// TODO: decide on what to do...
//				spdlog::warn( "Timeout in draw..." ); // TODO: better message
//			}
//
//		
		vk::PipelineStageFlags const wait_destination_stage_mask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput
		);
		
		pState->p_graphics_queue->submit(
			vk::SubmitInfo {
				.waitSemaphoreCount   = 1,
				.pWaitSemaphores      = &*pState->image_available[acquired_index],
				.pWaitDstStageMask    = &wait_destination_stage_mask,
				.commandBufferCount   = 1,
				.pCommandBuffers      = &*(*pState->p_command_buffers)[acquired_index],
				.signalSemaphoreCount = 1,
				.pSignalSemaphores    = &*pState->image_presentable[acquired_index], 
			} // `, *fence` here + wait loop later on VkSubpassDependency?
		);
//		
//		while (
//			m_p_state->p_logical_device->waitForFences(
//				{ *fence },
//				VK_TRUE,
//				TIMEOUT
//			) == vk::Result::eTimeout
//		);
//		
		vk::Result present_result;
		try {
			present_result = pState->p_present_queue->presentKHR(
				vk::PresentInfoKHR {
					.waitSemaphoreCount = 1,
					.pWaitSemaphores    = &*pState->image_presentable[acquired_index],
					.swapchainCount     = 1,
					.pSwapchains        = &*(pState->p_swapchain->access()),
					.pImageIndices      = &acquired_index,
				}
			);
		}
		// TODO find a way to avoid try-catch here
		catch ( vk::OutOfDateKHRError const &e) { /*do nothing*/ }
//		
//		if ( m_p_state->should_remake_swapchain
//		or   present_result==vk::Result::eSuboptimalKHR
//		or   present_result==vk::Result::eErrorOutOfDateKHR ) {
//			m_p_state->should_remake_swapchain = false;
//			make_swapchain( *m_p_state );
//		}
//		else if ( present_result != vk::Result::eSuccess )
//			throw std::runtime_error { "Failed to present swapchain image!" };
	} // end-of-function: gfx::Renderer::operator()
	
#if 0 // OLD
	void
	Renderer::operator()()
	{
		#define TIMEOUT 5000 // TEMP! TODO
		
		vk::raii::Semaphore const image_acquired_semaphore(
			*m_p_state->p_logical_device,
			vk::SemaphoreCreateInfo{}
		);
		
		u32        acquired_index;
		vk::Result acquired_result;
		try {
			auto const [result, index] {
				m_p_state->p_swapchain->access().acquireNextImage(
					TIMEOUT,
					*image_acquired_semaphore
				)
			};
			acquired_index  = index;
			acquired_result = result;
		}
		catch( vk::OutOfDateKHRError const &e ) {}
      // vk::Result::eSuccess
      // vk::Result::eTimeout
      // vk::Result::eNotReady
      // vk::Result::eSuboptimalKHR
		
		if ( acquired_result == vk::Result::eSuboptimalKHR
		or   acquired_result == vk::Result::eErrorOutOfDateKHR ) {
			make_swapchain( *m_p_state );
			return;
		}
		else if ( acquired_result != vk::Result::eSuccess)
			throw std::runtime_error { "Failed to acquire swapchain image!" };
		
		vk::raii::Fence const fence( *m_p_state->p_logical_device, vk::FenceCreateInfo{} );
		
		vk::PipelineStageFlags const wait_destination_stage_mask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput
		);
		
		m_p_state->p_graphics_queue->submit(
			vk::SubmitInfo {
				.pWaitDstStageMask  = &wait_destination_stage_mask,
				.commandBufferCount =  1,
				.pCommandBuffers    = &*(*m_p_state->p_command_buffers)[acquired_index],
				.pSignalSemaphores  = &*image_acquired_semaphore
			},
			*fence
		);
		
		while (
			m_p_state->p_logical_device->waitForFences(
				{ *fence },
				VK_TRUE,
				TIMEOUT
			) == vk::Result::eTimeout
		);
		
		vk::Result present_result;
		try {
			present_result = m_p_state->p_present_queue->presentKHR(
				vk::PresentInfoKHR {
					.waitSemaphoreCount = 0,
					.pWaitSemaphores    = nullptr, // TODO: comment
					.swapchainCount     = 1,
					.pSwapchains        = &*(m_p_state->p_swapchain->access()),
					.pImageIndices      = &acquired_index,
				}
			);
		}
		catch ( vk::OutOfDateKHRError const &e) {}
		
		if ( m_p_state->should_remake_swapchain
		or   present_result==vk::Result::eSuboptimalKHR
		or   present_result==vk::Result::eErrorOutOfDateKHR ) {
			m_p_state->should_remake_swapchain = false;
			make_swapchain( *m_p_state );
		}
		else if ( present_result != vk::Result::eSuccess )
			throw std::runtime_error { "Failed to present swapchain image!" };
	} // end-of-function: gfx::Renderer::operator()
#endif // end OLD
} // end-of-namespace: gfx
// EOF
