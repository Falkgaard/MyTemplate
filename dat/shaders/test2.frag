#version 450
#extension GL_ARB_separate_shader_objects : enable

// shader input:
layout(location = 0) in  vec3 inRGB;   // input  fragment colour

// shader output:
layout(location = 0) out vec4 outRGBA; // output fragment colour

// shader program:
void main() {
	outRGBA = vec4( inRGB, 1.0 );
}

// EOF
