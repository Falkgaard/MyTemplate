#!/usr/bin/env bash
# run from project root!
printf '\n \033[0;1;37mCompiling shaders from GLSL to SPIR-V...\n'
shopt -s nullglob
shopt -s nocaseglob
for input_shader in ./dat/shaders/*.{vert,geom,frag}; do
	output_shader=$input_shader.spv
	printf '   \033[0;2;37mCompiling `\033[0;1;4;37m%s\033[0;2;37m`... ' "$input_shader"
	glslc "$input_shader" -o "$output_shader"
	printf 'done!\n'
done
shopt -u nullglob
shopt -u nocaseglob
# eof
