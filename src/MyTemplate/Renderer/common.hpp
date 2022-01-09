#pragma once // potentially faster compile-times if supported
#ifndef COMMON_HPP_8A3EZGTJ
#define COMMON_HPP_8A3EZGTJA

#include "MyTemplate/info.hpp"
#include "MyTemplate/Common/aliases.hpp"
#include "MyTemplate/Common/utility.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

// TODO: split enums, structs, and forward declarations into separate files and include them
// TODO: move relevant stuff into specific classes (e.g. Renderer)

// forward declarations: (TODO: remove unused ones?)
	class GLFWwindow;
	namespace vk::raii {
		class CommandPool;
		class CommandBuffers;
		class Context;
		class DebugUtilsMessengerEXT;
		class Device;
		class Fence;
		class Framebuffer;
		class ImageView;
		class Instance;
		class PhysicalDevice;
		class Pipeline;
		class PipelineLayout;
		class Queue;
		class RenderPass;
		class ShaderModule;
		class Semaphore;
		class SurfaceKHR;
		class SwapchainKHR;
	} // end-of-namespace: vk::raii
	namespace gfx {
		class GlfwInstance;
		class Window      ;
		class Renderer    ;
	} // end-of-namespace: gfx


// enums:
	namespace gfx {
		// NOTE: value corresponds to number of frames to allow for static casting
		enum struct FramebufferingPriority: u32 {
			eSingle = 1,
			eDouble = 2,
			eTriple = 3,
		}; // end-of-enum-struct: FramebufferingPriority
		
		enum struct PresentationPriority {
			eMinimalLatency         ,
			eMinimalStuttering      ,
			eMinimalPowerConsumption,
		}; // end-of-enum-struct: PresentationPriority
	} // end-of-namespace: gfx


// POD structs:
	namespace gfx {
		struct QueueFamilyIndices {
			inline static u32 constexpr kUndefined { max<u32> };
			u32  presentIndex  { kUndefined };
			u32  graphicsIndex { kUndefined };
			u32  transferIndex { kUndefined };
			bool areSeparate   { false      };
		}; // end-of-struct: QueueFamilyIndices
		
		struct Buffer {
			vk::raii::Buffer        handle;
			vk::raii::DeviceMemory  memory;
		}; // end-of-struct: Buffer
	} // end-of-namespace: gfx


#endif // end-of-header-guard: COMMON_HPP_8A3EZGTJ
// EOF
