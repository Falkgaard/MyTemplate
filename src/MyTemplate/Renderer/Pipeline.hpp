#pragma once // potentially faster compile-times if supported
#ifndef PIPELINE_HPP_K7YBFDM9
#define PIPELINE_HPP_K7YBFDM9

// forward declarations:
class Swapchain;
namespace vk::raii {
	class Device;
} // end-of-namespace: vk::raii

class Pipeline {
	public:
		Pipeline( vk::raii::Device &logical_device, Swapchain &swapchain );
		~Pipeline() noexcept;
	private:
};

#endif // end-of-header-guard PIPELINE_HPP_K7YBFDM9
// EOF
