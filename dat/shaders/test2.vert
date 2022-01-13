#version 450
#extension GL_ARB_separate_shader_objects : enable

// shader input:
layout(location = 0) in  vec3 inXYZ;  // input vertex position
layout(location = 1) in  vec3 inRGB;  // input vertex colour   TODO: textures later

// shader output:
layout(location = 0) out vec3 outRGB; // output fragment colour

// uniform buffer data:
layout(binding = 0) uniform UniformBufferObject {
	mat4 mvp;
};

// shader program:
void main() {
	gl_Position = mvp * vec4( inXYZ, 1.0 );
	outRGB      = inRGB;
}

// EOF
