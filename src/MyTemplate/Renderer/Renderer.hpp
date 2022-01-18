#pragma once // potentially faster compile-times if supported
#ifndef RENDERER_HPP_YBLYHOXN
#define RENDERER_HPP_YBLYHOXN

#include "MyTemplate/Renderer/common.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>
#include <chrono>

// #include <glm/common.hpp>
// #include <glm/exponential.hpp>
// #include <glm/geometric.hpp>
#include <glm/matrix.hpp>
// #include <glm/trigonometric.hpp>
// #include <glm/ext/vector_float3.hpp>
// #include <glm/ext/matrix_float4x4.hpp>
// #include <glm/ext/matrix_clip_space.hpp>
// #include <glm/ext/scalar_constants.hpp>

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
			// functions:
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
			void                                                    makeQueues();
			void                                                    makeCommandPools();
			[[nodiscard]] std::unique_ptr<vk::raii::CommandBuffers> makeCommandBuffers( vk::CommandBufferLevel const, u32 const bufferCount );
			void                                                    selectSurfaceFormat();
			void                                                    selectSurfaceExtent();
			void                                                    selectPresentMode() noexcept;
			void                                                    selectFramebufferCount() noexcept;
			void                                                    makeDynamicState();
			void                                                    wipeDynamicState();
			void                                                    remakeDynamicState();
			void                                                    makeSwapchain();
			[[nodiscard]] std::unique_ptr<vk::raii::ShaderModule>   makeShaderModuleFromBinary( std::vector<char> const &shaderBinary ) const;
			[[nodiscard]] std::unique_ptr<vk::raii::ShaderModule>   makeShaderModuleFromFile( std::string const &shaderSpirvBytecodeFilename ) const;
			void                                                    makeDescriptorSetLayout();
			void                                                    makeGraphicsPipelineLayout();
			void                                                    makeRenderPass(); // TODO: rename?
			void                                                    makeGraphicsPipeline();
			[[nodiscard]] u32                                       findMemoryTypeIndex( u32 const typeFilter, vk::MemoryPropertyFlags const ) const;
			[[nodiscard]] std::unique_ptr<Buffer>                   makeBuffer( vk::BufferUsageFlags const, vk::DeviceSize const, vk::MemoryPropertyFlags const );
			void                                                    copy( Buffer const &src, Buffer &dst, vk::DeviceSize const );
			void                                                    makeImage();
			void                                                    makeTextureImage();
			void                                                    makeVertexBuffer();
			void                                                    makeIndexBuffer();
			void                                                    makeUniformBuffers();
			void                                                    makeDescriptorPool();
			void                                                    makeFramebuffers();
			void                                                    makeDescriptorSets();
			void                                                    makeCommandBuffers();
			void                                                    makeSyncPrimitives();
			void                                                    updateUniformBuffer( u32 bufferIndex );
			void                                                    initializeData();
			
			// structs:
			struct HostData { // CPU-only
				std::chrono::system_clock::time_point  startTime;
				glm::mat4                              model; // NOTE: will be replaced with model-local data
				glm::mat4                              view;
				glm::mat4                              projection;
				glm::mat4                              clip;
			};
			
			struct UniformBufferObject {
				glm::mat4 mvp;
			};
			
			// data members:          (NOTE: declaration/destruction order is very important here!)
			// 
			// fixed state:
			HostData                                             mData                            ;
			UniformBufferObject                                  mUbo                             ;
			std::vector<char const *>                            mValidationLayers                ;
			std::vector<char const *>                            mInstanceExtensions              ;
			std::unique_ptr<GlfwInstance>                        mpGlfwInstance                   ;
			std::unique_ptr<vk::raii::Context>                   mpVkContext                      ;
			std::unique_ptr<vk::raii::Instance>                  mpVkInstance                     ;
			#if !defined( NDEBUG ) // TODO: use app-specific define!
				std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> mpDebugMessenger                 ;
			#endif
			std::unique_ptr<Window>                              mpWindow                         ;
			std::unique_ptr<vk::raii::PhysicalDevice>            mpPhysicalDevice                 ;
			QueueFamilyIndices                                   mQueueFamilyIndices              ;
			std::unique_ptr<vk::raii::Device>                    mpDevice                         ; // TODO: rename?
			std::unique_ptr<vk::raii::Queue>                     mpGraphicsQueue                  ;
			std::unique_ptr<vk::raii::Queue>                     mpPresentQueue                   ;
			std::unique_ptr<vk::raii::Queue>                     mpTransferQueue                  ;
			std::unique_ptr<vk::raii::DescriptorPool>            mpDescriptorPool                 ;
			std::unique_ptr<vk::raii::CommandPool>               mpGraphicsCommandPool            ;
			std::unique_ptr<vk::raii::CommandPool>               mpTransferCommandPool            ;
			// dynamic state:
			vk::SurfaceFormatKHR                                 mSurfaceFormat                   ;
			vk::SurfaceCapabilitiesKHR                           mSurfaceCapabilities             ;
			vk::Extent2D                                         mSurfaceExtent                   ;
			vk::PresentModeKHR                                   mPresentMode                     ;
			u32                                                  mFramebufferCount                ;
			std::unique_ptr<vk::raii::SwapchainKHR>              mpSwapchain                      ;
			std::vector<VkImage>                                 mImages                          ; // TODO: rename/refactor?
			std::vector<vk::raii::ImageView>                     mImageViews                      ; // TODO: rename/refactor?
			std::unique_ptr<vk::raii::Image>                     mpTextureImage                   ;
			std::unique_ptr<Buffer>                              mpVertexBuffer                   ;
			std::unique_ptr<Buffer>                              mpIndexBuffer                    ;
			std::vector<std::unique_ptr<Buffer>>                 mUniformBuffers                  ; // on per swapchain frame (TODO: remove uptr?)
			std::unique_ptr<vk::raii::CommandBuffers>            mpCommandBuffers                 ; // NOTE: Must be deleted before command pool!
			std::unique_ptr<vk::raii::ShaderModule>              mpVertexShaderModule             ;
			std::unique_ptr<vk::raii::ShaderModule>              mpFragmentShaderModule           ;
			std::unique_ptr<vk::raii::PipelineLayout>            mpGraphicsPipelineLayout         ;
			std::unique_ptr<vk::raii::RenderPass>                mpRenderPass                     ;
			std::unique_ptr<vk::raii::DescriptorSetLayout>       mpDescriptorSetLayout            ;
			std::unique_ptr<vk::raii::DescriptorSets>            mpDescriptorSets                 ;
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
