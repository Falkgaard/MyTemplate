#include "MyTemplate/Renderer/Renderer.hpp"
#include "MyTemplate/Renderer/common.hpp"	
#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include "MyTemplate/Renderer/Window.hpp"

#include <spdlog/spdlog.h>

#include <ranges>
#include <algorithm>
#include <optional>
#include <array>
#include <vector>
#include <set>
#include <memory>
#include <cassert>

namespace gfx {	
	namespace { // private (file-scope)
		// TODO: refactor
		u32        constexpr gMaxConcurrentFrames        { 2 };
		std::array constexpr gRequiredDeviceExtensions   { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::array constexpr gRequiredValidationLayers   { "VK_LAYER_KHRONOS_validation"   };
		std::array constexpr gRequiredInstanceExtensions {
			#if !defined( NDEBUG )
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME ,
				VK_EXT_DEBUG_REPORT_EXTENSION_NAME
			#endif
		};
	} // end-of-unnamed-namespace	
	
/////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		
		// TODO: refactor common code shared by enableValidationLayers and enableInstanceExtensions.
		void
		Renderer::enableValidationLayers( vk::InstanceCreateInfo &createInfo )
		{
			spdlog::info( "... enabling validation layers:" );
			
			// pre-condition(s):
			//   shouldn't be null unless the function is called in the wrong order:
			assert( mpVkContext != nullptr );
			
			auto const availableLayers { mpVkContext->enumerateInstanceLayerProperties() };
			
			// print all required and available layers:
			for ( auto const &requiredLayer: gRequiredValidationLayers )
				spdlog::info( "      required  : `{}`", requiredLayer );
			for ( auto const &availableLayer: availableLayers )
				spdlog::info( "      available : `{}`", availableLayer.layerName );
			
			// ensure that all required layers are available:
			bool isMissingLayer { false };
			for ( auto const &requiredLayer: gRequiredValidationLayers ) {
				bool foundMatch { false };
				for ( auto const &availableLayer: availableLayers ) {
					if ( std::strcmp( requiredLayer, availableLayer.layerName ) == 0 ) [[unlikely]] {
						foundMatch = true;
						mValidationLayers.push_back( requiredLayer );
						break;
					}
				}
				if ( not foundMatch ) [[unlikely]] {
					isMissingLayer = true;
					spdlog::error( "    ! MISSING   : `{}`!", requiredLayer );
				}
			}
			
			// handle success or failure:
			if ( isMissingLayer ) [[unlikely]]
				throw std::runtime_error { "Failed to load required validation layers!" };
		} // end-of-function: Renderer::enableValidationLayers
		
		
		
		// TODO: refactor common code shared by enableValidationLayers and enableInstanceExtensions.
		void
		Renderer::enableInstanceExtensions( vk::InstanceCreateInfo &createInfo )
		{
			spdlog::info( "... enabling instance extensions:" );
			
			// pre-condition(s):
			//   shouldn't be null unless the function is called in the wrong order:
			assert( mpGlfwInstance != nullptr      );
			assert( mpVkContext    != nullptr      );
			//   should be empty unless the function has been called multiple times (which it shouldn't)
			assert( mInstanceExtensions.is_empty() );
			
			auto const availableExtensions    { mpVkContext->enumerateInstanceExtensionProperties() };
			auto const glfwRequiredExtensions { mpGlfwInstance->getRequiredExtensions()             };
			
			// make a union of all instance extensions requirements:
			std::set<char const *> allRequiredExtensions {};
			for ( auto const &requiredExtension: glfwRequiredExtensions )
				allRequiredExtensions.insert( requiredExtension );
			for ( auto const &requiredExtension: gRequiredInstanceExtensions )
				allRequiredExtensions.insert( requiredExtension );
			
			if constexpr ( gIsDebugMode ) {
				// print required and available instance extensions:
				for ( auto const &requiredExtension: allRequiredExtensions )
					spdlog::info( "      required  : `{}`", requiredExtension );
				for ( auto const &availableExtension: availableExtensions )
					spdlog::info( "      available : `{}`", availableExtension.extensionName );
			}
			
			// ensure required instance extensions are available:
			bool isAdequate { true }; // assume true until proven otherwise
			for ( auto const &requiredExtension: allRequiredExtensions ) {
				bool isSupported { false }; // assume false until found
				for ( auto const &availableExtension: availableExtensions ) {
					if ( std::strcmp( requiredExtension, availableExtension.extensionName ) == 0 ) [[unlikely]] {
						isSupported = true;
						mInstanceExtensions.push_back( requiredExtension );
						break; // early exit
					}
				}
				if ( not isSupported ) [[unlikely]] {
					isAdequate = false;
					spdlog::error( "   ! MISSING   : `{}`", requiredExtension );
				}
			}
			
			if ( not isAdequate ) [[unlikely]]
				throw std::runtime_error { "Failed to load required instance extensions!" };
		} // end-of-function: Renderer::enableInstanceExtensions
		
		
		
		[[nodiscard]] bool
		Renderer::meetsDeviceExtensionRequirements( vk::raii::PhysicalDevice const &physicalDevice )
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
			
			bool isAdequate { true }; // assume true until proven otherwise
			for ( auto const &requiredExtension: gRequiredDeviceExtensions ) { // TODO: make gRequiredDeviceExtensions a member
				bool isSupported { false }; // assume false until found
				for ( auto const &availableExtension: availableExtensions ) {
					if ( std::strcmp( requiredExtension, availableExtension.extensionName ) == 0 ) [[unlikely]] {
						isSupported = true;
						break; // early exit
					}
				}
				spdlog::info(
					"      support for required device extension `{}`: {}",
					requiredExtension, bIsSupported ? "OK" : "missing!"
				);
				if ( not bIsSupported ) [[unlikely]]
					isAdequate = false;
			}
			return isAdequate;
		} // end-of-function: Renderer::meetsDeviceExtensionRequirements
		
		
		
		[[nodiscard]] u32
		Renderer::calculateScore( vk::raii::PhysicalDevice const &physicalDevice )
		{
			spdlog::info( "Scoring physical device `{}`...", properties.deviceName.data() );
			
			auto const properties { physicalDevice.getProperties() };
			auto const features   { physicalDevice.getFeatures()   };
			
			u32 score { 0 };
			
			if ( not meetsDeviceExtensionRequirements( physicalDevice ) ) [[unlikely]]
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
				// TODO: add additional score factors here later on (if needed)
				spdlog::info( "... final score: {}", score );
			}
			return score;
		} // end-of-function: Renderer::calculateScore
		
		
		
		void
		Renderer::selectPhysicalDevice()
		{
			spdlog::info( "Selecting the most suitable physical device..." );
			
			// pre-condition(s):
			//   shouldn't be null unless the function is called in the wrong order:
			assert( mpVkContext      != nullptr );
			assert( mpVkInstance     != nullptr );
			//   should be null unless the function has been called multiple times (which it shouldn't)
			assert( mpPhysicalDevice == nullptr );
			
			vk::raii::PhysicalDevices physicalDevices( *mpVkInstance );
			spdlog::info( "... found {} physical device(s)...", physicalDevices.size() );
			if ( physicalDevices.empty() ) [[unlikely]]
				throw std::runtime_error { "Unable to find any physical devices!" };
			
			auto *pBestMatch { &physicalDevices.front()       };
			auto  bestScore  {  calculateScore( *pBestMatch ) };
			
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
				mpPhysicalDevice = std::make_unique<vk::raii::PhysicalDevice>( std::move( *pBestMatch ) );
			}
			else [[unlikely]] throw std::runtime_error { "Physical device does not support swapchains!" };
		} // end-of-function: Renderer::selectPhysicalDevice
		
		

		#if !defined( NDEBUG )
			VKAPI_ATTR VkBool32 VKAPI_CALL
			Renderer::debugCallback(
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
			} // end-of-function: Renderer::debugCallback
		#endif	
		
		
		
		#if !defined( NDEBUG )
			void
			Renderer::makeDebugMessenger()
			{
				spdlog::info( "Creating debug messenger..." );
				
				// pre-condition(s):
				//   shouldn't be null unless the function is called in the wrong order:
				assert( mpVkContext      != nullptr );
				assert( mpVkInstance     != nullptr );
				//   should be null unless the function has been called multiple times (which it shouldn't)
				assert( mpDebugMessenger == nullptr );
				
				auto const extensionProperties { mpVkContext->enumerateInstanceExtensionProperties() };
				
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
				
				mpDebugMessenger = std::make_unique<vk::raii::DebugUtilsMessengerEXT>(
					*mpVkInstance,
					vk::DebugUtilsMessengerCreateInfoEXT {
						.messageSeverity =  severityFlags,
						.messageType     =  typeFlags,
						.pfnUserCallback = &debugCallback
					}
				);
			} // end-of-function: Renderer::makeDebugMessenger
		#endif
		
		
		
		void
		Renderer::selectQueueFamilies()
		{
			spdlog::info( "Selecting queue families..." );
			
			// pre-condition(s):
			//   shouldn't be null unless the function is called in the wrong order:
			assert( mpWindow         != nullptr );
			assert( mpPhysicalDevice != nullptr );
			//   should be undefined unless the function has been called multiple times (which it shouldn't)
			assert( mQueueFamilyIndices.presentIndex  == QueueFamilyIndices::kUndefined );
			assert( mQueueFamilyIndices.graphicsIndex == QueueFamilyIndices::kUndefined );
			
			std::optional<u32> maybePresentIndex  {};
			std::optional<u32> maybeGraphicsIndex {};
			auto const &queueFamilyProperties { mpPhysicalDevice->getQueueFamilyProperties() };
			
			spdlog::info( "Searching for graphics queue family..." );
			for ( u32 index{0};  index < queueFamilyProperties.size();  ++index ) {
				bool const supportsGraphics { queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics };
				bool const supportsPresent  { physical_device.getSurfaceSupportKHR( index, *mpWindow->get_surface() ) == VK_TRUE };
				
				spdlog::info( "... Evaluating queue family index {}:", index );
				spdlog::info( "    ...  present support: {}", supportsPresent  ? "OK!" : "missing!" );
				spdlog::info( "    ... graphics support: {}", supportsGraphics ? "OK!" : "missing!" );
				if ( supportsGraphics and supportsPresent ) {
					maybePresentIndex  = index;
					maybeGraphicsIndex = index;
					break;
				}
				else if ( supportsGraphics )
					maybeGraphicsIndex = index;
				else if ( supportsPresent )
					maybePresentIndex  = index;
			}
			
			if ( maybePresentIndex.has_value() and maybeGraphicsIndex.has_value() ) [[likely]] {
				mQueueFamilies = QueueFamilyIndices {
					.presentIndex  = maybePresentIndex.value(),
					.graphicsIndex = maybeGraphicsIndex.value(),
					.areSeparate   = maybePresentIndex.value() != maybeGraphicsIndex.value()
				};
				if ( mQueueFamilyIndices.areSeparate ) [[unlikely]]
					spdlog::info( "... selected different queue families for graphics and present." );
				else [[likely]]
					spdlog::info( "... ideal queue family was found!" );
			}
			else [[unlikely]] throw std::runtime_error { "Queue family support for either graphics or present missing!" };
		} // end-of-function: Renderer::select_queue_family_indices
		
		
		
		void
		Renderer::makeLogicalDevice()
		{
			spdlog::info( "Creating logical device..." );
			
			// pre-condition(s):
			//   shouldn't be undefined and null unless the function is called in the wrong order:
			assert( mQueueFamilyIndices.presentIndex  != QueueFamilyIndices::kUndefined );
			assert( mQueueFamilyIndices.graphicsIndex != QueueFamilyIndices::kUndefined );
			assert( mpPhysicalDevice                  != nullptr                        );
			//   should be null unless the function has been called multiple times (which it shouldn't):
			assert( mpDevice                          == nullptr                        ); 
			
			// TODO: refactor approach when more queues are needed
			std::vector<vk::DeviceQueueCreateInfo> createInfos {};
			
			f32 const presentQueuePriority  { 1.0f }; // TODO: verify value
			f32 const graphicsQueuePriority { 1.0f }; // TODO: verify value
			
			createInfos.push_back(
				vk::DeviceQueueCreateInfo {
					.queueFamilyIndex =  mQueueFamilies.presentIndex
					.queueCount       =  1,
					.pQueuePriorities = &presentQueuePriority
				}
			);
			
			if ( mQueueFamilies.areSeparate ) [[unlikely]] {
				createInfos.push_back(
					vk::DeviceQueueCreateInfo {
						.queueFamilyIndex =  mQueueFamilies.graphicsIndex,
						.queueCount       =  1,
						.pQueuePriorities = &graphicsQueuePriority
					}
				);
			}
			
			return std::make_unique<vk::raii::Device>(
				physical_device,
				vk::DeviceCreateInfo {
					.queueCreateInfoCount    = static_cast<u32>( createInfos.size() ),
					.pQueueCreateInfos       = createInfos.data(),
					.enabledLayerCount       = 0,       // no longer used; TODO: add conditionally to support older versions?
					.ppEnabledLayerNames     = nullptr, // ^ ditto
					.enabledExtensionCount   = static_cast<u32>( gRequiredDeviceExtensions.size() ), // TODO?
					.ppEnabledExtensionNames = gRequiredDeviceExtensions.data()                      // TODO?
				}
			);
		} // end-of-function: Renderer::makeLogicalDevice
		
		
		
		// TODO: refactor away second arg and use automated tracking + exceptions
		[[nodiscard]] auto
		Renderer::makeQueue( u32 const queueFamilyIndex, u32 const queueIndex )
		{
			spdlog::info( "Creating handle for queue #{} of queue family #{}...", queueIndex, queueFamilyIndex );
			
			// pre-condition(s):
			//   shouldn't be null unless the function is called in the wrong order:
			assert( mpDevice != nullptr );
			
			return std::make_unique<vk::raii::Queue>( mpDevice->getQueue( queueFamilyIndex, queueIndex );
		} // end-of-function: Renderer::makeQueue
		
		
		
		void
		Renderer::makeGraphicsQueue()
		{
			// Just using one queue for now. TODO(1.0)
			spdlog::info( "Creating graphics queue..." );
			
			// pre-condition(s):
			//   should be null unless the function has been called multiple times (which it shouldn't):
			assert( mpGraphicsQueue == nullptr ); 
			
			mpGraphicsQueue = makeQueue( mQueueFamilyIndices.graphicsIndex, 0 );
		} // end-of-function: Renderer::makeGraphicsQueue
		
		
		
		void
		Renderer::makePresentQueue()
		{
			// Just using one queue for now. TODO(1.0)
			spdlog::info( "Creating present queue..." );
			
			// pre-condition(s):
			//	  should be null unless the function has been called multiple times (which it shouldn't):
			assert( mpPresentQueue == nullptr ); 
			
			mpGraphicsQueue = makeQueue( mQueueFamilyIndices.presentIndex, mQueueFamilyIndices.areSeparate ? 0 : 1 );
		} // end-of-function: Renderer::makePresentQueue
		
		
		
		void
		Renderer::makeCommandPool()
		{
			spdlog::info( "Creating command buffer pool..." );
			
			// pre-condition(s):
			//   shouldn't be null and undefined unless the function is called in the wrong order:
			assert( mpGraphicsQueue                   != nullptr                        ); 
			assert( mQueueFamilyIndices.graphicsIndex != QueueFamilyIndices::kUndefined ); 
			//	  should be null unless the function has been called multiple times (which it shouldn't):
			assert( mpCommandPool == nullptr ); 
			
			mpCommandPool = std::make_unique<vk::raii::CommandPool>(
				*mpDevice,
				vk::CommandPoolCreateInfo {
					// NOTE: Flags can be set here to optimize for lifetime or enable resetability.
					//       Also, one pool would be needed for each queue family (if ever extended).
					.queueFamilyIndex = mQueueFamilyIndices.graphicsIndex
				}
			);
		} // end-of-function: Renderer::makeCommandPool
		
		
		
		[[nodiscard]] auto
		makeCommandBuffers(
			vk::CommandBufferLevel const level,
			u32                    const bufferCount
		)
		{
			// TODO(refactor)
			spdlog::info( "Creating {} command buffer(s)...", bufferCount );
			return std::make_unique<vk::raii::CommandBuffers>(
				*mpDevice,
				vk::CommandBufferAllocateInfo {
					.commandPool        = **impCommandPool,
					.level              =   level,
					.commandBufferCount =   bufferCount
				}
			);
		} // end-of-function: Renderer::makeCommandBuffers()
		
		
		
		void
		makeSyncPrimitives()
		{
			// TODO(refactor)
			spdlog::info( "Creating synchronization primitives..." );
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
		







//instance
			createInfo.enabledLayerCount       = static_cast<u32>( mValidationLayers.size() );
			createInfo.ppEnabledLayerNames     = mValidationLayers.data();
			createInfo.enabledExtensionCount   = static_cast<u32>( mInstanceExtensions.size() );
			createInfo.ppEnabledExtensionNames = mInstanceExtensions.data();













		
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
