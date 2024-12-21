/*
 * Real Time Rendering 2024
 *
 * SEHEE CHO
 */
 
#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_tex_coords;

out vec2 tex_coords;

void main()
{
	tex_coords = a_tex_coords;
	gl_Position = vec4(a_position, 1.0f);
}