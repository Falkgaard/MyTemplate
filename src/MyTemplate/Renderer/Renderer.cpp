#include "MyTemplate/Renderer/Renderer.hpp"
#include "MyTemplate/Common/aliases.hpp"
#include "MyTemplate/Renderer/common.hpp"	
#include "MyTemplate/Renderer/GlfwInstance.hpp"
#include "MyTemplate/Renderer/Window.hpp"
#include "MyTemplate/Renderer/Primitives.hpp"

#include <spdlog/spdlog.h>

#include <ranges>
#include <algorithm>
#include <optional>
#include <array>
#include <vector>
#include <set>
#include <fstream>
#include <memory>
#include <cassert>

// TODO(later): Switch over to a custom allocator (e.g. for buffers) later, such as VulkanMemoryAllocator
// TODO(later): Use a single buffer for shared attributes (verts, indices, etc)
// TODO(later): Look into aliasing (memory buffer reuse)

namespace gfx {	
	namespace { // private (file-scope)
		// TODO(config): refactor
		u64                         constexpr kDrawWaitTimeout            { max<u64>                                 };
		u32                         constexpr kMaxConcurrentFrames        { 3                                        };
		PresentationPriority        constexpr kPresentationPriority       { PresentationPriority::eMinimalStuttering };
		FramebufferingPriority      constexpr kFramebufferingPriority     { FramebufferingPriority::eTriple          };
		std::array                  constexpr kRequiredDeviceExtensions   { VK_KHR_SWAPCHAIN_EXTENSION_NAME          };
		#if !defined( NDEBUG )
		std::array                  constexpr kRequiredValidationLayers   { "VK_LAYER_KHRONOS_validation"            };
		std::array                  constexpr kRequiredInstanceExtensions { VK_EXT_DEBUG_UTILS_EXTENSION_NAME        };
		#else
		std::array<char const *, 0> constexpr kRequiredValidationLayers   {};
		std::array<char const *, 0> constexpr kRequiredInstanceExtensions {};
		#endif
		
		
		#if !defined( NDEBUG )
			VKAPI_ATTR VkBool32 VKAPI_CALL
			debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT      msgSeverity,
				VkDebugUtilsMessageTypeFlagsEXT             msgType,
				VkDebugUtilsMessengerCallbackDataEXT const *fpCallbackData,
				void * // unused for now
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
						spdlog::error( "{} | {} | {}: {}", msgTypeName, pMsgIdName, msgId, pMsg );
						//spdlog::error( "{}: {}", msgTypeName, pMsg );
						break;
					}
					[[likely]] case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
						spdlog::info( "{} | {} | {}: {}", msgTypeName, pMsgIdName, msgId, pMsg );
						//spdlog::info( "{}: {}", msgTypeName, pMsg );
						break;
					}
					case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: { // TODO: contemplate a better fit
						spdlog::debug( "{} | {} | {}: {}", msgTypeName, pMsgIdName, msgId, pMsg );
						//spdlog::debug( "{}: {}", msgTypeName, pMsg );
						break;
					}
					[[unlikely]] case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
						//spdlog::warn( "[{}] {} (#{}): {}", msgTypeName, pMsgIdName, msgId, pMsg );
						spdlog::warn( "{} | {} | {}: {}", msgTypeName, pMsgIdName, msgId, pMsg );
						//spdlog::warn( "{}: {}", msgTypeName, pMsg );
						break;
					}
				}
				// TODO: expand with more info from pCallbackData later
				return false; // TODO: add explanation
			} // end-of-function: debugCallback
		#endif // end-of-debug-block
			
		[[nodiscard]] auto
		loadBinaryFromFile( std::string const &binaryFilename )
		{  // TODO: null_terminated_string_view?
			spdlog::info( "Attempting to open binary file `{}`...", binaryFilename );
			std::ifstream binaryFile {
				binaryFilename,
				std::ios::binary | std::ios::ate // start at the end of the binary
			};
			if ( binaryFile ) {
				spdlog::info( "... successful!" );
				auto const binarySize {
					static_cast<std::size_t>( binaryFile.tellg() )
				};
				spdlog::info( "... binary size: {}", binarySize );
				binaryFile.seekg( 0 );
				std::vector<char> binaryBuffer( binarySize );
				binaryFile.read( binaryBuffer.data(), binarySize );
				return binaryBuffer;
			}
			else {
				spdlog::warn( "... failure!" );
				throw std::runtime_error { "Failed to load binary file!" };
			}
		} // end-of-function: loadBinaryFromFile()
	} // end-of-unnamed-namespace	
	
	
	
	[[nodiscard]] u32
	Renderer::findMemoryTypeIndex( u32 const typeFilter, vk::MemoryPropertyFlags const flags ) const
	{
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpPhysicalDevice != nullptr );
		
		auto const memoryProperties { mpPhysicalDevice->getMemoryProperties() };
		for ( u32 i{0};  i < memoryProperties.memoryTypeCount;  ++i ) 
			if ( (typeFilter & (1 << i))
			and  (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags )
				return i;
		
		throw std::runtime_error { "Unable to find an index of a suitable memory type!" };
	} // end-of-function: Renderer::findMemoryTypeIndex
	
	
	
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
		for ( auto const &requiredLayer: kRequiredValidationLayers )
			spdlog::info( "        required  : `{}`", requiredLayer );
		if constexpr ( kIsVerbose )
			for ( auto const &availableLayer: availableLayers )
				spdlog::info( "        available : `{}`", availableLayer.layerName );
		
		// ensure that all required layers are available:
		bool isMissingLayer { false };
		for ( auto const &requiredLayer: kRequiredValidationLayers ) {
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
				spdlog::error( "   !!! MISSING   : `{}`!", requiredLayer );
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
		assert( mInstanceExtensions.empty() );
		
		auto const availableExtensions    { mpVkContext->enumerateInstanceExtensionProperties() };
		auto const glfwRequiredExtensions { mpGlfwInstance->getRequiredExtensions()             };
		
		// make a union of all instance extensions requirements:
		std::set<char const *> allRequiredExtensions {};
		for ( auto const &requiredExtension: glfwRequiredExtensions )
			allRequiredExtensions.insert( requiredExtension );
		for ( auto const &requiredExtension: kRequiredInstanceExtensions )
			allRequiredExtensions.insert( requiredExtension );
		
		if constexpr ( kIsDebugMode ) {
			// print required and available instance extensions:
			for ( auto const &requiredExtension: allRequiredExtensions )
				spdlog::info( "        required  : `{}`", requiredExtension );
			if constexpr ( kIsVerbose )
			   for ( auto const &availableExtension: availableExtensions )
					spdlog::info( "        available : `{}`", availableExtension.extensionName );
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
				spdlog::error( "   !!! MISSING   : `{}`", requiredExtension );
			}
		}
		
		if ( not isAdequate ) [[unlikely]]
			throw std::runtime_error { "Failed to load required instance extensions!" };
	} // end-of-function: Renderer::enableInstanceExtensions
	
	
	
	[[nodiscard]] bool
	Renderer::meetsDeviceExtensionRequirements( vk::raii::PhysicalDevice const &physicalDevice ) const
	{
		// NOTE: all extensions in this function are device extensions
		auto const &availableExtensions { physicalDevice.enumerateDeviceExtensionProperties() };
		
		if constexpr ( kIsDebugMode ) {
			spdlog::info( "... checking device extension support:" );
			// print required and available device extensions:
			for ( auto const &requiredExtension: kRequiredDeviceExtensions )
				spdlog::info( "      required  : `{}`", requiredExtension );
			if constexpr ( kIsVerbose )
   			for ( auto const &availableExtension: availableExtensions )
				   spdlog::info( "      available : `{}`", availableExtension.extensionName );
		}	
		
		bool isAdequate { true }; // assume true until proven otherwise
		for ( auto const &requiredExtension: kRequiredDeviceExtensions ) { // TODO: make gRequiredDeviceExtensions a member
			bool isSupported { false }; // assume false until found
			for ( auto const &availableExtension: availableExtensions ) {
				if ( std::strcmp( requiredExtension, availableExtension.extensionName ) == 0 ) [[unlikely]] {
					isSupported = true;
					break; // early exit
				}
			}
			spdlog::info(
				"      support for required device extension `{}`: {}",
				requiredExtension, isSupported ? "OK" : "missing!"
			);
			if ( not isSupported ) [[unlikely]]
				isAdequate = false;
		}
		return isAdequate;
	} // end-of-function: Renderer::meetsDeviceExtensionRequirements
	
	
	
	[[nodiscard]] u32
	Renderer::calculateScore( vk::raii::PhysicalDevice const &physicalDevice ) const
	{
		auto const properties { physicalDevice.getProperties() };
		auto const features   { physicalDevice.getFeatures()   };
		
		spdlog::info( "Scoring physical device `{}`...", properties.deviceName.data() );
		
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
		assert( pBestMatch != nullptr );
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
			mpPhysicalDevice                 = std::make_unique<vk::raii::PhysicalDevice>( std::move( *pBestMatch ) );
		}
		else [[unlikely]]
			throw std::runtime_error { "Physical device does not support swapchains!" };
	} // end-of-function: Renderer::selectPhysicalDevice
	
	
	
	void
	Renderer::makeVkContext()
	{
		spdlog::info( "Creating a Vulkan context..." );
		
		// pre-condition(s):
		//   should be null unless the function has been called multiple times (which it shouldn't)
		assert( mpVkContext == nullptr );
		
		mpVkContext = std::make_unique<vk::raii::Context>();
	} // end-of-function: Renderer::makeVkContext
	
	
	
	void
	Renderer::makeVkInstance()
	{
		spdlog::info( "Creating a Vulkan instance..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpVkContext  != nullptr );		
		//   should be null unless the function has been called multiple times (which it shouldn't)
		assert( mpVkInstance == nullptr );
		
		// TODO: split engine/app versions and make customization point
		// TODO: move into some ApplicationInfo class created in main?
		auto const version {
			VK_MAKE_VERSION(
				MYTEMPLATE_VERSION_MAJOR,
				MYTEMPLATE_VERSION_MINOR,
				MYTEMPLATE_VERSION_PATCH
			)
		};
		vk::ApplicationInfo const appInfo {
			.pApplicationName   = "MyTemplate App", // TODO: make customization point 
			.applicationVersion = version,          // TODO: make customization point 
			.pEngineName        = "MyTemplate Engine",
			.engineVersion      = version,
			.apiVersion         = VK_API_VERSION_1_1
		};
		
		enableValidationLayers();
		enableInstanceExtensions();
		
		mpVkInstance = std::make_unique<vk::raii::Instance>(
			*mpVkContext,
			vk::InstanceCreateInfo {
				.pApplicationInfo        = &appInfo,
				.enabledLayerCount       = static_cast<u32>( mValidationLayers.size() ),
				.ppEnabledLayerNames     = mValidationLayers.data(),
				.enabledExtensionCount   = static_cast<u32>( mInstanceExtensions.size() ),
				.ppEnabledExtensionNames = mInstanceExtensions.data(),
			}
		);
	} // end-of-function: Renderer::makeVkInstance
	
	
	
	void
	Renderer::maybeMakeDebugMessenger()
	{
		#if !defined( NDEBUG )
			spdlog::info( "Creating a debug messenger..." );
			
			// pre-condition(s):
			//   shouldn't be null unless the function is called in the wrong order:
			assert( mpVkContext      != nullptr );
			assert( mpVkInstance     != nullptr );
			//   should be null unless the function has been called multiple times (which it shouldn't)
			assert( mpDebugMessenger == nullptr );
			
			vk::DebugUtilsMessageSeverityFlagsEXT const severityFlags {
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError   |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo // |
			//	vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose // TODO: tie to kIsVerbose
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
		assert( mQueueFamilyIndices.transferIndex == QueueFamilyIndices::kUndefined );
		
		std::optional<u32> maybePresentIndex  {};
		std::optional<u32> maybeGraphicsIndex {};
		std::optional<u32> maybeTransferIndex {};
		bool hasFoundDualSupport { false }; // assume false until present-graphics supporting queue family is found
		auto const &queueFamilyProperties { mpPhysicalDevice->getQueueFamilyProperties() };
		
		for ( u32 index{0};  index < queueFamilyProperties.size();  ++index ) {
			bool const supportsGraphics { queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics };
			bool const supportsPresent  { mpPhysicalDevice->getSurfaceSupportKHR( index, *mpWindow->getSurface() ) == VK_TRUE };
			bool const supportsTransfer { queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eTransfer };
			
			spdlog::info( "... Evaluating queue family index {}:", index );
			spdlog::info( "    ...  present support: {}", supportsPresent  ? "yes" : "no" );
			spdlog::info( "    ... graphics support: {}", supportsGraphics ? "yes" : "no" );
			spdlog::info( "    ... graphics support: {}", supportsTransfer ? "yes" : "no" );
			if ( not hasFoundDualSupport ) {
				if ( supportsGraphics and supportsPresent ) {
					maybePresentIndex   = index;
					maybeGraphicsIndex  = index;
					hasFoundDualSupport = true;
				}
				else if ( supportsGraphics )
					maybeGraphicsIndex = index;
				else if ( supportsPresent )
					maybePresentIndex  = index;
			}
			if ( supportsTransfer and not supportsGraphics )
				maybeTransferIndex = index;
		}
		
		if ( maybePresentIndex.has_value() and maybeGraphicsIndex.has_value() and maybeTransferIndex.value() ) [[likely]] {
			mQueueFamilyIndices = QueueFamilyIndices {
				.presentIndex  = maybePresentIndex.value(),
				.graphicsIndex = maybeGraphicsIndex.value(),
				.transferIndex = maybeTransferIndex.value(),
				.areSeparate   = not hasFoundDualSupport
			};
			if ( mQueueFamilyIndices.areSeparate ) [[unlikely]]
				spdlog::info( "... selected different queue families for graphics and present." );
			else [[likely]]
				spdlog::info( "... ideal queue family was found graphics and present!" );
		}
		else [[unlikely]]
			throw std::runtime_error { "Queue family support for either graphics, present, or transfer is missing!" };
	} // end-of-function: Renderer::select_queue_family_indices
	
	
	
	void
	Renderer::makeLogicalDevice()
	{
		spdlog::info( "Creating a logical device..." );
		
		// pre-condition(s):
		//   shouldn't be undefined and null unless the function is called in the wrong order:
		assert( mQueueFamilyIndices.presentIndex  != QueueFamilyIndices::kUndefined );
		assert( mQueueFamilyIndices.graphicsIndex != QueueFamilyIndices::kUndefined );
		assert( mpPhysicalDevice                  != nullptr                        );
		//   should be null unless the function has been called multiple times (which it shouldn't):
		assert( mpDevice                          == nullptr                        ); 
		
		// TODO: refactor approach when more queues are needed
		std::vector<vk::DeviceQueueCreateInfo> createInfos {};
		
		f32 const presentQueuePriority  { 1.0f };
		f32 const graphicsQueuePriority { 1.0f };
		
		createInfos.push_back(
			vk::DeviceQueueCreateInfo {
				.queueFamilyIndex =  mQueueFamilyIndices.presentIndex,
				.queueCount       =  1,
				.pQueuePriorities = &presentQueuePriority
			}
		);
		
		if ( mQueueFamilyIndices.areSeparate ) [[unlikely]] {
			createInfos.push_back(
				vk::DeviceQueueCreateInfo {
					.queueFamilyIndex =  mQueueFamilyIndices.graphicsIndex,
					.queueCount       =  1,
					.pQueuePriorities = &graphicsQueuePriority
				}
			);
		}
		
		createInfos.push_back(
			vk::DeviceQueueCreateInfo {
				.queueFamilyIndex =  mQueueFamilyIndices.transferIndex,
				.queueCount       =  1,
				.pQueuePriorities = &presentQueuePriority
			}
		);
		
		mpDevice = std::make_unique<vk::raii::Device>(
			*mpPhysicalDevice,
			vk::DeviceCreateInfo {
				.queueCreateInfoCount    = static_cast<u32>( createInfos.size() ),
				.pQueueCreateInfos       = createInfos.data(),
				.enabledLayerCount       = 0,       // TODO(verify): deprecated
				.ppEnabledLayerNames     = nullptr, // TODO(verify): deprecated
				.enabledExtensionCount   = static_cast<u32>( kRequiredDeviceExtensions.size() ), // TODO(member)
				.ppEnabledExtensionNames = kRequiredDeviceExtensions.data(),                     // TODO(member)
			}
		);
	} // end-of-function: Renderer::makeLogicalDevice
	
	
	
	// TODO: refactor away second arg and use automated tracking + exceptions
	[[nodiscard]] std::unique_ptr<vk::raii::Queue>
	Renderer::makeQueue( u32 const queueFamilyIndex, u32 const queueIndex )
	{
		spdlog::info( "Creating a handle for queue #{} of queue family #{}...", queueIndex, queueFamilyIndex );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpDevice != nullptr );
		
		return std::make_unique<vk::raii::Queue>( mpDevice->getQueue( queueFamilyIndex, queueIndex ) );
	} // end-of-function: Renderer::makeQueue
	
	
	
	void
	Renderer::makeQueues()
	{
		spdlog::info( "Creating queues..." );

		// pre-condition(s):
		//   should be null unless the function has been called multiple times (which it shouldn't):
		assert( mpGraphicsQueue                   == nullptr                        ); 
		assert( mpPresentQueue                    == nullptr                        ); 
		assert( mpTransferQueue                   == nullptr                        ); 
		assert( mQueueFamilyIndices.graphicsIndex != QueueFamilyIndices::kUndefined ); 
		assert( mQueueFamilyIndices.presentIndex  != QueueFamilyIndices::kUndefined ); 
		assert( mQueueFamilyIndices.transferIndex != QueueFamilyIndices::kUndefined ); 
		
		spdlog::info( "... creating the graphics queue" );
		mpGraphicsQueue = makeQueue( mQueueFamilyIndices.graphicsIndex, 0 );
		spdlog::info( "... creating the present queue" );
		mpPresentQueue  = makeQueue( mQueueFamilyIndices.presentIndex,  0 );
		spdlog::info( "... creating the transfer queue" );
		mpTransferQueue = makeQueue( mQueueFamilyIndices.transferIndex, 0 );
	} // end-of-function: Renderer::makeQueues
	
	
	
	void
	Renderer::makeCommandPools()
	{
		spdlog::info( "Creating a command buffer pools..." );
		
		// pre-condition(s):
		//   shouldn't be null or undefined unless the function is called in the wrong order:
		assert( mpGraphicsQueue                   != nullptr                        ); 
		assert( mpTransferQueue                   != nullptr                        ); 
		assert( mQueueFamilyIndices.graphicsIndex != QueueFamilyIndices::kUndefined ); 
		assert( mQueueFamilyIndices.transferIndex != QueueFamilyIndices::kUndefined ); 
		//   should be null unless the function has been called multiple times (which it shouldn't):
		assert( mpGraphicsCommandPool == nullptr ); 
		assert( mpTransferCommandPool == nullptr ); 
		
		spdlog::info( "... creating a graphics command buffer pool" );
		mpGraphicsCommandPool = std::make_unique<vk::raii::CommandPool>(
			*mpDevice,
			vk::CommandPoolCreateInfo {
				// NOTE: Flags can be set here to optimize for lifetime or enable resetability.
				//       Also, one pool would be needed for each queue family (if ever extended).
				.queueFamilyIndex = mQueueFamilyIndices.graphicsIndex
			}
		);
		
		spdlog::info( "... creating a graphics command buffer pool" );
		mpTransferCommandPool = std::make_unique<vk::raii::CommandPool>(
			*mpDevice,
			vk::CommandPoolCreateInfo {
				// NOTE: Flags can be set here to optimize for lifetime or enable resetability.
				//       Also, one pool would be needed for each queue family (if ever extended).
				.queueFamilyIndex = mQueueFamilyIndices.transferIndex
			}
		);
	} // end-of-function: Renderer::makeCommandPools
	
	
	
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
				.commandPool        = **mpGraphicsCommandPool,
				.level              =   level,
				.commandBufferCount =   bufferCount
			}
		);
	} // end-of-function: Renderer::makeCommandBuffers()
	
	
	
	void
	Renderer::selectSurfaceFormat()
	{
		spdlog::info( "Selecting a swapchain surface format..." );
			
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpPhysicalDevice != nullptr );
		assert( mpWindow         != nullptr );
		
		auto const availableSurfaceFormats { mpPhysicalDevice->getSurfaceFormatsKHR( *mpWindow->getSurface() ) };
		for ( auto const &surfaceFormat: availableSurfaceFormats ) {
			if ( surfaceFormat.format     == vk::Format::eB8G8R8A8Srgb
			and  surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear ) [[unlikely]] {
				spdlog::info(
					"... selected surface format `{}` in color space `{}`",
					to_string( surfaceFormat.format     ),
					to_string( surfaceFormat.colorSpace )
				);
				mSurfaceFormat = surfaceFormat;
				return;
			}
		}
		// TODO: add contingency decisions to fallback on
		throw std::runtime_error { "Unable to find the desired surface format!" };	
	} // end-of-function: Renderer::selectSurfaceFormat
	
	
	
	void
	Renderer::selectSurfaceExtent()
	{
		spdlog::info( "Selecting a swapchain surface extent..." );
			
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
//		assert( mSurfaceCapabilities != ???     );
		assert( mpWindow             != nullptr );
		
		vk::Extent2D result {};
		if ( mSurfaceCapabilities.currentExtent.height == max<u32> ) // TODO: unlikely? likely?
			result = mSurfaceCapabilities.currentExtent;
		else {
			auto [width, height] = mpWindow->getWindowDimensions();
			result.width =
				std::clamp(
					static_cast<u32>(width),
					mSurfaceCapabilities.minImageExtent.width,
					mSurfaceCapabilities.maxImageExtent.width
				);
			result.height =
				std::clamp(
					static_cast<u32>(height),
					mSurfaceCapabilities.minImageExtent.height,
					mSurfaceCapabilities.maxImageExtent.height
				);
		}
		spdlog::info( "... selected swapchain extent: {}x{}", result.width, result.height );
		mSurfaceExtent = result;
	} // end-of-function: Renderer::selectSurfaceExtent
	
	
	
	void
	Renderer::selectPresentMode() noexcept
	{
		// TODO: add support for ordered priorities instead of just ideal/fallback.
		spdlog::info( "Selecting a swapchain present mode..." );
			
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpPhysicalDevice != nullptr );
		assert( mpWindow         != nullptr );
		
		auto const &windowSurface         { mpWindow->getSurface() };
		auto const  fallbackPresentMode   { vk::PresentModeKHR::eFifo };
		auto const  availablePresentModes { mpPhysicalDevice->getSurfacePresentModesKHR(*windowSurface) };
		auto const  idealPresentMode {
			[/*gPresentationPriority*/] {
				switch ( kPresentationPriority ) {
					case PresentationPriority::eMinimalLatency          : return vk::PresentModeKHR::eMailbox;
					case PresentationPriority::eMinimalStuttering       : return vk::PresentModeKHR::eFifoRelaxed;
					case PresentationPriority::eMinimalPowerConsumption : return vk::PresentModeKHR::eFifo;
				}
				unreachable();
			}()
		};
		bool const hasSupportForIdealMode {
			std::ranges::find( availablePresentModes, idealPresentMode ) != availablePresentModes.end()
		};
		if ( hasSupportForIdealMode ) [[likely]] {
			spdlog::info( "... ideal present mode `{}` is supported by device!", to_string(idealPresentMode) );
			mPresentMode = idealPresentMode;
		}
		else [[unlikely]] {
			spdlog::warn(
				"... ideal present mode is not supported by device; using fallback present mode `{}`!",
				to_string( fallbackPresentMode )
			);
			mPresentMode = fallbackPresentMode;
		}
	} // end-of-function: Renderer::selectPresentMode
	
	
	
	void
	Renderer::selectFramebufferCount() noexcept
	{
		spdlog::info( "Selecting a swapchain framebuffer count..." );
			
		// pre-condition(s):
		//   shouldn't be ??? unless the function is called in the wrong order:
//		assert( mSurfaceCapabilities != ??? );
		
		auto const idealFramebufferCount { static_cast<u32>( kFramebufferingPriority ) };
		spdlog::info( "... ideal framebuffer count: {}", idealFramebufferCount );
		auto const minimumFramebufferCount { mSurfaceCapabilities.minImageCount };
		auto const maximumFramebufferCount {
			mSurfaceCapabilities.maxImageCount == 0 ? // handle special 0 (uncapped) case
				idealFramebufferCount : mSurfaceCapabilities.maxImageCount
		};
		auto result {
			std::clamp(
				idealFramebufferCount,
				minimumFramebufferCount,
				maximumFramebufferCount
			)
		};
		spdlog::info( "... nearest available framebuffer count: {}", result );
		mFramebufferCount = result;
	} // end-of-function: Renderer::selectFramebufferCount
	
	
	
	void
	Renderer::makeSwapchain()
	{
		spdlog::info( "Making a swapchain..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpPhysicalDevice != nullptr );
		assert( mpDevice         != nullptr );
		assert( mpWindow         != nullptr );
		
		mSurfaceCapabilities = mpPhysicalDevice->getSurfaceCapabilitiesKHR( *mpWindow->getSurface() ); // TODO(refactor)
		selectSurfaceFormat();
		selectSurfaceExtent();
		selectPresentMode();
		selectFramebufferCount();
		
		u32 const queueFamilyIndices[] {
			mQueueFamilyIndices.presentIndex,
			mQueueFamilyIndices.graphicsIndex
		};
		// TODO: check present support	
		mpSwapchain = std::make_unique<vk::raii::SwapchainKHR>(
			*mpDevice,
			vk::SwapchainCreateInfoKHR {
				.surface               = *mpWindow->getSurface(),
				.minImageCount         =  mFramebufferCount,
				.imageFormat           =  mSurfaceFormat.format,
				.imageColorSpace       =  mSurfaceFormat.colorSpace,
				.imageExtent           =  mSurfaceExtent,
				.imageArrayLayers      =  1, // non-stereoscopic
				.imageUsage            =  vk::ImageUsageFlagBits::eColorAttachment, // TODO(later): eTransferDst for post-processing
				.imageSharingMode      =  mQueueFamilyIndices.areSeparate ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
				.queueFamilyIndexCount =  mQueueFamilyIndices.areSeparate ?                           2u : 0u,
				.pQueueFamilyIndices   =  mQueueFamilyIndices.areSeparate ?           queueFamilyIndices : nullptr,
				.preTransform          =  mSurfaceCapabilities.currentTransform,
				.compositeAlpha        =  vk::CompositeAlphaFlagBitsKHR::eOpaque,
				.presentMode           =  mPresentMode,
				.clipped               =  VK_TRUE,
				.oldSwapchain          =  VK_NULL_HANDLE // TODO: revisit later after implementing resizing
			}
		);
		
		// Image views: TODO(refactor)
		spdlog::info( "Creating swapchain framebuffer image view(s)..." );
		mImages = mpSwapchain->getImages();
		mImageViews.reserve( mImages.size() );
		for ( auto const &image: mImages ) {
			mImageViews.emplace_back(
				*mpDevice,
				vk::ImageViewCreateInfo {
					.image            = static_cast<vk::Image>( image ),
					.viewType         = vk::ImageViewType::e2D,
					.format           = mSurfaceFormat.format, // TODO: verify
					.subresourceRange = vk::ImageSubresourceRange {
					                     .aspectMask     = vk::ImageAspectFlagBits::eColor,
					                     .baseMipLevel   = 0u,
					                     .levelCount     = 1u,
					                     .baseArrayLayer = 0u,
					                     .layerCount     = 1u
					                  }
				}
			);
		}	
	} // end-of-function: Renderer::makeSwapchain
	
	
	
	[[nodiscard]] std::unique_ptr<vk::raii::ShaderModule>
	Renderer::makeShaderModuleFromBinary( std::vector<char> const &shaderBinary ) const
	{
		spdlog::info( "Creating a shader module from shader SPIR-V bytecode..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpDevice != nullptr );
		
		return std::make_unique<vk::raii::ShaderModule>(
			*mpDevice,
			vk::ShaderModuleCreateInfo {
				.codeSize = shaderBinary.size(),
				.pCode    = reinterpret_cast<u32 const *>( shaderBinary.data() ) /// TODO: UB or US?
			}	
		);
	} // end-of-function: Renderer::makeShaderModuleFromBinary
	
	
	
	[[nodiscard]] std::unique_ptr<vk::raii::ShaderModule>
	Renderer::makeShaderModuleFromFile( std::string const &shaderSpirvBytecodeFilename ) const
	{
		spdlog::info( "Creating a shader module from shader SPIR-V bytecode file..." );
		try {
			return makeShaderModuleFromBinary( loadBinaryFromFile( shaderSpirvBytecodeFilename ) );
		}
		catch( std::runtime_error const &e ) {
			spdlog::error( "Failed to load shader binary from file! Check path and/or re-compile shaders." );
			throw e;
		}
	} // end-of-function: Renderer::makeShaderModuleFromFile
	
	
	
	void
	Renderer::makeGraphicsPipelineLayout()
	{
		spdlog::info( "Creating a graphics pipeline layout..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpDevice != nullptr );
		
		mpGraphicsPipelineLayout = std::make_unique<vk::raii::PipelineLayout>(
			*mpDevice,
			vk::PipelineLayoutCreateInfo {
				.setLayoutCount         = 0,       // TODO: explain
				.pSetLayouts            = nullptr, // TODO: explain
				.pushConstantRangeCount = 0,       // TODO: explain
				.pPushConstantRanges    = nullptr  // TODO: explain
			}
		);
	} // end-of-function: Renderer::makeGraphicsPipelineLayout
	
	
	
	void
	Renderer::makeRenderPass()
	{
		spdlog::info( "Creating a render pass..." );
		
		// pre-condition(s):
		//   shouldn't be null or ??? unless the function is called in the wrong order:
		assert( mpDevice       != nullptr );
//		assert( mSurfaceFormat != ???     );
		
		vk::AttachmentDescription const colorAttachmentDesc {
			.format         = mSurfaceFormat.format,
			.samples        = vk::SampleCountFlagBits::e1,      // no MSAA yet
			.loadOp         = vk::AttachmentLoadOp::eClear,
			.storeOp        = vk::AttachmentStoreOp::eStore,
			.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,  // no depth/stencil yet
			.stencilStoreOp = vk::AttachmentStoreOp::eDontCare, // no depth/stencil yet
			.initialLayout  = vk::ImageLayout::eUndefined,
			.finalLayout    = vk::ImageLayout::ePresentSrcKHR
		};
		
		vk::AttachmentReference const colorAttachmentRef {
			.attachment = 0, // index 0; we only have one attachment at the moment
			.layout     = vk::ImageLayout::eColorAttachmentOptimal
		};
		
		vk::SubpassDescription const colorSubpassDesc {
			.pipelineBindPoint    =  vk::PipelineBindPoint::eGraphics,
			.colorAttachmentCount =  1,
			.pColorAttachments    = &colorAttachmentRef
		};
		
		mpRenderPass = std::make_unique<vk::raii::RenderPass>(
			*mpDevice,
			vk::RenderPassCreateInfo {
				.attachmentCount =  1,
				.pAttachments    = &colorAttachmentDesc,
				.subpassCount    =  1,
				.pSubpasses      = &colorSubpassDesc
			}
		);
	} // end-of-function: Renderer::makeRenderPass
	
	
	
	void
	Renderer::makeGraphicsPipeline()
	{
		// TODO: lots of refactoring!
		spdlog::info( "Creating a graphics pipeline..." );
		
		
		// pre-condition(s):
		//   shouldn't be null or ??? unless the function is called in the wrong order:
		assert( mpDevice       != nullptr );
		
		spdlog::info( "Creating shader modules..." );
		auto const vertexModule   = makeShaderModuleFromFile( "../dat/shaders/test1.vert.spv" );
		auto const fragmentModule = makeShaderModuleFromFile( "../dat/shaders/test1.frag.spv" );	
		
		spdlog::info( "Creating pipeline shader stages..." );
		vk::PipelineShaderStageCreateInfo const shaderStageCreateInfo[] {
			{
				.stage  =   vk::ShaderStageFlagBits::eVertex,
				.module = **vertexModule,
				.pName  =   "main" // shader program entry point
			// .pSpecializationInfo is unused (for now); but it allows for setting shader constants
			},
			{
				.stage  =   vk::ShaderStageFlagBits::eFragment,
				.module = **fragmentModule,
				.pName  =   "main" // shader program entry point
			// .pSpecializationInfo is unused (for now); but it allows for setting shader constants
			}
		};
		
		// WHAT: configures the vertex data format (spacing, instancing, loading...)
		vk::PipelineVertexInputStateCreateInfo const vertexInputStateCreateInfo {
			// NOTE: no data here since it's hardcoded (for now)
			.vertexBindingDescriptionCount   =  1,
			.pVertexBindingDescriptions      = &Vertex2D::kVertexInputBindingDescription,
			.vertexAttributeDescriptionCount =  static_cast<u32>( Vertex2D::kVertexInputAttributeDescriptions.size() ),
			.pVertexAttributeDescriptions    =  Vertex2D::kVertexInputAttributeDescriptions.data()
		};
		
		// WHAT: configures the primitive topology of the geometry
		vk::PipelineInputAssemblyStateCreateInfo const inputAssemblyStateCreateInfo {
			.topology               = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = VK_FALSE // unused (for now); allows designating strip gap indices
		};
		
		vk::Viewport const viewport {
			.x        = 0.0f,
			.y        = 0.0f,
			.width    = static_cast<f32>( mSurfaceExtent.width  ),
			.height   = static_cast<f32>( mSurfaceExtent.height ),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		
		vk::Rect2D const scissor {
			.offset = { 0, 0 },
			.extent = mSurfaceExtent
		};
		
		vk::PipelineViewportStateCreateInfo const viewportStateCreateInfo {
			.viewportCount =  1,
			.pViewports    = &viewport,
			.scissorCount  =  1,
			.pScissors     = &scissor,
		};
		
		vk::PipelineRasterizationStateCreateInfo const rasterizationStateCreateInfo {
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
		
		vk::PipelineMultisampleStateCreateInfo const multisampleStateCreateInfo {
			.rasterizationSamples  = vk::SampleCountFlagBits::e1,
			.sampleShadingEnable   = VK_FALSE, // unused; otherwise enables MSAA; requires GPU feature
			.minSampleShading      = 1.0f,
			.pSampleMask           = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable      = VK_FALSE 
		}; // TODO: revisit later
		
		// vk::PipelineDepthStencilStateCreateInfo const depthStencilStateCreateInfo {
		// 	// TODO: unused for now; revisit later
		// };
		
		vk::PipelineColorBlendAttachmentState const colorBlendAttachmentState {
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
		vk::PipelineColorBlendStateCreateInfo const colorBlendStateCreateInfo {
			.logicOpEnable     =  VK_FALSE,
			.logicOp           =  vk::LogicOp::eCopy,
			.attachmentCount   =  1, // TODO: explain
			.pAttachments      = &colorBlendAttachmentState,
			.blendConstants    =  std::array<f32,4>{ 0.0f, 0.0f, 0.0f, 0.0f }
		};
		
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
		
		makeGraphicsPipelineLayout();
		makeRenderPass();
		mpGraphicsPipeline = std::make_unique<vk::raii::Pipeline>(
			*mpDevice,
			nullptr,
			vk::GraphicsPipelineCreateInfo {
				.stageCount          =   2,
				.pStages             =   shaderStageCreateInfo,
				.pVertexInputState   =  &vertexInputStateCreateInfo,
				.pInputAssemblyState =  &inputAssemblyStateCreateInfo,
				.pViewportState      =  &viewportStateCreateInfo,
				.pRasterizationState =  &rasterizationStateCreateInfo,
				.pMultisampleState   =  &multisampleStateCreateInfo,
				.pDepthStencilState  =   nullptr, // unused for now
				.pColorBlendState    =  &colorBlendStateCreateInfo,
				.pDynamicState       =   nullptr, // unused for now
				.layout              = **mpGraphicsPipelineLayout,
				.renderPass          = **mpRenderPass,
				.subpass             =   0,
				.basePipelineHandle  =   VK_NULL_HANDLE,
				.basePipelineIndex   =   -1,
			}
		);
	} // end-of-function: Renderer::makeGraphicsPipeline
	
	
	
	void
	Renderer::makeFramebuffers()
	{
		spdlog::info( "Creating framebuffer(s)..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpDevice     != nullptr );
		assert( mpRenderPass != nullptr );
			
		mFramebuffers.reserve( mFramebufferCount );
		for ( auto const &imageView: mImageViews ) {
			mFramebuffers.emplace_back(
				*mpDevice,
				vk::FramebufferCreateInfo {
					.renderPass      = **mpRenderPass,
					.attachmentCount =   1, // only color for now
					.pAttachments    = &*imageView,
					.width           =   mSurfaceExtent.width,
					.height          =   mSurfaceExtent.height,
					.layers          =   1 // TODO: explain
				}
			);
		}
	} // end-of-function: Renderer::makeFramebuffers	
	
	
	
	[[nodiscard]] std::unique_ptr<Buffer>
	Renderer::makeBuffer(
		vk::BufferUsageFlags    const usage,
		vk::DeviceSize          const size,
		vk::MemoryPropertyFlags const properties
	)
	{
		spdlog::info( "Creating buffer..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpDevice != nullptr );
		
		u32 const queueFamilyIndices[] {
			mQueueFamilyIndices.graphicsIndex,
			mQueueFamilyIndices.transferIndex
		};
		
		spdlog::info( "... creating buffer handle" );
		auto bufferHandle {
			vk::raii::Buffer(
				*mpDevice,
				vk::BufferCreateInfo {
					.size                  = size,
					.usage                 = usage,
					.sharingMode           = vk::SharingMode::eConcurrent,
					.queueFamilyIndexCount = 2,
					.pQueueFamilyIndices   = queueFamilyIndices
				}
			)
		};
		
		auto const requirements { bufferHandle.getMemoryRequirements() };
		
		vk::MemoryAllocateInfo const allocateInfo {
			.allocationSize  = requirements.size,
			.memoryTypeIndex = findMemoryTypeIndex( requirements.memoryTypeBits, properties )
		};
		                 
		spdlog::info( "... allocating buffer device memory" );
		auto bufferMemory {
			vk::raii::DeviceMemory(
				std::move(
					mpDevice->allocateMemory(
						{
							.allocationSize  = requirements.size,
							.memoryTypeIndex = findMemoryTypeIndex(
							                      requirements.memoryTypeBits,
							                      properties
							                 )
						}
					)
				)
			)
		};
		
		
		spdlog::info( "... binding buffer device memory to buffer handle" );
		bufferHandle.bindMemory( *bufferMemory, 0 );
   
		return std::make_unique<Buffer>( std::move(bufferHandle), std::move(bufferMemory) );
	} // end-of-function: Renderer::makeBuffer
	
	
	
	void
	Renderer::copy( Buffer const &src, Buffer &dst, vk::DeviceSize const size )
	{
		spdlog::info( "Copying {} bytes of data from one buffer to another...", size );
		vk::raii::CommandBuffers commandBuffer(
			*mpDevice,
			vk::CommandBufferAllocateInfo {
				.commandPool        = **mpTransferCommandPool,
				.level              =   vk::CommandBufferLevel::ePrimary,
				.commandBufferCount =   1
			}
		);
		commandBuffer[0].begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandBuffer[0].copyBuffer( *src.handle, *dst.handle, {{.srcOffset=0, .dstOffset=0, .size=size}} );
		commandBuffer[0].end();
		vk::SubmitInfo const submitInfo {
			.commandBufferCount =  1,
			.pCommandBuffers    = &*commandBuffer[0]
		};
		mpTransferQueue->submit({ submitInfo });
		mpTransferQueue->waitIdle(); // TODO(later): use fence instead if allowing concurrent transfers
	} // end-of-function: Renderer::copy
	
	
	
	void
	Renderer::makeVertexBuffer()
	{
		spdlog::info( "Creating vertex buffer..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpDevice != nullptr );
		

		spdlog::info( "... creating staging buffer" );
		vk::DeviceSize const size { kRectangleVertices.size() * sizeof(Vertex2D) };
		auto stagingBuffer = makeBuffer(
			vk::BufferUsageFlagBits::eTransferSrc,
			size,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);
		
		spdlog::info( "... mapping staging buffer memory to CPU memory" );
		auto *mappedMemory { stagingBuffer->memory.mapMemory( 0, size ) };
		
		spdlog::info( "... copying vertex data to staging buffer's memory" );
		std::memcpy( mappedMemory, kRectangleVertices.data(), static_cast<std::size_t>(size) );
		// NOTE: if not using host coherent memory (which we are),
		// call flushMappedMemoryRanges here and invalidateMappedMemoryRanges before reading it
		spdlog::info( "... unmapping memory" );
		stagingBuffer->memory.unmapMemory();
		
		spdlog::info( "... creating vertex buffer" );
		mpVertexBuffer = makeBuffer(
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			size,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);
		spdlog::info( "... copying data from staging buffer memory to vertex buffer memory" );
		copy( *stagingBuffer, *mpVertexBuffer, size );
		spdlog::info( "... done!" );
	} // end-of-function: Renderer::makeVertexBuffer
	
	
	
	void
	Renderer::makeIndexBuffer()
	{
		spdlog::info( "Creating index buffer..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpDevice != nullptr );
		

		spdlog::info( "... creating staging buffer" );
		vk::DeviceSize const size { kRectangleIndices.size() * sizeof(u16) };
		auto stagingBuffer = makeBuffer(
			vk::BufferUsageFlagBits::eTransferSrc,
			size,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);
		
		spdlog::info( "... mapping staging buffer memory to CPU memory" );
		auto *mappedMemory { stagingBuffer->memory.mapMemory( 0, size ) };
	   
		spdlog::info( "... copying index data to staging buffer's memory" );
		std::memcpy( mappedMemory, kRectangleIndices.data(), static_cast<std::size_t>(size) );
		// NOTE: if not using host coherent memory (which we are),
		// call flushMappedMemoryRanges here and invalidateMappedMemoryRanges before reading it
		spdlog::info( "... unmapping memory" );
		stagingBuffer->memory.unmapMemory();
		
		spdlog::info( "... creating vertex buffer" );
		mpIndexBuffer = makeBuffer(
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			size,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);
		spdlog::info( "... copying data from staging buffer memory to index buffer memory" );
		copy( *stagingBuffer, *mpIndexBuffer, size );
		spdlog::info( "... done!" );
	} // end-of-function: Renderer::makeIndexBuffer
	
	
	
	void
	Renderer::makeCommandBuffers()
	{
		spdlog::info( "Creating command buffer(s)..." );
		
		// pre-condition(s):
		//   shouldn't be null unless the function is called in the wrong order:
		assert( mpDevice     != nullptr );
		assert( mpRenderPass != nullptr );
		
		mpCommandBuffers = makeCommandBuffers( vk::CommandBufferLevel::ePrimary, mFramebufferCount ); // TODO(1.0)
		vk::ClearValue const clearValue { .color = {{{ 0.02f, 0.02f, 0.02f, 1.0f }}} };
		
		for ( u32 index{0}; index < mFramebufferCount; ++index ) {
			auto &commandBuffer = (*mpCommandBuffers)[index];
			commandBuffer.begin( {} );
			commandBuffer.beginRenderPass(
				vk::RenderPassBeginInfo {
					.renderPass      = **mpRenderPass,
					.framebuffer     = *(mFramebuffers)[index],
					.renderArea      = vk::Rect2D {
					                    .extent = mSurfaceExtent,
					                 },
					.clearValueCount = 1, // TODO(explain)
					.pClearValues    = &clearValue
				},
				vk::SubpassContents::eInline // inline; no secondary command buffers allowed
			);
			commandBuffer.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				**mpGraphicsPipeline
			);
			// TODO(later): commandBuffer.bindDescriptorSets(
			// TODO(later): 	vk::PipelineBindPoint::eGraphics,
			// TODO(later): 	*pipelineLayout,
			// TODO(later): 	0,
			// TODO(later): 	{ *descriptorSet },
			// TODO(later): 	nullptr
			// TODO(later): );
			commandBuffer.bindVertexBuffers( 0, { *mpVertexBuffer->handle }, { 0 } );
			commandBuffer.bindIndexBuffer( *mpIndexBuffer->handle, 0, vk::IndexType::eUint16 );
			// NOTE(possibility): command_buffer.bindDescriptorSets()
			// NOTE(possibility): command_buffer.setViewport()
			// NOTE(possibility): command_buffer.setScissor()
			commandBuffer.drawIndexed(
				static_cast<u32>( kRectangleIndices.size() ), // index count
				1, // instance count
				0, // first index
				0, // vertex offset
				0  // first instance
			);
			commandBuffer.endRenderPass();
			commandBuffer.end();
		}
	} // end-of-function: Renderer::makeCommandBuffers
	
	
	
	void
	Renderer::makeSyncPrimitives()
	{
		// TODO(refactor)
		spdlog::info( "Creating synchronization primitive(s)..." );
		
		mImagePresentable .clear();
		mImageAvailable   .clear();
		mFencesInFlight   .clear();
		mImagePresentable .reserve( kMaxConcurrentFrames );
		mImageAvailable   .reserve( kMaxConcurrentFrames );
		mFencesInFlight   .reserve( kMaxConcurrentFrames );
		for ( auto i{0}; i < kMaxConcurrentFrames; ++i ) {
			mImagePresentable .emplace_back( mpDevice->createSemaphore({}) );
			mImageAvailable   .emplace_back( mpDevice->createSemaphore({}) );
			mFencesInFlight   .emplace_back( mpDevice->createFence({.flags = vk::FenceCreateFlagBits::eSignaled}) );
		}
	} // end-of-function: Renderer::makeSyncPrimitives()
	
	
	
	void
	Renderer::generateDynamicState()
	{
		// TODO(cleanup): Might need to clean up mpCommandBuffers, mDescriptors, whatever here (IMPORTANT!!!) TODO TODO TODO TODO TODO
		// TODO(cleanup): Might need to clean up mpCommandBuffers, mDescriptors, whatever here (IMPORTANT!!!) TODO TODO TODO TODO TODO
		// TODO(cleanup): Might need to clean up mpCommandBuffers, mDescriptors, whatever here (IMPORTANT!!!) TODO TODO TODO TODO TODO
		// TODO(cleanup): Might need to clean up mpCommandBuffers, mDescriptors, whatever here (IMPORTANT!!!) TODO TODO TODO TODO TODO
		// TODO(refactor)
		spdlog::debug( "Generating dynamic state..." );
		
		// delete previous state (if any) in the right order:
		mImages.clear();
		mImageViews.clear();
		mFramebuffers.clear();
		if ( mpCommandBuffers ) {
			mpCommandBuffers->clear();
			mpCommandBuffers.reset();
		}
		if ( mpGraphicsPipeline )
			mpGraphicsPipeline.reset();
		if ( mpSwapchain )
			mpSwapchain.reset();
		
		assert( mFramebuffers.empty()         );
		assert( mpCommandBuffers   == nullptr );
		assert( mpGraphicsPipeline == nullptr );
		assert( mpSwapchain        == nullptr );
		
		// handle minimization:
		mpWindow->waitResize();
		mpDevice->waitIdle();	
		
		makeSwapchain();
		makeGraphicsPipeline();
		makeFramebuffers();
		makeCommandBuffers();
	} // end-of-function: Renderer::generateDynamicState
	
	
	
	Renderer::Renderer():
		mShouldRemakeSwapchain { false },
		mCurrentFrame          { 0     }
	{
		spdlog::info( "Constructing a Renderer instance..." );
		// "fixed" part:
		mpGlfwInstance = std::make_unique<GlfwInstance>();
		makeVkContext();
		makeVkInstance();
		maybeMakeDebugMessenger();
		mpWindow = std::make_unique<Window>( *mpGlfwInstance, *mpVkInstance, mShouldRemakeSwapchain );
		selectPhysicalDevice();
		selectQueueFamilies(); // TODO: pick a better name
		makeLogicalDevice();
		makeQueues();
		makeCommandPools();
		// "dynamic" part:
		makeSwapchain();
		makeGraphicsPipeline();
		makeFramebuffers();
		makeVertexBuffer();
		makeIndexBuffer();
		makeCommandBuffers();
		makeSyncPrimitives(); // TODO(config): refactor so it is updated whenever framebuffer count changes
	//instance
	} // end-of-function: Renderer::Renderer
	
	
	
	Renderer::~Renderer() noexcept
	{
		spdlog::info( "Destroying a Renderer instance..." );
		mpDevice->waitIdle();
		if ( mpCommandBuffers )
			mpCommandBuffers->clear();
	} // end-of-function: Renderer::~Renderer
	
	
	
	[[nodiscard]] Window const &
	Renderer::getWindow() const
	{
		assert( mpWindow != nullptr );
		return *mpWindow;
	} // end-of-function: Renderer::getWindow
	
	
	
	[[nodiscard]] Window &
	Renderer::getWindow()
	{
		assert( mpWindow != nullptr );
		return *mpWindow;
	} // end-of-function: Renderer::getWindow
	
	
   
	void
	Renderer::operator()()
	{
		auto const frame = mCurrentFrame % kMaxConcurrentFrames;
		if constexpr ( kIsDebugMode ) spdlog::info( "[draw]: Drawing frame #{} (@{})...", mCurrentFrame, frame );
		
		{
			auto const waitResult {
				mpDevice->waitForFences( *mFencesInFlight[frame], VK_TRUE, kDrawWaitTimeout )
			};
			if ( waitResult != vk::Result::eSuccess )
				throw std::runtime_error { "Draw fence wait timed out!" }; // TEMP: handle eTimeout properly
		}
		
		u32 acquiredIndex;
		try {
			auto const [result, index] {
				mpSwapchain->acquireNextImage( kDrawWaitTimeout, *mImageAvailable[frame] )
			};
			acquiredIndex = index;
		}
		catch ( vk::OutOfDateKHRError const & ) {
			spdlog::info( "Swapchain out-of-date!" );
			generateDynamicState(); // likely having to handle a screen resize...
			return;
		}
		catch ( vk::SystemError const &e ) {
			spdlog::error( "Encountered system error: \"{}\"!", e.what() );
			throw std::runtime_error { "Failed to acquire swapchain image!" };
		}	
		
		try {
			mpDevice->resetFences( *mFencesInFlight[frame] );
			vk::PipelineStageFlags const waitDstStages ( vk::PipelineStageFlagBits::eColorAttachmentOutput );
			mpGraphicsQueue->submit(
				vk::SubmitInfo {
					.waitSemaphoreCount   = 1,
					.pWaitSemaphores      = &*mImageAvailable[frame],
					.pWaitDstStageMask    = &waitDstStages,
					.commandBufferCount   = 1,
					.pCommandBuffers      = &*(*mpCommandBuffers)[acquiredIndex],
					.signalSemaphoreCount = 1,
					.pSignalSemaphores    = &*mImagePresentable[frame], 
				},
				*mFencesInFlight[frame]
			);
		}
		catch ( vk::SystemError const &e ) {
			spdlog::error( "Encountered system error: \"{}\"!", e.what() );
			throw std::runtime_error { "Failed to submit draw command buffer!" };
		}
		
		vk::Result presentResult;
		try {
			presentResult = mpPresentQueue->presentKHR(
				vk::PresentInfoKHR {
					.waitSemaphoreCount = 1,
					.pWaitSemaphores    = &*mImagePresentable[frame],
					.swapchainCount     = 1,
					.pSwapchains        = &**mpSwapchain,
					.pImageIndices      = &acquiredIndex,
				}
			);
		}
		catch ( vk::OutOfDateKHRError const & ) {
			/* do nothing */
		}
		catch ( vk::SystemError const &e ) {
			spdlog::error( "Encountered system error: \"{}\"!", e.what() );
			throw std::runtime_error { "Failed to present swapchain image!" };
		}
		
		if ( mShouldRemakeSwapchain
//		or   presentResult == vk::Result::eSuboptimalKHR // TEMP disable
		or   presentResult == vk::Result::eErrorOutOfDateKHR )
		{
			spdlog::info( "Swapchain out-of-date or window resized!" );
			mShouldRemakeSwapchain = false;
			generateDynamicState();
			return; // TODO: verify that this is needed
		}
		
		++mCurrentFrame;
	} // end-of-function: Renderer::operator()
	
	
	
} // end-of-namespace: gfx
// EOF
