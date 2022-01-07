#pragma once // potentially faster compile-times if supported
#ifndef RENDERER_HPP_YBLYHOXN
#define RENDERER_HPP_YBLYHOXN

#include "MyTemplate/Renderer/common.hpp"	

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>

namespace gfx {
	class Renderer final {
		public:
			Renderer();
			Renderer( Renderer const & )                 = delete;
			Renderer & operator=( Renderer const &  )    = delete;
			Renderer( Renderer && ) noexcept             = default;
			~Renderer() noexcept                         = default;
			Renderer & operator=( Renderer && ) noexcept = default;
			
			[[nodiscard]] Window const & get_window() const;
			[[nodiscard]] Window       & get_window();
			void operator()(); // renders
			
		private:
			void enableValidationLayers( vk::InstanceCreateInfo & );
			
			
			
			
			
			
			
			// NOTE: declaration order is very important here! (it dictates the order of destruction)
			std::unique_ptr<GlfwInstance>                         mpGlfwInstance         ;
			std::unique_ptr<vk::raii::Context>                    mpVkContext            ;
			std::unique_ptr<vk::raii::Instance>                   mpVkInstance           ;
			#if !defined( NDEBUG )
				std::unique_ptr<vk::raii::DebugUtilsMessengerEXT>  mpDebugMessenger       ;
			#endif
			std::unique_ptr<Window>                               mpWindow               ;
			std::unique_ptr<vk::raii::PhysicalDevice>             mpPhysicalDevice       ;
			QueueFamilyIndices                                    mqueueFamilies         ;
			std::unique_ptr<vk::raii::Device>                     mpDevice               ;
			std::unique_ptr<vk::raii::Queue>                      mpGraphicsQueue        ;
			std::unique_ptr<vk::raii::Queue>                      mpPresentQueue         ;
			std::unique_ptr<vk::raii::CommandPool>                mpCommandPool          ;
			// dynamic:
			vk::SurfaceFormatKHR                                  mSurfaceFormat         ;
			vk::SurfaceCapabilitiesKHR                            mSurfaceCapabilities   ;
			vk::Extent2D                                          mSurfaceExtent         ;
			vk::PresentModeKHR                                    mPresentMode           ;
			u32                                                   mFramebufferCount      ;
			std::unique_ptr<vk::raii::SwapchainKHR>               mpSwapchain            ;
			std::vector<VkImage>                                  mImages                ;
			std::vector<vk::raii::ImageView>                      mImageViews            ;
			std::unique_ptr<vk::raii::CommandBuffers>             mpCommandBuffers       ; // NOTE: Must be deleted before command pool!
			std::unique_ptr<vk::raii::ShaderModule>               mpVertexShaderModule   ;
			std::unique_ptr<vk::raii::ShaderModule>               mpFragmentShaderModule ;
			std::unique_ptr<vk::raii::PipelineLayout>             mpPipelineLayout       ;
			std::unique_ptr<vk::raii::RenderPass>                 mpRenderPass           ;
			std::unique_ptr<vk::raii::Pipeline>                   mpGraphicsPipeline     ;
			std::vector<vk::raii::Framebuffer>                    mFramebuffers          ; // NOTE: Must be deleted before swapchain!
			bool                                                  mbBadSwapchain         ;
			std::vector<vk::raii::Semaphore>                      mImageAvailable        ;
			std::vector<vk::raii::Semaphore>                      mImagePresentable      ;
			std::vector<vk::raii::Fence>                          mImagesInFlight        ; // TODO: better names
			std::vector<vk::raii::Fence>                          mFencesInFlight        ; // TODO: better names
			u64                                                   mCurrentFrame          ;
	}; // end-of-class: Renderer
} // end-of-namespace: gfx

#endif // end-of-header-guard RENDERER_HPP_YBLYHOXN
// EOF
