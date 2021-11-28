#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 frag_rgb;

vec2 xy[6] = vec2[] (
	vec2(  .00, -.90 ),
	vec2(  .75,  .5 ),
	vec2( -.75,  .5 ),

	vec2(  .00,  .90 ),
	vec2( -.75, -.5 ),
	vec2(  .75, -.5 )
);

vec3 rgb[6] = vec3[] (
	vec3( 1.0,  .0,  .0 ),
	vec3(  .0, 1.0,  .0 ),
	vec3(  .0,  .0, 1.0 ),

	vec3( 1.0, 1.0,  .0 ),
	vec3( 1.0,  .0, 1.0 ),
	vec3(  .0, 1.0, 1.0 )
);

void main() {
	gl_Position = vec4( xy[gl_VertexIndex], .0, 1.0 );
	frag_rgb    = rgb[gl_VertexIndex];
}

