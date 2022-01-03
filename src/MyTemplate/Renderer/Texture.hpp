#pragma once // potentially faster compile-times if supported
#ifndef TEXTURE_HPP_LIQCQ7RZ
#define TEXTURE_HPP_LIQCQ7RZ

#include "MyTemplate/Common/aliases.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

struct Texture final
{
	vk::Image         image;
	vk::ImageView     view;    // needed for use within shaders
	vk::DeviceMemory  memory;  // needed for the image data
	vk::ImageLayout   layout;  // current memory layout
	vk::Sampler       sampler; // current sampler state
	u16               width;
	u16               height;
};

#endif // end-of-header-guard TEXTURE_HPP_LIQCQ7RZ
// EOF
