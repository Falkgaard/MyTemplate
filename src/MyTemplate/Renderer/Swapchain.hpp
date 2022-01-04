#pragma once // potentially faster compile-times if supported
#ifndef SWAPCHAIN_HPP_WNJK5MZX
#define SWAPCHAIN_HPP_WNJK5MZX

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "MyTemplate/Common/aliases.hpp"
#include <memory>
#include <vector>

// forward declarations:
class Window;
struct QueueFamilyIndices;

class Swapchain final {
	public:
		Swapchain( vk::raii::PhysicalDevice const &, vk::raii::Device const &, Window const &, QueueFamilyIndices const & );
		Swapchain(                    ) = delete;
		Swapchain( Swapchain const &  ) = delete;
		Swapchain( Swapchain       && ) noexcept;
		~Swapchain()                    noexcept;
		// TODO: assignment ops?
		[[nodiscard]] vk::Extent2D         const & get_surface_extent() const;
		[[nodiscard]] vk::SurfaceFormatKHR const & get_surface_format() const;
	private:
		// NOTE: declaration order here matters!
		vk::SurfaceFormatKHR              m_surface_format       ;
		vk::SurfaceCapabilitiesKHR        m_surface_capabilities ;
		vk::Extent2D                      m_surface_extent       ;
		vk::PresentModeKHR                m_present_mode         ;
		u32                               m_framebuffer_count    ;
		vk::raii::SwapchainKHR            m_swapchain            ;
		std::vector<VkImage>              m_images               ;
		std::vector<vk::raii::ImageView>  m_image_views          ;
};

#endif // end-of-header-guard SWAPCHAIN_HPP_WNJK5MZX
// EOF
