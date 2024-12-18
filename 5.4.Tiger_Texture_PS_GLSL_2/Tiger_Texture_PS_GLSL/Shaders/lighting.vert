/*
 * Real Time Rendering 2024
 *
 * SEHEE CHO
 */
 
#version 330 core

// input vertex attributes
//		: sphere's position, texture coordinates.
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;	// not used.
layout (location = 2) in vec2 a_tex_coords;

out vec2 tex_coords;

uniform mat4 u_ModelViewProjectionMatrix;

void main()
{
	tex_coords = a_tex_coords;
	gl_Position = u_ModelViewProjectionMatrix*vec4(a_position, 1.0f);
}