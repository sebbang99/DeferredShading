/*
 * Real Time Rendering 2024
 *
 * SEHEE CHO
 */

#version 330 core

// input vertex attributes
//		: floor, tiger(, axes)'s position, normal, texture coordinates.
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

// output vertex attributes
out vec3 frag_pos;
out vec3 normal;
out vec2 tex_coords;

// uniform variables
uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_ModelMatrix;
uniform mat3 u_ModelMatrixInvTrans;

void main() 
{
	frag_pos = vec3(u_ModelMatrix*vec4(a_position, 1.0f));
	normal = normalize(u_ModelMatrixInvTrans*a_normal); 
	tex_coords = a_tex_coords;

	gl_Position = u_ModelViewProjectionMatrix*vec4(a_position, 1.0f);	// CC, essential.
}