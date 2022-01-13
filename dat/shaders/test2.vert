#version 450
#extension GL_ARB_separate_shader_objects : enable

// shader input:
layout(location = 0) in  vec2 inXY;   // input vertex position TODO: vec3 inXYZ
layout(location = 1) in  vec3 inRGB;  // input vertex colour   TODO: textures later

// shader output:
layout(location = 0) out vec3 outRGB; // output fragment colour

// uniform buffer data:
layout(binding = 0) uniform UniformBufferObject {
	mat4 mvp;
};

// shader program:
void main() {
	gl_Position = mvp * vec4( inXY, .0, 1.0 );
	outRGB      = inRGB;
}

// EOF
