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
			~Renderer() noexcept;
			Renderer(             Renderer const &  )          = delete;
			Renderer(             Renderer       && ) noexcept = default;
			Renderer & operator=( Renderer const &  )          = delete;
			Renderer & operator=( Renderer       && ) noexcept = default;
			
			[[nodiscard]] Window const & getWindow() const;
			[[nodiscard]] Window       & getWindow();
			void operator()(); // renders
			
		private:
			void                                                    enableValidationLayers();
			void                                                    enableInstanceExtensions();
			[[nodiscard]] bool                                      meetsDeviceExtensionRequirements( vk::raii::PhysicalDevice const & ) const;
			[[nodiscard]] u32                                       calculateScore(                   vk::raii::PhysicalDevice const & ) const;
			void                                                    makeVkContext();
			void                                                    makeVkInstance();
			void                                                    selectPhysicalDevice();
			void                                                    maybeMakeDebugMessenger();
			void                                                    selectQueueFamilies();
			void                                                    makeLogicalDevice();
			[[nodiscard]] std::unique_ptr<vk::raii::Queue>          makeQueue( u32 const queueFamilyIndex, u32 const queueIndex );
			void                                                    makeGraphicsQueue();
			void                                                    makePresentQueue();
			void                                                    makeCommandPool();
			[[nodiscard]] std::unique_ptr<vk::raii::CommandBuffers> makeCommandBuffers( vk::CommandBufferLevel const, u32 const bufferCount );
			void                                                    selectSurfaceFormat();
			void                                                    selectSurfaceExtent();
			void                                                    selectPresentMode() noexcept;
			void                                                    selectFramebufferCount() noexcept;
			void                                                    generateDynamicState();
			void                                                    makeSwapchain();
			[[nodiscard]] std::unique_ptr<vk::raii::ShaderModule>   makeShaderModuleFromBinary( std::vector<char> const &shaderBinary ) const;
			[[nodiscard]] std::unique_ptr<vk::raii::ShaderModule>   makeShaderModuleFromFile( std::string const &shaderSpirvBytecodeFilename ) const;
			void                                                    makeGraphicsPipelineLayout();
			void                                                    makeRenderPass(); // TODO: rename?
			void                                                    makeGraphicsPipeline();
			[[nodiscard]] u32                                       findMemoryTypeIndex( u32 const typeFilter, vk::MemoryPropertyFlags const ) const;
			void                                                    makeVertexBuffers();
			void                                                    makeFramebuffers();
			void                                                    makeCommandBuffers();
			void                                                    makeSyncPrimitives();
			
			// NOTE: declaration order is very important here! (it dictates the order of destruction)
			std::vector<char const *>                            mValidationLayers                ;
			std::vector<char const *>                            mInstanceExtensions              ;
			std::unique_ptr<GlfwInstance>                        mpGlfwInstance                   ;
			std::unique_ptr<vk::raii::Context>                   mpVkContext                      ;
			std::unique_ptr<vk::raii::Instance>                  mpVkInstance                     ;
			#if !defined( NDEBUG )
				std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> mpDebugMessenger                 ;
			#endif
			std::unique_ptr<Window>                              mpWindow                         ;
			std::unique_ptr<vk::raii::PhysicalDevice>            mpPhysicalDevice                 ;
			QueueFamilyIndices                                   mQueueFamilyIndices              ;
			std::unique_ptr<vk::raii::Device>                    mpDevice                         ;
			std::unique_ptr<vk::raii::Queue>                     mpGraphicsQueue                  ;
			std::unique_ptr<vk::raii::Queue>                     mpPresentQueue                   ;
			std::unique_ptr<vk::raii::CommandPool>               mpCommandPool                    ;
			// dynamic:
			vk::SurfaceFormatKHR                                 mSurfaceFormat                   ;
			vk::SurfaceCapabilitiesKHR                           mSurfaceCapabilities             ;
			vk::Extent2D                                         mSurfaceExtent                   ;
			vk::PresentModeKHR                                   mPresentMode                     ;
			u32                                                  mFramebufferCount                ;
			std::unique_ptr<vk::raii::SwapchainKHR>              mpSwapchain                      ;
			std::vector<VkImage>                                 mImages                          ;
			std::vector<vk::raii::ImageView>                     mImageViews                      ;
			std::unique_ptr<vk::raii::Buffer>                    mpVertexBuffer                   ;
			std::unique_ptr<vk::raii::DeviceMemory>              mpVertexBufferMemory             ;
			std::unique_ptr<vk::raii::CommandBuffers>            mpCommandBuffers                 ; // NOTE: Must be deleted before command pool!
			std::unique_ptr<vk::raii::ShaderModule>              mpVertexShaderModule             ;
			std::unique_ptr<vk::raii::ShaderModule>              mpFragmentShaderModule           ;
			std::unique_ptr<vk::raii::PipelineLayout>            mpGraphicsPipelineLayout         ;
			std::unique_ptr<vk::raii::RenderPass>                mpRenderPass                     ;
			std::unique_ptr<vk::raii::Pipeline>                  mpGraphicsPipeline               ;
			std::vector<vk::raii::Framebuffer>                   mFramebuffers                    ; // NOTE: Must be deleted before swapchain!
			bool                                                 mShouldRemakeSwapchain           ;
			std::vector<vk::raii::Semaphore>                     mImageAvailable                  ;
			std::vector<vk::raii::Semaphore>                     mImagePresentable                ;
			std::vector<vk::raii::Fence>                         mFencesInFlight                  ; // TODO: better name
			u64                                                  mCurrentFrame                    ;
	}; // end-of-class: Renderer
} // end-of-namespace: gfx

#endif // end-of-header-guard RENDERER_HPP_YBLYHOXN
// EOF
