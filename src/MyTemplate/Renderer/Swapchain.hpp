#pragma once // potentially faster compile-times if supported
#ifndef SWAPCHAIN_HPP_WNJK5MZX
#define SWAPCHAIN_HPP_WNJK5MZX

#include "MyTemplate/Renderer/common.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>

namespace gfx {
class Swapchain final {
		public:
			Swapchain( vk::raii::PhysicalDevice const &, vk::raii::Device const &, Window const &, QueueFamilyIndices const & );
			Swapchain(                    ) = delete;
			Swapchain( Swapchain const &  ) = delete;
			Swapchain( Swapchain       && ) noexcept;
			~Swapchain()                    noexcept;
			// TODO: assignment ops?
			[[nodiscard]] vk::Extent2D                     const & get_surface_extent() const;
			[[nodiscard]] vk::SurfaceFormatKHR             const & get_surface_format() const;
			[[nodiscard]] std::vector<vk::raii::ImageView> const & get_image_views()    const; // TODO: rename/refactor?
		private:
			// NOTE: declaration order here matters!
			vk::SurfaceFormatKHR                    m_surface_format       ;
			vk::SurfaceCapabilitiesKHR              m_surface_capabilities ;
			vk::Extent2D                            m_surface_extent       ;
			vk::PresentModeKHR                      m_present_mode         ;
			u32                                     m_framebuffer_count    ;
			std::unique_ptr<vk::raii::SwapchainKHR> m_p_swapchain          ;
			std::vector<VkImage>                    m_images               ;
			std::vector<vk::raii::ImageView>        m_image_views          ;
	};
} // end-of-namespace: gfx

#endif // end-of-header-guard SWAPCHAIN_HPP_WNJK5MZX
// EOF
