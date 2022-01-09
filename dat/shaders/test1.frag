#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in  vec3 inRGB;   // input  fragment colour
layout(location = 0) out vec4 outRGBA; // output fragment colour

void main() {
	outRGBA = vec4( inRGB, 1.0 );
}

// EOF
