#pragma once // potentially faster compile-times if supported
#ifndef PRIMITIVES_HPP_7NQRHOKZ
#define PRIMITIVES_HPP_7NQRHOKZ

// #include <glm/common.hpp>
// #include <glm/exponential.hpp>
// #include <glm/geometric.hpp>
// #include <glm/matrix.hpp>
// #include <glm/trigonometric.hpp>
// #include <glm/ext/matrix_float4x4.hpp>
// #include <glm/ext/matrix_transform.hpp>
// #include <glm/ext/matrix_clip_space.hpp>
// #include <glm/ext/scalar_constants.hpp><

#include "MyTemplate/Common/aliases.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

#include <array>
#include <cstddef> // offsetof

namespace gfx {
	// TODO(later): experiment with de-interleaved vertex attributes
	
	struct Vertex2D {
		glm::vec2 xy;  // 2D position
		glm::vec3 rgb; // colour
		static const vk::VertexInputBindingDescription                 kVertexInputBindingDescription; 
		static const std::array<vk::VertexInputAttributeDescription,2> kVertexInputAttributeDescriptions;
	}; // end-of-struct: Vertex2D
	
	inline const vk::VertexInputBindingDescription
	Vertex2D::kVertexInputBindingDescription {
		vk::VertexInputBindingDescription {
			.binding   = 0, // TODO(doc): pipeline pBuffers index?
			.stride    = sizeof(Vertex2D),
			.inputRate = vk::VertexInputRate::eVertex // eInstance is another option
		}
	};
	
	inline const std::array<vk::VertexInputAttributeDescription,2>
	Vertex2D::kVertexInputAttributeDescriptions {
		{ // XY:
			vk::VertexInputAttributeDescription {
				.location  = 0, // vertex shader location
				.binding   = 0, // TODO(doc): pipeline pBuffers index?
				.format    = vk::Format::eR32G32Sfloat, // 2 x 32-bit floats
				.offset    = offsetof( Vertex2D, xy )
			}, // RGB:
			vk::VertexInputAttributeDescription {
				.location  = 1, // vertex shader location
				.binding   = 0, // TODO(doc): pipeline pBuffers index? 
				.format    = vk::Format::eR32G32B32Sfloat, // 3 x 32-bit floats
				.offset    = offsetof( Vertex2D, rgb )
			}
		}
	};
	
	
	
	struct Vertex3D {
		glm::vec3 xyz; // 3D position
		glm::vec3 rgb; // colour
		static const vk::VertexInputBindingDescription                 kVertexInputBindingDescription; 
		static const std::array<vk::VertexInputAttributeDescription,2> kVertexInputAttributeDescriptions;
	}; // end-of-struct: Vertex3D
	
	inline const vk::VertexInputBindingDescription
	Vertex3D::kVertexInputBindingDescription {
		vk::VertexInputBindingDescription {
			.binding   = 0, // TODO(doc): pipeline pBuffers index?
			.stride    = sizeof(Vertex3D),
			.inputRate = vk::VertexInputRate::eVertex // eInstance is another option
		}
	};
	
	inline const std::array<vk::VertexInputAttributeDescription,2>
	Vertex3D::kVertexInputAttributeDescriptions {
		{ // XYZ:
			vk::VertexInputAttributeDescription {
				.location  = 0, // vertex shader location
				.binding   = 0, // TODO(doc): pipeline pBuffers index?
				.format    = vk::Format::eR32G32B32Sfloat, // 3 x 32-bit floats
				.offset    = offsetof( Vertex3D, xyz )
			}, // RGB:
			vk::VertexInputAttributeDescription {
				.location  = 1, // vertex shader location
				.binding   = 0, // TODO(doc): pipeline pBuffers index? 
				.format    = vk::Format::eR32G32B32Sfloat, // 3 x 32-bit floats
				.offset    = offsetof( Vertex3D, rgb )
			}
		}
	};
	
	
	// Temp for test1 (TODO: remove)
	static std::array constexpr kCubeVertices {
		/*0*/ Vertex3D { .xyz = { -0.5f, -0.5f, -0.5f }, .rgb = { 1.0f, 0.0f, 1.0f } },
		/*1*/ Vertex3D { .xyz = { +0.5f, -0.5f, -0.5f }, .rgb = { 1.0f, 0.0f, 1.0f } },
		/*2*/ Vertex3D { .xyz = { +0.5f, +0.5f, -0.5f }, .rgb = { 1.0f, 1.0f, 1.0f } },
		/*3*/ Vertex3D { .xyz = { -0.5f, +0.5f, -0.5f }, .rgb = { 1.0f, 1.0f, 1.0f } },
		/*4*/ Vertex3D { .xyz = { -0.5f, -0.5f, +0.5f }, .rgb = { 1.0f, 0.0f, 1.0f } },
		/*5*/ Vertex3D { .xyz = { +0.5f, -0.5f, +0.5f }, .rgb = { 1.0f, 0.0f, 1.0f } },
		/*6*/ Vertex3D { .xyz = { +0.5f, +0.5f, +0.5f }, .rgb = { 1.0f, 1.0f, 1.0f } },
		/*7*/ Vertex3D { .xyz = { -0.5f, +0.5f, +0.5f }, .rgb = { 1.0f, 1.0f, 1.0f } }
	};
	
	static std::array<u16,36> constexpr kCubeIndices {
		0, 1, 2, 2, 3, 0, // Z-
		6, 5, 4, 4, 7, 6, // Z+
		5, 1, 0, 0, 4, 5, // Y-
		3, 2, 6, 6, 7, 3, // Y+
		0, 3, 7, 7, 4, 0, // X-
		6, 2, 1, 1, 5, 6, // X+
	};
	
} // end-of-namespace: gfx

#endif // end-of-header-guard: PRIMITIVES_HPP_7NQRHOKZ
// EOF
