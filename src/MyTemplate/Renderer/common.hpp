#pragma once // potentially faster compile-times if supported
#ifndef COMMON_HPP_8A3EZGTJ
#define COMMON_HPP_8A3EZGTJA

#include "MyTemplate/info.hpp"
#include "MyTemplate/Common/aliases.hpp"
#include "MyTemplate/Common/utility.hpp"

// TODO: split enums, structs, and forward declarations into separate files and include them

// forward declarations:
	class GLFWwindow;
//	class VkImage;

namespace vk {
	// forward declarations:
//		class Extent2D;
//		class PresentModeKHR;
//		class SurfaceCapabilitiesKHR;
//		class SurfaceFormatKHR;
	
	namespace raii {
		// forward declarations:
			class CommandPool;
			class CommandBuffers;
			class Context;
			class DebugUtilsMessengerEXT;
			class Device;
			class ImageView;
			class Instance;
			class PhysicalDevice;
			class Pipeline;
			class PipelineLayout;
			class Queue;
			class RenderPass;
			class ShaderModule;
			class SurfaceKHR;
			class SwapchainKHR;
		
	} // end-of-namespace: vk::raii
} // end-of-namespace: vk

namespace gfx {
	// forward declarations:
		class GlfwInstance;
		class VkInstance;
		class Window;
		class Swapchain;
		class Pipeline;
		struct QueueFamilyIndices; // refactor definition into POD structs instead?
	
	// enums:
		// NOTE: value corresponds to number of frames to allow for static casting
		enum struct FramebufferingPriority: u32 {
			eSingle = 1,
			eDouble = 2,
			eTriple = 3 
		}; // end-of-enum-struct: FramebufferingPriority
		
		enum struct PresentationPriority {
			eMinimalLatency,        
			eMinimalStuttering,     
			eMinimalPowerConsumption
		}; // end-of-enum-struct: PresentationPriority
	
	// POD structs:
		struct QueueFamilyIndices {
			u32  present;
			u32  graphics;
			bool are_separate;
		}; // end-of-struct: QueueFamilyIndices
} // end-of-namespace: vk::raii

#endif // end-of-header-guard: COMMON_HPP_8A3EZGTJ
// EOF
