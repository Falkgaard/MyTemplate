#include "MyTemplate/Renderer/Swapchain.hpp"

#include "MyTemplate/Renderer/Renderer.hpp"
#include "MyTemplate/Renderer/Window.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <limits>
#include <spdlog/spdlog.h>

namespace gfx {
	namespace { // private (file-scope)
		
		[[nodiscard]] auto
		select_surface_format( vk::raii::PhysicalDevice const &physical_device, Window const &window )
		{
			spdlog::info( "Selecting swapchain surface format..." );
			auto const available_surface_formats {
				physical_device.getSurfaceFormatsKHR( *window.get_surface() ) // TODO: 2KHR?
			};
			for ( auto const &available_surface_format: available_surface_formats ) {
				if ( available_surface_format.format     == vk::Format::eB8G8R8A8Srgb               // TODO: refactor out
				and  available_surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear ) [[unlikely]] { // ^ ditto
					spdlog::info(
						"... selected surface format `{}` in color space `{}`",
						to_string( available_surface_format.format     ),
						to_string( available_surface_format.colorSpace )
					);
					return available_surface_format;
				}
			}
			// TODO: add contingency decisions to fallback on
			throw std::runtime_error { "Unable to find the desired surface format!" };	
		} // end-of-function: select_surface_format
		
		[[nodiscard]] auto
		select_surface_extent( Window const &window, vk::SurfaceCapabilitiesKHR const &surface_capabilities )
		{
			spdlog::info( "Selecting swapchain surface extent..." );
			vk::Extent2D result {};
			if ( surface_capabilities.currentExtent.height == std::numeric_limits<u32>::max() ) // TODO: unlikely? likely?
				result = surface_capabilities.currentExtent;
			else {
				auto [width, height] = window.get_dimensions();
				result.width =
					std::clamp(
						static_cast<u32>(width),
						surface_capabilities.minImageExtent.width,
						surface_capabilities.maxImageExtent.width
					);
				result.height =
					std::clamp(
						static_cast<u32>(height),
						surface_capabilities.minImageExtent.height,
						surface_capabilities.maxImageExtent.height
					);
			}
			spdlog::info( "... selected swapchain extent: {}x{}", result.width, result.height );
			return result;
		} // end-of-function: select_surface_extent
		
		[[nodiscard]] auto
		select_present_mode(
			vk::raii::PhysicalDevice const &physical_device,
			vk::raii::SurfaceKHR     const &surface,
			PresentationPriority     const  presentation_priority
		) noexcept
		{
			// TODO: add support for ordered priorities instead of just ideal/fallback.
			spdlog::info( "Selecting swapchain present mode..." );
			auto const fallback_present_mode {
				vk::PresentModeKHR::eFifo
			};
			auto const available_present_modes {
				physical_device.getSurfacePresentModesKHR( *surface )
			};
			auto const ideal_present_mode {
				[presentation_priority] {
					switch ( presentation_priority ) {
						case PresentationPriority::eMinimalLatency          : return vk::PresentModeKHR::eMailbox;
						case PresentationPriority::eMinimalStuttering       : return vk::PresentModeKHR::eFifoRelaxed;
						case PresentationPriority::eMinimalPowerConsumption : return vk::PresentModeKHR::eFifo;
					}
					unreachable();
				}()
			};
			bool const has_support_for_ideal_mode {
				std::ranges::find( available_present_modes, ideal_present_mode )
				!= std::end( available_present_modes )
			};
			if ( has_support_for_ideal_mode ) [[likely]] {
				spdlog::info(
					"... ideal present mode `{}` is supported by device!",
					to_string( ideal_present_mode )
				);
				return ideal_present_mode;
			}
			else [[unlikely]] {
				spdlog::warn(
					"... ideal present mode is not supported by device; using fallback present mode `{}`!",
					to_string( fallback_present_mode )
				);
				return fallback_present_mode;
			}
		} // end-of-function: select_present_mode
		
		[[nodiscard]] auto
		select_framebuffer_count(
			vk::SurfaceCapabilitiesKHR const &surface_capabilities,
			FramebufferingPriority     const  framebuffering_priority
		) noexcept
		{
			// TODO: maybe add 1 to the ideal framebuffer count..?
			spdlog::info( "Selecting swapchain framebuffer count..." );
			auto const ideal_framebuffer_count {
				static_cast<u32>( framebuffering_priority )
			};
			spdlog::info( "... ideal framebuffer count: {}", ideal_framebuffer_count );
			auto const minimum_framebuffer_count {
				surface_capabilities.minImageCount
			};
			auto const maximum_framebuffer_count {
				surface_capabilities.maxImageCount == 0 ? // handle special 0 (uncapped) case
					ideal_framebuffer_count : surface_capabilities.maxImageCount
			};
			auto result {
				std::clamp(
					ideal_framebuffer_count,
					minimum_framebuffer_count,
					maximum_framebuffer_count
				)
			};
			spdlog::info( "... nearest available framebuffer count: {}", result );
			return result;
		} // end-of-function: select_framebuffer_count
		
	} // end-of-unnamed-namespace
	
	Swapchain::Swapchain(
		vk::raii::PhysicalDevice const &physical_device,
		vk::raii::Device         const &logical_device,
		Window                   const &window,
		QueueFamilyIndices       const &queue_family_indices
	)
	{
		spdlog::info( "Constructing a Swapchain instance..." );
		
		m_surface_format       = select_surface_format( physical_device, window );
		m_surface_capabilities = physical_device.getSurfaceCapabilitiesKHR( *window.get_surface() );
		m_surface_extent       = select_surface_extent( window, m_surface_capabilities );
		m_present_mode         = select_present_mode( physical_device, window.get_surface(), PresentationPriority::eMinimalStuttering );
		m_framebuffer_count    = select_framebuffer_count( m_surface_capabilities, FramebufferingPriority::eTriple );
		
		// temporary array:
		u32 queue_family_indices_array[] {
			queue_family_indices.present,
			queue_family_indices.graphics
		};
		// TODO: check present support	
		m_p_swapchain = std::make_unique<vk::raii::SwapchainKHR>(
			logical_device,
			vk::SwapchainCreateInfoKHR {
				.surface               = *window.get_surface(),
				.minImageCount         =  m_framebuffer_count,         // TODO: config refactor
				.imageFormat           =  m_surface_format.format,     // TODO: config refactor
				.imageColorSpace       =  m_surface_format.colorSpace, // TODO: config refactor
				.imageExtent           =  m_surface_extent,
				.imageArrayLayers      =  1, // non-stereoscopic
				.imageUsage            =  vk::ImageUsageFlagBits::eColorAttachment, // NOTE: change to eTransferDst if doing post-processing later
				.imageSharingMode      =  queue_family_indices.are_separate ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
				.queueFamilyIndexCount =  queue_family_indices.are_separate ?                           2u : 0u,
				.pQueueFamilyIndices   =  queue_family_indices.are_separate ?   queue_family_indices_array : nullptr,
				.preTransform          =  m_surface_capabilities.currentTransform,
				.compositeAlpha        =  vk::CompositeAlphaFlagBitsKHR::eOpaque,
				.presentMode           =  m_present_mode, // TODO: config refactor
				.clipped               =  VK_TRUE,
				.oldSwapchain          =  VK_NULL_HANDLE // TODO: revisit later after implementing resizing
			}
		);
		
		// Image views:
		spdlog::info( "Creating swapchain framebuffer image view(s)..." );
		m_images = m_p_swapchain->getImages();
		m_image_views.reserve( std::size( m_images ) );
		for ( auto const &image: m_images ) {
			m_image_views.emplace_back(
				logical_device,
				vk::ImageViewCreateInfo {
					.image            = static_cast<vk::Image>( image ),
					.viewType         = vk::ImageViewType::e2D,
					.format           = m_surface_format.format, // TODO: verify
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
	}
	
	Swapchain::Swapchain( Swapchain &&other ) noexcept:
		m_surface_format       ( other.m_surface_format             ),
		m_surface_capabilities ( other.m_surface_capabilities       ),
		m_surface_extent       ( other.m_surface_extent             ),
		m_present_mode         ( other.m_present_mode               ),
		m_framebuffer_count    ( other.m_framebuffer_count          ),
		m_p_swapchain          ( std::move( other.m_p_swapchain   ) ),
		m_images               ( std::move( other.m_images        ) ),
		m_image_views          ( std::move( other.m_image_views   ) )
	{
		spdlog::info( "Moving a Swapchain instance..." );
	}
	
	Swapchain::~Swapchain() noexcept
	{
		spdlog::info( "Destroying a Swapchain instance..." );
	}
	
	[[nodiscard]] vk::Extent2D const &
	Swapchain::get_surface_extent() const {
		return m_surface_extent;
	}
	
	[[nodiscard]] vk::SurfaceFormatKHR const &
	Swapchain::get_surface_format() const {
		return m_surface_format;
	}
	
	[[nodiscard]] std::vector<vk::raii::ImageView> const &
	Swapchain::get_image_views() const
	{
		return m_image_views;
	}
	
	[[nodiscard]] vk::raii::SwapchainKHR const &
	Swapchain::access() const
	{
		spdlog::debug( "Accessing swapchain..." );
		return *m_p_swapchain;
	} // end-of-function: gfx::Swapchain::access
	
	[[nodiscard]] vk::raii::SwapchainKHR &
	Swapchain::access()
	{
		spdlog::debug( "Accessing swapchain..." );
		return *m_p_swapchain;
	} // end-of-function: gfx::Swapchain::access
	
} // end-of-namespace: gfx	
// EOF
