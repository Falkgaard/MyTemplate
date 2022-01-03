# TODO

## Add dependencies

gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
gh:malte-v/VulkanMemoryAllocator-Hpp
gh:bulletphysics/bullet3
glTF stuff
stb stuff
Spir-V shader compilation
Sol 3.2 + LuaJIT 2.1.x-beta3+
EnTT 3.9.0
Doxygen
VulkanMemoryAllocator
VulkanSceneGraph
simple\_vulkan\_synchronization
Fossilize

## Code-wise roadmap

the big TODO

Depth Buffer
Uniform Buffer
Pipeline Layout
Descriptor Set
Render Pass
Shaders
Framebuffers
Vertex buffer
Pipeline

...

vkBeginCommandBuffer()
vkCmd???()
vkEndCommandBuffer()
vkQueueSubmit()

## Misc

Extend the CMake build process (e.g. move the shader compilation there)
Add tests
Add log files

## Graphics-wise

VRS (Variable Rate Shading)

Occlusion Query limiting (queries + delay)

Depth optimization (bit-depth, bias, etc, near/far planes, etc)

Blitting if internal render resolution is less than the surface resolution

Use mipmapping

Drop frame buffers ASAP?

Use ASTC image compression

Geometry Instancing

Mouse-picking

PBR

Tessellation

Parallax mapping

SSAO

SSIL?

Volumetric clouds?

Godrays

### Optimization:

Refer to <https://developer.nvidia.com/blog/vulkan-dos-donts/>
