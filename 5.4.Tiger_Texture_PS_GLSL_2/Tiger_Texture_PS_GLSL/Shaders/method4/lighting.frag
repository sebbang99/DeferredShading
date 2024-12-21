/*
 * Real Time Rendering 2024
 *
 * SEHEE CHO
 */
 
#version 330 core


uniform sampler2D g_pos;
uniform sampler2D g_norm;
uniform sampler2D g_albedo_spec;
uniform vec2 u_width_height;

struct LIGHT {
	vec4 position; 
	vec4 ambient_color, diffuse_color, specular_color;
	vec4 light_attenuation_factors; 
	vec3 spot_direction;
	float spot_exponent;
	float spot_cutoff_angle;
	bool light_on;
	
};

struct MATERIAL {
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	vec4 emissive_color;
	float specular_exponent;
};

uniform vec4 u_global_ambient_color;
#define NUMBER_OF_LIGHTS_SUPPORTED 100
uniform LIGHT u_light;

#define NUMBER_OF_MATERIALS 3 // default, tiger, floor
uniform MATERIAL u_material[NUMBER_OF_MATERIALS];

// for local illumination
//#define LIGHT_RANGE 100.0f

uniform bool u_flag_texture_mapping = true;

const float zero_f = 0.0f;
const float one_f = 1.0f;

out vec4 final_color;

vec4 lighting_equation_textured(in vec3 P_WC, in vec3 N_WC, in vec4 base_color, int m_id) {
	vec4 color_sum;
	float local_scale_factor, tmp_float; 
	vec3 L_WC;

	vec4 ambient_color = u_material[m_id].ambient_color;
    vec4 specular_color = u_material[m_id].specular_color;
    vec4 emissive_color = u_material[m_id].emissive_color;
    float specular_exponent = u_material[m_id].specular_exponent;

    color_sum = vec4(0.0f);//emissive_color + u_global_ambient_color * base_color;
	// Note by Sehee
	// The method 4 runs fragment shader by each sphere of light in the lighting pass,
	// and each fragment shader run has only fragments of corresponding sphere region.
	// If you turn on the comment above, the global illumination effect will be overlapped.
	// Therefore, for the global illumination effect, it is recommended that to use new last pass. 
 

	local_scale_factor = one_f;
	if (u_light.position.w != zero_f) { // point light source
		L_WC = u_light.position.xyz - P_WC.xyz;

		if (u_light.light_attenuation_factors.w != zero_f) {
			vec4 tmp_vec4;

			tmp_vec4.x = one_f;
			tmp_vec4.z = dot(L_WC, L_WC);
			tmp_vec4.y = sqrt(tmp_vec4.z);
			tmp_vec4.w = zero_f;

			local_scale_factor = one_f/dot(tmp_vec4, u_light.light_attenuation_factors);
		}

		// for local illumination.
//			if (sqrt(dot(L_WC, L_WC)) > LIGHT_RANGE) {	// should be upgraded for efficiency.
//				local_scale_factor = zero_f;
//			}

		L_WC = normalize(L_WC);

		if (u_light.spot_cutoff_angle < 180.0f) { // [0.0f, 90.0f] or 180.0f
			float spot_cutoff_angle = clamp(u_light.spot_cutoff_angle, zero_f, 90.0f);
			vec3 spot_dir = normalize(u_light.spot_direction);

			tmp_float = dot(-L_WC, spot_dir);
			if (tmp_float >= cos(radians(spot_cutoff_angle))) {
				tmp_float = pow(tmp_float, u_light.spot_exponent);
			}
			else 
				tmp_float = zero_f;
			local_scale_factor *= tmp_float;
		}
	}
	else {  // directional light source
		L_WC = normalize(u_light.position.xyz);
	}	

	if (local_scale_factor > zero_f) {		
		vec4 local_color_sum = u_light.ambient_color * ambient_color;

		tmp_float = dot(N_WC, L_WC);  
		if (tmp_float > zero_f) {  
			local_color_sum += u_light.diffuse_color*base_color*tmp_float;
			
			vec3 H_WC = normalize(L_WC - normalize(P_WC));
			tmp_float = dot(N_WC, H_WC); 
			if (tmp_float > zero_f) {
				local_color_sum += u_light.specular_color
				                    *specular_color*pow(tmp_float, specular_exponent);
			}
		}
		color_sum += local_scale_factor*local_color_sum;
	}

 	return color_sum;
}

void main()
{	
	vec2 tex_coords = gl_FragCoord.xy / u_width_height;

	vec3 frag_pos = texture(g_pos, tex_coords).rgb;
	vec3 normal = texture(g_norm, tex_coords).rgb;
	int material_id = int(texture(g_albedo_spec, tex_coords).a * 255.0f);

	vec4 base_color, shaded_color;

	vec4 diffuse_color = u_material[material_id].diffuse_color;

	if (u_flag_texture_mapping) 
		base_color = vec4(texture(g_albedo_spec, tex_coords).rgb, 1.0f);
	else 
		base_color = diffuse_color;
	// Note meanings of these variables below in the light equation.
	// base_color	: kd
	// spec			: ks

	shaded_color = lighting_equation_textured(frag_pos, normalize(normal), base_color, material_id);

	final_color = shaded_color;

	// just for debugging pass 1.
//	final_color = vec4(frag_pos, 1.0f);
//	final_color = vec4(normal, 1.0f);
//	final_color = base_color;
//
//	final_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}