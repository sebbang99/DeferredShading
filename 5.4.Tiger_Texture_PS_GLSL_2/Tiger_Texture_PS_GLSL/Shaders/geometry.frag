/*
 * Real Time Rendering 2024
 *
 * SEHEE CHO
 */
 
#version 330 core

layout (location = 0) out vec3 g_pos;
layout (location = 1) out vec3 g_norm;
layout (location = 2) out vec4 g_albedo_spec;

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coords;

uniform sampler2D u_base_texture;

void main()
{
	g_pos = frag_pos;	// WC
	g_norm = normal;	// WC
	g_albedo_spec.rgb = texture(u_base_texture, tex_coords).rgb;
	g_albedo_spec.a = 1.0f;	// TODO
}