/*
 * Real Time Rendering 2024
 *
 * SEHEE CHO
 */
 
#version 330 core

in vec2 tex_coords;

out vec4 final_color;

uniform vec4 u_global_ambient_color;
uniform sampler2D g_albedo_spec;

struct MATERIAL {
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	vec4 emissive_color;
	float specular_exponent;
};

#define NUMBER_OF_MATERIALS 3
uniform MATERIAL u_material[NUMBER_OF_MATERIALS];

uniform bool u_flag_texture_mapping = true;

void main()
{
	int material_id = int(texture(g_albedo_spec, tex_coords).a * 255.0f);

	vec4 base_color;

	vec4 diffuse_color = u_material[material_id].diffuse_color;

	if (u_flag_texture_mapping) 
		base_color = vec4(texture(g_albedo_spec, tex_coords).rgb, 1.0f);
	else 
		base_color = diffuse_color;

	vec4 emissive_color = u_material[material_id].emissive_color;

	final_color = emissive_color + u_global_ambient_color * base_color;
}