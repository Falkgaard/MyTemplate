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
		// TODO(config): refactor
		u32        constexpr gMaxConcurrentFrames        { 2 };
		std::array constexpr gRequiredDeviceExtensions   { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::array constexpr gRequiredValidationLayers   { "VK_LAYER_KHRONOS_validation"   };
		std::array constexpr gRequiredInstanceExtensions {
			#if !defined( NDEBUG )
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME ,
				VK_EXT_DEBUG_REPORT_EXTENSION_NAME  // TODO: obsolete?
			#endif
		};
		
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
			} // end-of-function: debugCallback
		#endif // end-of-debug-block
	} // end-of-unnamed-namespace	
	
/////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		
		// TODO: refactor common code shared by enableValidationLayers and enableInstanceExtensions.
		void
		Renderer::enableValidationLayers()
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
		Renderer::enableInstanceExtensions()
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
			else [[unlikely]]
				throw std::runtime_error { "Physical device does not support swapchains!" };
		} // end-of-function: Renderer::selectPhysicalDevice
		
		
		
		void
		Renderer::maybeMakeDebugMessenger()
		{
			#if !defined( NDEBUG )
				spdlog::info( "Creating debug messenger..." );
				
				// pre-condition(s):
				//   shouldn't be null unless the function is called in the wrong order:
				assert( mpVkContext      != nullptr );
				assert( mpVkInstance     != nullptr );
				//   should be null unless the function has been called multiple times (which it shouldn't)
				assert( mpDebugMessenger == nullptr );
				
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
			#endif
		} // end-of-function: Renderer::maybeMakeDebugMessenger
		
		
		
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
			else [[unlikely]]
				throw std::runtime_error { "Queue family support for either graphics or present missing!" };
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
					.enabledLayerCount       = 0,       // no longer used; TODO(support): add conditionally to support older versions?
					.ppEnabledLayerNames     = nullptr, // ^ ditto
					.enabledExtensionCount   = static_cast<u32>( gRequiredDeviceExtensions.size() ), // TODO?
					.ppEnabledExtensionNames = gRequiredDeviceExtensions.data()                      // TODO?
				}
			);
		} // end-of-function: Renderer::makeLogicalDevice
		
		
		
		// TODO: refactor away second arg and use automated tracking + exceptions
		[[nodiscard]] std::unique_ptr<vk::raii::Queue>
		Renderer::makeQueue(
			u32 const queueFamilyIndex,
			u32 const queueIndex
		)
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
			//   shouldn't be null or undefined unless the function is called in the wrong order:
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
		
		
		
		[[nodiscard]] std::unique_ptr<vk::raii::CommandBuffers>
		Renderer::makeCommandBuffers(
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
		Renderer::makeSyncPrimitives()
		{
			// TODO(refactor)
			spdlog::info( "Creating synchronization primitives..." );
			mImagePresentable .clear();
			mImageAvailable   .clear();
			mFencesInFlight   .clear();
			mImagesInFlight   .clear();
			mImagePresentable .reserve( gMaxConcurrentFrames );
			mImageAvailable   .reserve( gMaxConcurrentFrames );
			mFencesInFlight   .reserve( gMaxConcurrentFrames );
//			mImagesInFlight   .resize(  mFramebufferCount    );
			for ( auto i{0}; i < gMaxConcurrentFrames; ++i ) {
				mImagePresentable .emplace_back( *mpDevice, vk::SemaphoreCreateInfo {} );
				mImageAvailable   .emplace_back( *mpDevice, vk::SemaphoreCreateInfo {} );
				mFencesInFlight   .emplace_back( *mpDevice, vk::FenceCreateInfo     {} );
			}
		} // end-of-function: Renderer::makeSyncPrimitives()
		
		
		
		void
		Renderer::makeSwapchain()
		{
			// TODO(refactor)
			spdlog::debug( "Creating swapchain and necessary state!" );
			
			mpWindow->waitResize();
			mpDevice->waitIdle();
			
			// delete previous state (if any) in the right order:
			if ( mpFramebuffers ) {
				mpFramebuffers.reset();
			}
			if ( mpCommandBuffers ) {
				mpCommandBuffers->clear();
				mpCommandBuffers.reset();
			}
			if ( mpGraphicsPipeline ) {
				mpGraphicsPipeline.reset();
		   }
			if ( mpSwapchain ) {
				mpSwapchain.reset();
			}
			
			// TODO(refactor)
			mpSwapchain = std::make_unique<Swapchain>(
				*mpPhysicalDevice,
				*mpDevice,
				*mpWindow,
				 mQueueFamilies
			);
			
			mpGraphicsPipeline = std::make_unique<Pipeline>(
				*mpDevice,
				*mpSwapchain
			);
			
			mpFramebuffers = std::make_unique<Framebuffers>(
				*mpDevice,
				*mpSwapchain,
				*mpGraphicsPipeline
			);
			
			mpCommandBuffers = makeCommandBuffers(
				vk::CommandBufferLevel::ePrimary,
				mFramebufferCount
			);
			
			// TODO: refactor
			vk::ClearValue const clearValue { .color = {{{ 0.02f, 0.02f, 0.02f, 1.0f }}} };
			
			for ( u32 index{0}; index < mFramebufferCount; ++index ) {
				auto &commandBuffer = *mpCommandBuffers[index];
				commandBuffer.begin( {} );
				commandBuffer.beginRenderPass(
					vk::RenderPassBeginInfo {
						.renderPass      = *mpRenderPass,
						.framebuffer     = *mpFramebuffers[index],
						.renderArea      = vk::Rect2D {
						                    .extent = mSurfaceExtent,
						                 },
						.clearValueCount =  1, // TODO: explain
						.pClearValues    = &clearValue
					},
					vk::SubpassContents::eInline // inline; no secondary command buffers allowed
				);
				commandBuffer.bindPipeline(
					vk::PipelineBindPoint::eGraphics,
					*mpGraphicsPipeline
				);
				// TODO(later): commandBuffer.bindDescriptorSets(
				// TODO(later): 	vk::PipelineBindPoint::eGraphics,
				// TODO(later): 	*pipelineLayout,
				// TODO(later): 	0,
				// TODO(later): 	{ *descriptorSet },
				// TODO(later): 	nullptr
				// TODO(later): );
				// TODO(later): commandBuffer.bindVertexBuffers( 0, { *vertexBuffer }, { 0 } );
				// NOTE(possibility): command_buffer.bindDescriptorSets()
				// NOTE(possibility): command_buffer.setViewport()
				// NOTE(possibility): command_buffer.setScissor()
				commandBuffer.draw(
					3, // vertex count
					1, // instance count
					0, // first vertex
					0  // first instance
				);
				commandBuffer.endRenderPass();
				commandBuffer.end();
			}
		} // end-of-function: Renderer::createSwapchain
	} // end-of-unnamed-namespace
	
	
	
	Renderer::Renderer():
		mShouldRemakeSwapchain { false },
		mCurrentFrame          { 0     }
	{
		spdlog::info( "Constructing a Renderer instance..." );
		mpGlfwInstance = std::make_unique<GlfwInstance>();
		mpVkContext    = std::make_unique<vk::raii::Context>();
		makeInstance();
		maybeMakeDebugMessenger();
		mpWindow       = std::make_unique<Window>( *mpGlfwInstance, *mpVkInstance );
		selectPhysicalDevice();
		selectQueueFamilies();
		makeLogicalDevice();
		makeGraphicsQueue();
		makePresentQueue();
		makeCommandPool();
		makeSwapchain();
		makeSyncPrimitives(); // TODO(config): refactor so it is updated whenever framebuffer count changes
	//instance
			createInfo.enabledLayerCount       = static_cast<u32>( mValidationLayers.size() );
			createInfo.ppEnabledLayerNames     = mValidationLayers.data();
			createInfo.enabledExtensionCount   = static_cast<u32>( mInstanceExtensions.size() );
			createInfo.ppEnabledExtensionNames = mInstanceExtensions.data();
	} // end-of-function: Renderer::Renderer
	
	
	
	Renderer::~Renderer() noexcept
	{
		spdlog::info( "Destroying a Renderer instance..." );
		pState->p_logical_device->waitIdle();
		if ( pState and pState->p_command_buffers )
			pState->p_command_buffers->clear();
	} // end-of-function: Renderer::~Renderer
	
	
	
	[[nodiscard]] Window const &
	Renderer::getWindow() const
	{
		return *pWindow;
	} // end-of-function: Renderer::getWindow
	
	
	
	[[nodiscard]] Window &
	Renderer::getWindow()
	{
		return *pWindow;
	} // end-of-function: Renderer::getWindow
	
	
   
	void
	Renderer::operator()()
	{
		#define TIMEOUT 5000000 // TEMP! TODO // UINT64_MAX? (or limits)
		auto const frame = mCurrentFrame % gMaxConcurrentFrames;
		++mCurrentFrame;
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
		u32        acquiredIndex;
		vk::Result acquiredResult; // TODO: move inside?
		try {
			auto const [result, index] {
				mpSwapchain.acquireNextImage(
					TIMEOUT,
					*mImageAvailable[frame]
				)
			};
			acquiredIndex  = index;
			acquiredResult = result;
		}
		catch( vk::OutOfDateKHRError const & ) { /* do nothing */ }
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
		vk::PipelineStageFlags const waitDestinationStageMask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput
		);
		
		mpGraphicsQueue->submit(
			vk::SubmitInfo {
				.waitSemaphoreCount   = 1,
				.pWaitSemaphores      = &mImageAvailable[acquiredIndex],
				.pWaitDstStageMask    = &waitDestinationStageMask,,
				.commandBufferCount   = 1,
				.pCommandBuffers      = &*(*mpCommandBuffers)[acquiredIndex],
				.signalSemaphoreCount = 1,
				.pSignalSemaphores    = &*mImagePresentable[acquiredIndex], 
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
		vk::Result presentResult;
		try {
			presentResult = mpPresentQueue->presentKHR(
				vk::PresentInfoKHR {
					.waitSemaphoreCount = 1,
					.pWaitSemaphores    = &*mImagePresentable[acquiredIndex],
					.swapchainCount     = 1,
					.pSwapchains        = &*(mpSwapchain),
					.pImageIndices      = &acquiredIndex,
				}
			);
		}
		// TODO find a way to avoid try-catch here
		catch ( vk::OutOfDateKHRError const & ) { /* do nothing */ }
//		
//		if ( m_p_state->should_remake_swapchain
//		or   present_result==vk::Result::eSuboptimalKHR
//		or   present_result==vk::Result::eErrorOutOfDateKHR ) {
//			m_p_state->should_remake_swapchain = false;
//			make_swapchain( *m_p_state );
//		}
//		else if ( present_result != vk::Result::eSuccess )
//			throw std::runtime_error { "Failed to present swapchain image!" };
	} // end-of-function: Renderer::operator()
	
	
	
} // end-of-namespace: gfx
// EOF
