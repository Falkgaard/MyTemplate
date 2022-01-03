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
namespace vk::raii {
	class PhysicalDevice;
} // end-of-namespace: vk::raii

class Swapchain final {
	public:
		Swapchain( vk::raii::PhysicalDevice &, vk::raii::Device &, Window &, QueueFamilyIndices & );
		Swapchain(                    ) = delete;
		Swapchain( Swapchain const &  ) = delete;
		Swapchain( Swapchain       && ) noexcept;
		~Swapchain()                    noexcept;
		// TODO: assignment ops?
	private:
		// NOTE: declaration order here matters!
		vk::SurfaceFormatKHR              m_surface_format       ;
		vk::SurfaceCapabilitiesKHR        m_surface_capabilities ;
		vk::Extent2D                      m_surface_extent       ;
		vk::PresentModeKHR                m_present_mode         ;
		u32                               m_framebuffer_count    ;
		std::vector<vk::Image>            m_images               ;
		std::vector<vk::raii::ImageView>  m_image_views          ;
};

#endif // end-of-header-guard SWAPCHAIN_HPP_WNJK5MZX
// EOF
