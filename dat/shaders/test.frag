#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 frag_rgb;

layout(location = 0) out vec4 out_rgba;

void main() {
	out_rgba = vec4( frag_rgb, 1.0 );
}

