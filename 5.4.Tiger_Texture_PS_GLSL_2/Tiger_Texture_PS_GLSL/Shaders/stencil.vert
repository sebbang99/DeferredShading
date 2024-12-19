/*
 * Real Time Rendering 2024
 *
 * SEHEE CHO
 */

#version 330

layout (location = 0) in vec3 a_position;

uniform mat4 u_ModelViewProjectionMatrix;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(a_position, 1.0f);
}