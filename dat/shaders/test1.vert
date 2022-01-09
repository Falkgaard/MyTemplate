#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in  vec2 inXY;   // input vertex position TODO: vec3 inXYZ
layout(location = 1) in  vec3 inRGB;  // input vertex colour   TODO: textures later
layout(location = 0) out vec3 outRGB; // output fragment colour

void main() {
	gl_Position = vec4( inXY, .0, 1.0 );
	outRGB      = inRGB;
}

// EOF
