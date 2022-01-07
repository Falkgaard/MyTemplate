#pragma once // potentially faster compile-times if supported
#ifndef COMMON_HPP_8A3EZGTJ
#define COMMON_HPP_8A3EZGTJA

#include "MyTemplate/info.hpp"
#include "MyTemplate/Common/aliases.hpp"
#include "MyTemplate/Common/utility.hpp"

// TODO: split enums, structs, and forward declarations into separate files and include them
// TODO: move relevant stuff into specific classes (e.g. Renderer)

// forward declarations:
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
		}; // end-of-enum-struct: gfx::FramebufferingPriority
		
		enum struct PresentationPriority {
			eMinimalLatency         ,
			eMinimalStuttering      ,
			eMinimalPowerConsumption,
		}; // end-of-enum-struct: gfx::PresentationPriority
	} // end-of-namespace: gfx


// POD structs:
	namespace gfx {
		struct QueueFamilyIndices {
			static inline u32 constexpr kUndefined { max<u32> };
			u32  mPresentIndex  { kUndefined };
			u32  mGraphicsIndex { kUndefined };
			bool mbSeparate     { false      };
		}; // end-of-struct: gfx::QueueFamilyIndices
	} // end-of-namespace: gfx


#endif // end-of-header-guard: COMMON_HPP_8A3EZGTJ
// EOF
