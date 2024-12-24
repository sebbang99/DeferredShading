#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <FreeImage/FreeImage.h>

// for calculating FPS
#include <chrono>
#include <iostream>
//std::chrono::steady_clock::time_point base_time, cur_time;
#include "./wglext.h"
int base_time = 0;
int frame_cnt = 0;

#include "Shaders/LoadShaders.h"
#include "My_Shading_Method3.h"
// handles to shader programs
GLuint h_ShaderProgram_geometry, h_ShaderProgram_lighting;

#define NUMBER_OF_LIGHT_SUPPORTED 100

#define NUMBER_OF_MATERIALS 3 // default, tiger, floor
#define MATERIAL_ID_TIGER 1
#define MATERIAL_ID_FLOOR 2

// location of uniform variables for geometry pass shaders
GLint loc_ModelViewProjectionMatrix_geometry;
GLint loc_ModelMatrix_geometry, loc_ModelMatrixInvTrans_geometry;
GLint loc_texture_geometry;
GLint loc_material_id;

// location of uniform variables for lighting pass shaders
GLint loc_g_pos, loc_g_norm, loc_g_albedo_spec;
GLint loc_global_ambient_color_lighting;
loc_light_Parameters loc_light_lighting[NUMBER_OF_LIGHT_SUPPORTED];
GLint loc_flag_texture_mapping_lighting;
loc_Material_Parameters loc_material_lighting[NUMBER_OF_LIGHT_SUPPORTED];

unsigned int g_buffer;
unsigned int g_pos, g_norm, g_albedo_spec;
unsigned int quad_VAO = 0;
unsigned int quad_VBO;

// include glm/*.hpp only if necessary
//#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
#include <glm/gtc/matrix_inverse.hpp> // inverseTranspose, etc.
glm::mat4 ModelViewProjectionMatrix, ModelViewMatrix;
glm::mat3 ModelViewMatrixInvTrans;
glm::mat4 ModelMatrix;
glm::mat3 ModelMatrixInvTrans;
glm::mat4 ViewMatrix, ProjectionMatrix;

// settings
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 800;

// for camera moving
glm::vec3 initialCameraPosition;
glm::vec3 cameraPosition;
int PRP_distance_level = 0;
glm::vec3 PRP, VRP, VUV;
glm::vec3 u, v, n;
#define MOVE_SPEED 2.0f

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_VERTEX 0
#define LOC_NORMAL 1
#define LOC_TEXCOORD 2

// lights in scene
Light_Parameters light[NUMBER_OF_LIGHT_SUPPORTED];

// texture stuffs
#define N_TEXTURES_USED 8
#define TEXTURE_ID_FLOOR 3
#define TEXTURE_ID_TIGER 4
#define TEXTURE_ID_GRASS 5
#define TEXTURE_ID_BIRDS 6
#define TEXTURE_ID_APPLES 7

#define TEXTURE_ID_G_POS 0
#define TEXTURE_ID_G_NORM 1
#define TEXTURE_ID_G_ALBEDO_SPEC 2

GLuint texture_names[N_TEXTURES_USED];
int flag_texture_mapping;

void My_glTexImage2D_from_file(char *filename) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP *tx_pixmap, *tx_pixmap_32;

	int width, height;
	GLvoid *data;
	
	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
	if (tx_bits_per_pixel == 32)
		tx_pixmap_32 = tx_pixmap;
	else {
		fprintf(stdout, " * Converting texture from %d bits to 32 bits...\n", tx_bits_per_pixel);
		tx_pixmap_32 = FreeImage_ConvertTo32Bits(tx_pixmap);
	}

	width = FreeImage_GetWidth(tx_pixmap_32);
	height = FreeImage_GetHeight(tx_pixmap_32);
	data = FreeImage_GetBits(tx_pixmap_32);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap_32);
	if (tx_bits_per_pixel != 32)
		FreeImage_Unload(tx_pixmap);
}

// fog stuffs
// you could control the fog parameters interactively: FOG_COLOR, FOG_NEAR_DISTANCE, FOG_FAR_DISTANCE   
int flag_fog;

// for tiger animation
unsigned int timestamp_scene = 0; // the global clock in the scene
int flag_tiger_animation, flag_polygon_fill;
int cur_frame_tiger = 0;
float rotation_angle_tiger = 0.0f;

 // floor object
#define TEX_COORD_EXTENT 1.0f
 GLuint rectangle_VBO, rectangle_VAO;
 GLfloat rectangle_vertices[6][8] = {  // vertices enumerated counterclockwise
	 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	 { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, 0.0f },
	 { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, TEX_COORD_EXTENT },
	 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	 { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, TEX_COORD_EXTENT },
	 { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, TEX_COORD_EXTENT }
 };

 Material_Parameters material_floor;

 void prepare_floor(void) { // Draw coordinate axes.
	 // Initialize vertex buffer object.
	 glGenBuffers(1, &rectangle_VBO);

	 glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), &rectangle_vertices[0][0], GL_STATIC_DRAW);

	 // Initialize vertex array object.
	 glGenVertexArrays(1, &rectangle_VAO);
	 glBindVertexArray(rectangle_VAO);

	 glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	 glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(0));
	 glEnableVertexAttribArray(0);
	 glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
 	 glEnableVertexAttribArray(1);
	 glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(6 * sizeof(float)));
	 glEnableVertexAttribArray(2);

	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 glBindVertexArray(0);

	 material_floor.ambient_color[0] = 0.0f;
	 material_floor.ambient_color[1] = 0.05f;
	 material_floor.ambient_color[2] = 0.0f;
	 material_floor.ambient_color[3] = 1.0f;

	 material_floor.diffuse_color[0] = 0.2f;
	 material_floor.diffuse_color[1] = 0.5f;
	 material_floor.diffuse_color[2] = 0.2f;
	 material_floor.diffuse_color[3] = 1.0f;

	 material_floor.specular_color[0] = 0.24f;
	 material_floor.specular_color[1] = 0.5f;
	 material_floor.specular_color[2] = 0.24f;
	 material_floor.specular_color[3] = 1.0f;

	 material_floor.specular_exponent = 2.5f;

	 material_floor.emissive_color[0] = 0.0f;
	 material_floor.emissive_color[1] = 0.0f;
	 material_floor.emissive_color[2] = 0.0f;
	 material_floor.emissive_color[3] = 1.0f;

 	 glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	 glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_FLOOR);
	 glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_FLOOR]);

 //	 My_glTexImage2D_from_file("Data/grass_tex.jpg");
	 My_glTexImage2D_from_file("Data/checker_tex.jpg");

	 glGenerateMipmap(GL_TEXTURE_2D);

 	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

 	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

// 	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
// 	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

// 	 float border_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
// 	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
// 	 glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
 }

 void set_material_floor(void) {
	 glUniform4fv(loc_material_lighting[MATERIAL_ID_FLOOR].ambient_color, 1, material_floor.ambient_color);
	 glUniform4fv(loc_material_lighting[MATERIAL_ID_FLOOR].diffuse_color, 1, material_floor.diffuse_color);
	 glUniform4fv(loc_material_lighting[MATERIAL_ID_FLOOR].specular_color, 1, material_floor.specular_color);
	 glUniform1f(loc_material_lighting[MATERIAL_ID_FLOOR].specular_exponent, material_floor.specular_exponent);
	 glUniform4fv(loc_material_lighting[MATERIAL_ID_FLOOR].emissive_color, 1, material_floor.emissive_color);
 }

 void draw_floor(void) {
	 glFrontFace(GL_CCW);

	 glBindVertexArray(rectangle_VAO);
	 glDrawArrays(GL_TRIANGLES, 0, 6);
	 glBindVertexArray(0);
 }

 // tiger object
#define N_TIGER_FRAMES 12
GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat *tiger_vertices[N_TIGER_FRAMES];

Material_Parameters material_tiger;

int read_geometry(GLfloat **object, int bytes_per_primitive, char *filename) {
	int n_triangles;
	FILE *fp;

	// fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	fp = fopen(filename, "rb");
	if (fp == NULL){
		fprintf(stderr, "Cannot open the object file %s ...", filename);
		return -1;
	}
	fread(&n_triangles, sizeof(int), 1, fp);
	*object = (float *)malloc(n_triangles*bytes_per_primitive);
	if (*object == NULL){
		fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
		return -1;
	}

	fread(*object, bytes_per_primitive, n_triangles, fp);
	// fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);

	return n_triangles;
}

void prepare_tiger(void) { // vertices enumerated clockwise
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_TIGER_FRAMES; i++) {
		sprintf(filename, "Data/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
		tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		tiger_n_total_triangles += tiger_n_triangles[i];

		if (i == 0)
			tiger_vertex_offset[i] = 0;
		else
			tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &tiger_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_TIGER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
		tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_TIGER_FRAMES; i++)
		free(tiger_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &tiger_VAO);
	glBindVertexArray(tiger_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_tiger.ambient_color[0] = 0.24725f;
	material_tiger.ambient_color[1] = 0.1995f;
	material_tiger.ambient_color[2] = 0.0745f;
	material_tiger.ambient_color[3] = 1.0f;

	material_tiger.diffuse_color[0] = 0.75164f;
	material_tiger.diffuse_color[1] = 0.60648f;
	material_tiger.diffuse_color[2] = 0.22648f;
	material_tiger.diffuse_color[3] = 1.0f;

	material_tiger.specular_color[0] = 0.728281f;
	material_tiger.specular_color[1] = 0.655802f;
	material_tiger.specular_color[2] = 0.466065f;
	material_tiger.specular_color[3] = 1.0f;

	material_tiger.specular_exponent = 51.2f;

	material_tiger.emissive_color[0] = 0.1f;
	material_tiger.emissive_color[1] = 0.1f;
	material_tiger.emissive_color[2] = 0.0f;
	material_tiger.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_GRASS);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_GRASS]);

	My_glTexImage2D_from_file("Data/grass_tex.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_BIRDS);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_BIRDS]);

	My_glTexImage2D_from_file("Data/Image_4_6304_4192.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_APPLES);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_APPLES]);

	My_glTexImage2D_from_file("Data/apples.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void set_material_tiger(void) {
	glUniform4fv(loc_material_lighting[MATERIAL_ID_TIGER].ambient_color, 1, material_tiger.ambient_color);
	glUniform4fv(loc_material_lighting[MATERIAL_ID_TIGER].diffuse_color, 1, material_tiger.diffuse_color);
	glUniform4fv(loc_material_lighting[MATERIAL_ID_TIGER].specular_color, 1, material_tiger.specular_color);
	glUniform1f(loc_material_lighting[MATERIAL_ID_TIGER].specular_exponent, material_tiger.specular_exponent);
	glUniform4fv(loc_material_lighting[MATERIAL_ID_TIGER].emissive_color, 1, material_tiger.emissive_color);
}

void draw_tiger(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(tiger_VAO);
	glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger], 3 * tiger_n_triangles[cur_frame_tiger]);
	glBindVertexArray(0);
}

// callbacks
float PRP_distance_scale[6] = { 0.5f, 1.0f, 2.5f, 5.0f, 10.0f, 20.0f };

//void CalculateFPS() {
//	frame_cnt++;
//
//	if (frame_cnt >= 1000) {
//		glFinish();
//
//		cur_time = std::chrono::high_resolution_clock::now();
//		std::chrono::duration<double, std::milli> inter_time = cur_time - base_time;
//		printf("*** %lf (ms) for 1 frame, %lf (fps)\n", inter_time.count() / 1000.0, 1000000.0 / inter_time.count());
//
//		frame_cnt = 0;
//		base_time = cur_time;
//	}
//}

void setVSync(int interval) {
	// wglSwapIntervalEXT¸¦ °¡Á®¿É´Ï´Ù.
	typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

	if (wglSwapIntervalEXT) {
		wglSwapIntervalEXT(interval); // interval °ª: 0 (VSync ²û), 1 (ÄÔ)
	}
}

void display(void) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 1. Geometry pass BEGIN
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(h_ShaderProgram_geometry);

		glUniform1f(loc_material_id, float(MATERIAL_ID_FLOOR));
		glUniform1i(loc_texture_geometry, TEXTURE_ID_FLOOR);
		ModelMatrix = glm::mat4(1.0f);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-500.0f, 0.0f, 500.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1000.0f, 1000.0f, 1000.0f));
		ModelMatrix = glm::rotate(ModelMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
		ModelViewMatrix = ViewMatrix * ModelMatrix;
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

		glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
		draw_floor();
	
		glUniform1f(loc_material_id, float(MATERIAL_ID_TIGER));
		{
			glUniform1i(loc_texture_geometry, TEXTURE_ID_TIGER);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(200.0f, 0.0f, 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 1

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-200.0f, 0.0f, 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 2

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f, 0.0f, 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 3

			glUniform1i(loc_texture_geometry, TEXTURE_ID_GRASS);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-100.0f, 0.0f, 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 4

			glUniform1i(loc_texture_geometry, TEXTURE_ID_GRASS);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 200.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 5

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, -200.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 6

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 100.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 7

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, -100.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 8

			glUniform1i(loc_texture_geometry, TEXTURE_ID_BIRDS);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(200.0f, 0.0f, 200.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 9

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-200.0f, 0.0f, -200.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 10

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f, 0.0f, 100.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 11

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-100.0f, 0.0f, -100.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 12

			glUniform1i(loc_texture_geometry, TEXTURE_ID_APPLES);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-150.0f, 50.0f, -150.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 13

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(150.0f, 50.0f, -150.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 14

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(150.0f, 50.0f, -150.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 15

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-150.0f, 50.0f, -150.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 16
		}

		//////////// +16 //////////////////////////////////////////////////////////////////////////
		{		
			glUniform1i(loc_texture_geometry, TEXTURE_ID_TIGER);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(200.0f, 100.0f, 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 1

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-200.0f, 100.0f, 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 2

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f, 100.0f, 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 3

			glUniform1i(loc_texture_geometry, TEXTURE_ID_GRASS);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-100.0f, 100.0f, 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 4

			glUniform1i(loc_texture_geometry, TEXTURE_ID_GRASS);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 100.0f, 200.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
			ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 5

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 100.0f, -200.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 6

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 100.0f, 100.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 7

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 100.0f, -100.0f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 8

			glUniform1i(loc_texture_geometry, TEXTURE_ID_BIRDS);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(200.0f, 100.0f, 200.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 9

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-200.0f, 100.0f, -200.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 10

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f, 100.0f, 100.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 11

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-100.0f, 100.0f, -100.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 12

			glUniform1i(loc_texture_geometry, TEXTURE_ID_APPLES);
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-150.0f, 150.0f, -150.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 13

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(150.0f, 150.0f, -150.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 14

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(150.0f, 150.0f, -150.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

			glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelMatrixInvTrans[0][0]);
			draw_tiger(); // 15

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-150.0f, 150.0f, -150.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
			ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelMatrix));
			ModelViewMatrix = ViewMatrix * ModelMatrix;
			ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
 
			draw_tiger(); // 16

		}

		//////////////////////////////////////////////////////////////////////////////////////////////

		ModelMatrix = glm::mat4(1.0f);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(500.0f, 0.0f, 500.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
		ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		ModelViewMatrix = ViewMatrix * ModelMatrix;
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;

		glUniformMatrix4fv(loc_ModelViewProjectionMatrix_geometry, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glUniformMatrix4fv(loc_ModelMatrix_geometry, 1, GL_FALSE, &ModelViewMatrix[0][0]);
		glUniformMatrix3fv(loc_ModelMatrixInvTrans_geometry, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
		draw_tiger(); 
		// flag tiger

	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Geometry pass END

	// 2. Lighting pass BEGIN
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(h_ShaderProgram_lighting);

		glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_G_POS);
		glBindTexture(GL_TEXTURE_2D, g_pos);
		glUniform1i(loc_g_pos, TEXTURE_ID_G_POS);
		glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_G_NORM);
		glBindTexture(GL_TEXTURE_2D, g_norm);
		glUniform1i(loc_g_norm, TEXTURE_ID_G_NORM);
		glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_G_ALBEDO_SPEC);
		glBindTexture(GL_TEXTURE_2D, g_albedo_spec);
		glUniform1i(loc_g_albedo_spec, TEXTURE_ID_G_ALBEDO_SPEC);

		set_material_floor();
		set_material_tiger();

		if (quad_VAO == 0)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &quad_VAO);
			glGenBuffers(1, &quad_VBO);
			glBindVertexArray(quad_VAO);
			glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quad_VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

	glUseProgram(0);
	// Lighting pass END

	//CalculateFPS();
	frame_cnt++;

	glutSwapBuffers();
}

void timer_scene(int value) {
	timestamp_scene = (timestamp_scene + 1) % UINT_MAX;
	cur_frame_tiger = timestamp_scene % N_TIGER_FRAMES;
	rotation_angle_tiger = (timestamp_scene % 360)*TO_RADIAN;
	//glutPostRedisplay();
	if (flag_tiger_animation)
		glutTimerFunc(100, timer_scene, 0);
}

void keyboard(unsigned char key, int x, int y) {
	static int flag_cull_face = 0;
	//static int PRP_distance_level = 0;
	static int flag_floor_mag_filter = 0, flag_floor_min_filter = 0;
	static int flag_tiger_mag_filter = 0, flag_tiger_min_filter = 0;

	glm::vec4 position_EC;
	glm::vec3 direction_EC;

	//if ((key >= '0') && (key <= '0' + NUMBER_OF_LIGHT_SUPPORTED - 1)) {
	//	int light_ID = (int) (key - '0');

	//	glUseProgram(h_ShaderProgram_lighting);
	//	light[light_ID].light_on = 1 - light[light_ID].light_on;
	//	glUniform1i(loc_light_lighting[light_ID].light_on, light[light_ID].light_on);
	//	glUseProgram(0);

	//	glutPostRedisplay();
	//	return;
	//}

	switch (key) {
	case 'z':
		glUseProgram(h_ShaderProgram_lighting);
		for (uint32_t i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
			light[i].light_on = 1 - light[i].light_on;
			glUniform1i(loc_light_lighting[i].light_on, light[i].light_on);
		}
		glUseProgram(0);

		glutPostRedisplay();
		break;
	case 'a': // toggle the animation effect.
		flag_tiger_animation = 1 - flag_tiger_animation;
		if (flag_tiger_animation) {
			glutTimerFunc(100, timer_scene, 0);
			fprintf(stdout, "^^^ Animation mode ON.\n");
		}
		else
			fprintf(stdout, "^^^ Animation mode OFF.\n");
		break;
	case 'f': 
		//flag_fog = 1 - flag_fog;
		//if (flag_fog)
		//	fprintf(stdout, "^^^ Fog mode ON.\n");
		//else
		//	fprintf(stdout, "^^^ Fog mode OFF.\n");
		//glUseProgram(h_ShaderProgram_lighting);
		//glUniform1i(loc_flag_fog, flag_fog);
		//glUseProgram(0);
		//glutPostRedisplay();
		break;
	case 't':
		flag_texture_mapping = 1 - flag_texture_mapping;
		if (flag_texture_mapping)
			fprintf(stdout, "^^^ Texture mapping ON.\n");
		else 
			fprintf(stdout, "^^^ Texture mapping OFF.\n");
		glUseProgram(h_ShaderProgram_lighting);
		glUniform1i(loc_flag_texture_mapping_lighting, flag_texture_mapping);
		glUseProgram(0);
		glutPostRedisplay();
		break;
	//case 'y': // Change the floor texture's magnification filter.
	//	flag_floor_mag_filter = (flag_floor_mag_filter + 1) % 2;
	//	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_FLOOR);
	//	if (flag_floor_mag_filter == 0) {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	//		fprintf(stdout, "^^^ Mag filter for floor: GL_NEAREST.\n");
	//	}
	//	else {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//		fprintf(stdout, "^^^ Mag filter for floor: GL_LINEAR.\n");
	//	}
	//	glutPostRedisplay();
	//	break;
	//case 'u': // Change the floor texture's minification filter.
	//	flag_floor_min_filter = (flag_floor_min_filter + 1) % 3;
	//	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_FLOOR);
	//	if (flag_floor_min_filter == 0) {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//		fprintf(stdout, "^^^ Min filter for floor: GL_NEAREST.\n");
	//	}
	//	else if (flag_floor_min_filter == 1) {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//		fprintf(stdout, "^^^ Min filter for floor: GL_LINEAR.\n");
	//	}
	//	else {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//		fprintf(stdout, "^^^ Min filter for floor: GL_LINEAR_MIPMAP_LINEAR.\n");
	//	}
	//	glutPostRedisplay();
	//	break;
	//case 'i': // Change the tiger texture's magnification filter.
	//	flag_tiger_mag_filter = (flag_tiger_mag_filter + 1) % 2;
	//	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	//	if (flag_tiger_mag_filter == 0) {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//		fprintf(stdout, "^^^ Mag filter for tiger: GL_NEAREST.\n");
	//	}
	//	else {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//		fprintf(stdout, "^^^ Mag filter for tiger: GL_LINEAR.\n");
	//	}
	//	glutPostRedisplay();
	//	break;
	//case 'o': // Change the tiger texture's minification filter.
	//	flag_tiger_min_filter = (flag_tiger_min_filter + 1) % 3;
	//	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	//	if (flag_tiger_min_filter == 0) {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//		fprintf(stdout, "^^^ Min filter for tiger: GL_NEAREST.\n");
	//	}
	//	else if (flag_tiger_min_filter == 1) {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//		fprintf(stdout, "^^^ Min filter for tiger: GL_LINEAR.\n");
	//	}
	//	else {
	//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//		fprintf(stdout, "^^^ Min filter for tiger: GL_LINEAR_MIPMAP_LINEAR.\n");
	//	}
	//	glutPostRedisplay();
	//	break;
	case 'c':
		flag_cull_face = (flag_cull_face + 1) % 3;
		switch (flag_cull_face) {
		case 0:
			fprintf(stdout, "^^^ Face culling OFF.\n");
			glDisable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		case 1: // cull back faces;
			fprintf(stdout, "^^^ BACK face culled.\n");
			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		case 2: // cull front faces;
			fprintf(stdout, "^^^ FRONT face culled.\n");
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		}
		break;
	case 'd':
	{
		PRP_distance_level = (PRP_distance_level + 1) % 6;
		fprintf(stdout, "^^^ Distance level = %d.\n", PRP_distance_level);

		//ViewMatrix = glm::lookAt(PRP_distance_scale[PRP_distance_level] * glm::vec3(700.0f, 400.0f, 700.0f),
		//	glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		PRP = PRP_distance_scale[PRP_distance_level] * initialCameraPosition;
		ViewMatrix = glm::lookAt(PRP, VRP, VUV);

		// Update u, v, n of camera coordinates.
		glm::vec3 VPN = PRP - VRP;
		u = glm::normalize(glm::cross(VUV, VPN));
		v = glm::normalize(glm::cross(VPN, u));
		n = glm::normalize(VPN);

		/*glUseProgram(h_ShaderProgram_TXPS);
		// Must update the light 1's geometry in EC.
		position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
			light[1].position[2], light[1].position[3]);
		glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
		direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
			light[1].spot_direction[1], light[1].spot_direction[2]);
		glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
		glUseProgram(0);*/
		glutPostRedisplay();
		break;
	}
	case 'p':
		flag_polygon_fill = 1 - flag_polygon_fill;
		if (flag_polygon_fill) 
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glutPostRedisplay();
		break;
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups
		break;
	}
}

void special(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_LEFT:
	{
		// Move left
		PRP -= u * MOVE_SPEED;
		VRP -= u * MOVE_SPEED;
		ViewMatrix = glm::lookAt(PRP, VRP, VUV);

		// Update u, v, n of camera coordinates.
		glm::vec3 VPN = PRP - VRP;
		u = glm::normalize(glm::cross(VUV, VPN));
		v = glm::normalize(glm::cross(VPN, u));
		n = glm::normalize(VPN);

		glutPostRedisplay();
		break;
	}
	case GLUT_KEY_RIGHT:
	{
		// Move right
		PRP += u * MOVE_SPEED;
		VRP += u * MOVE_SPEED;
		ViewMatrix = glm::lookAt(PRP, VRP, VUV);

		// Update u, v, n of camera coordinates.
		glm::vec3 VPN = PRP - VRP;
		u = glm::normalize(glm::cross(VUV, VPN));
		v = glm::normalize(glm::cross(VPN, u));
		n = glm::normalize(VPN);

		glutPostRedisplay();
		break;
	}
	case GLUT_KEY_UP:
	{
		// Move forward
		PRP -= n * MOVE_SPEED;
		VRP -= n * MOVE_SPEED;
		ViewMatrix = glm::lookAt(PRP, VRP, VUV);

		// Update u, v, n of camera coordinates.
		glm::vec3 VPN = PRP - VRP;
		u = glm::normalize(glm::cross(VUV, VPN));
		v = glm::normalize(glm::cross(VPN, u));
		n = glm::normalize(VPN);

		glutPostRedisplay();
		break;
	}
	case GLUT_KEY_DOWN:
	{
		// Move backward
		PRP += n * MOVE_SPEED;
		VRP += n * MOVE_SPEED;
		ViewMatrix = glm::lookAt(PRP, VRP, VUV);

		// Update u, v, n of camera coordinates.
		glm::vec3 VPN = PRP - VRP;
		u = glm::normalize(glm::cross(VUV, VPN));
		v = glm::normalize(glm::cross(VPN, u));
		n = glm::normalize(VPN);

		glutPostRedisplay();
		break;
	}
	case GLUT_KEY_PAGE_UP:
	{
		// Move up
		PRP += v * MOVE_SPEED;
		VRP += v * MOVE_SPEED;
		ViewMatrix = glm::lookAt(PRP, VRP, VUV);

		// Update u, v, n of camera coordinates.
		glm::vec3 VPN = PRP - VRP;
		u = glm::normalize(glm::cross(VUV, VPN));
		v = glm::normalize(glm::cross(VPN, u));
		n = glm::normalize(VPN);

		glutPostRedisplay();
		break;
	}
	case GLUT_KEY_PAGE_DOWN:
	{
		// Move down
		PRP -= v * MOVE_SPEED;
		VRP -= v * MOVE_SPEED;
		ViewMatrix = glm::lookAt(PRP, VRP, VUV);

		// Update u, v, n of camera coordinates.
		glm::vec3 VPN = PRP - VRP;
		u = glm::normalize(glm::cross(VUV, VPN));
		v = glm::normalize(glm::cross(VPN, u));
		n = glm::normalize(VPN);

		glutPostRedisplay();
		break;
	}
	}
}

void reshape(int width, int height) {
	float aspect_ratio;

	glViewport(0, 0, width, height);
	
	aspect_ratio = (float) width / height;
	ProjectionMatrix = glm::perspective(45.0f*TO_RADIAN, aspect_ratio, 100.0f, 20000.0f);

	glutPostRedisplay();
}

void cleanup(void) {

	glDeleteVertexArrays(1, &rectangle_VAO);
	glDeleteBuffers(1, &rectangle_VBO);

	glDeleteVertexArrays(1, &tiger_VAO);
	glDeleteBuffers(1, &tiger_VBO);
	
	glDeleteVertexArrays(1, &quad_VAO);
	glDeleteBuffers(1, &quad_VBO);

	glDeleteTextures(N_TEXTURES_USED, texture_names);
}

void idle() {
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	int timeInterval = currentTime - base_time;

	if (timeInterval > 1000) {
		float fps = frame_cnt * 1000.0f / timeInterval;
		base_time = currentTime;
		frame_cnt = 0;

		printf("FPS: %.2f\n", fps);
	}

	glutPostRedisplay();
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutCloseFunc(cleanup);
	glutIdleFunc(idle);
}

void prepare_shader_program(void) {
	int i;
	char string[256];
	ShaderInfo shader_info_geometry[3] = {
		{ GL_VERTEX_SHADER, "Shaders/method3/geometry.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/method3/geometry.frag" },
		{ GL_NONE, NULL }	// need this?
	};
	ShaderInfo shader_info_lighting[3] = {
		{ GL_VERTEX_SHADER, "Shaders/method3/lighting.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/method3/lighting.frag" },
		{ GL_NONE, NULL }	// need this?
	};

	// geometry
	{
		h_ShaderProgram_geometry = LoadShaders(shader_info_geometry);
		loc_ModelMatrix_geometry = glGetUniformLocation(h_ShaderProgram_geometry, "u_ModelMatrix");
		loc_ModelMatrixInvTrans_geometry = glGetUniformLocation(h_ShaderProgram_geometry, "u_ModelMatrixInvTrans");
		loc_ModelViewProjectionMatrix_geometry = glGetUniformLocation(h_ShaderProgram_geometry, "u_ModelViewProjectionMatrix");
		loc_texture_geometry = glGetUniformLocation(h_ShaderProgram_geometry, "u_base_texture");
		loc_material_id = glGetUniformLocation(h_ShaderProgram_geometry, "u_material_id");
	}

	// lighting
	{
		h_ShaderProgram_lighting = LoadShaders(shader_info_lighting);
		loc_g_pos = glGetUniformLocation(h_ShaderProgram_lighting, "g_pos");
		loc_g_norm = glGetUniformLocation(h_ShaderProgram_lighting, "g_norm");
		loc_g_albedo_spec = glGetUniformLocation(h_ShaderProgram_lighting, "g_albedo_spec");
		loc_global_ambient_color_lighting = glGetUniformLocation(h_ShaderProgram_lighting, "u_global_ambient_color");

		for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
			sprintf(string, "u_light[%d].light_on", i);
			loc_light_lighting[i].light_on = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].position", i);
			loc_light_lighting[i].position = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].ambient_color", i);
			loc_light_lighting[i].ambient_color = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].diffuse_color", i);
			loc_light_lighting[i].diffuse_color = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].specular_color", i);
			loc_light_lighting[i].specular_color = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].spot_direction", i);
			loc_light_lighting[i].spot_direction = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].spot_exponent", i);
			loc_light_lighting[i].spot_exponent = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].spot_cutoff_angle", i);
			loc_light_lighting[i].spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].light_attenuation_factors", i);
			loc_light_lighting[i].light_attenuation_factors = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_light[%d].radius", i);
			loc_light_lighting[i].radius = glGetUniformLocation(h_ShaderProgram_lighting, string);
		}

		for (i = 0; i < NUMBER_OF_MATERIALS; i++) {
			sprintf(string, "u_material[%d].ambient_color", i);
			loc_material_lighting[i].ambient_color = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_material[%d].diffuse_color", i);
			loc_material_lighting[i].diffuse_color = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_material[%d].specular_color", i);
			loc_material_lighting[i].specular_color = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_material[%d].emissive_color", i);
			loc_material_lighting[i].emissive_color = glGetUniformLocation(h_ShaderProgram_lighting, string);
			sprintf(string, "u_material[%d].specular_exponent", i);
			loc_material_lighting[i].specular_exponent = glGetUniformLocation(h_ShaderProgram_lighting, string);
		}

		loc_flag_texture_mapping_lighting = glGetUniformLocation(h_ShaderProgram_lighting, "u_flag_texture_mapping");
	}
}

void initialize_lights_and_material(void) { // follow OpenGL conventions for initialization
	int i;

	glUseProgram(h_ShaderProgram_lighting);

	glUniform4f(loc_global_ambient_color_lighting, 0.115f, 0.115f, 0.115f, 1.0f);
	//glUniform4f(loc_global_ambient_color_lighting, 1.0f, 1.0f, 1.0f, 1.0f);	// just for debugging
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		glUniform1i(loc_light_lighting[i].light_on, 0); // turn off all lights initially
		glUniform4f(loc_light_lighting[i].position, 0.0f, 0.0f, 1.0f, 0.0f);
		glUniform4f(loc_light_lighting[i].ambient_color, 0.0f, 0.0f, 0.0f, 1.0f);
		if (i == 0) {
			glUniform4f(loc_light_lighting[i].diffuse_color, 1.0f, 1.0f, 1.0f, 1.0f);
			glUniform4f(loc_light_lighting[i].specular_color, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		else {
			glUniform4f(loc_light_lighting[i].diffuse_color, 0.0f, 0.0f, 0.0f, 1.0f);
			glUniform4f(loc_light_lighting[i].specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
		}
		glUniform3f(loc_light_lighting[i].spot_direction, 0.0f, 0.0f, -1.0f);
		glUniform1f(loc_light_lighting[i].spot_exponent, 0.0f); // [0.0, 128.0]
		glUniform1f(loc_light_lighting[i].spot_cutoff_angle, 180.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
		glUniform4f(loc_light_lighting[i].light_attenuation_factors, 1.0f, 0.014f, 0.0007f, 1.0f); // .w == 0.0f for no light attenuation
	}

	for (i = 0; i < NUMBER_OF_MATERIALS; i++) {
		glUniform4f(loc_material_lighting[i].ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);
		glUniform4f(loc_material_lighting[i].diffuse_color, 0.8f, 0.8f, 0.8f, 1.0f);
		glUniform4f(loc_material_lighting[i].specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
		glUniform4f(loc_material_lighting[i].emissive_color, 0.0f, 0.0f, 0.0f, 1.0f);
		glUniform1f(loc_material_lighting[i].specular_exponent, 0.0f); // [0.0, 128.0]
	}

	glUseProgram(0);
}

void initialize_flags(void) {
	flag_tiger_animation = 1;
	flag_polygon_fill = 1;
	flag_texture_mapping = 1;
	//flag_fog = 0;

	glUseProgram(h_ShaderProgram_lighting);
	glUniform1i(loc_flag_texture_mapping_lighting, flag_texture_mapping);
	glUseProgram(0);
}

void initialize_OpenGL(void) {
  	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	//ViewMatrix = glm::lookAt(PRP_distance_scale[0] * glm::vec3(500.0f, 400.0f, 500.0f),
	//	glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	initialCameraPosition = glm::vec3(500.0f, 400.0f, 500.0f);
	cameraPosition = initialCameraPosition;
	PRP = PRP_distance_scale[2] * initialCameraPosition;
	VRP = glm::vec3(0.0f, 0.0f, 0.0f);
	VUV = glm::vec3(0.0f, 1.0f, 0.0f);
	ViewMatrix = glm::lookAt(PRP, VRP, VUV);
	
	// Prepare u, v, n of camera coordinates.
	glm::vec3 VPN = PRP - VRP;	// "View Plane Normal"
	u = glm::normalize(glm::cross(VUV, VPN));
	v = glm::normalize(glm::cross(VPN, u));
	n = glm::normalize(VPN);

	initialize_lights_and_material();
	initialize_flags();

	glGenTextures(N_TEXTURES_USED, texture_names);
}

void set_up_scene_lights(void) {

	// Change this value if you want refresh colors of lights! :)
	srand(125);

	for (uint32_t i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		light[i].light_on = 1;
			
		// light position in WC
		light[i].position[0] = static_cast<float>(rand() % 1000) - 500.0f;
		light[i].position[1] = static_cast<float>(rand() % 50) + 50.0f;
		light[i].position[2] = static_cast<float>(rand() % 1000) - 500.0f;
		light[i].position[3] = 1.0f;
		// just for debugging
		//printf("light %d position : (%f, %f, %f, %f)\n", i, light[i].position[0], light[i].position[1], 
		//	light[i].position[2], light[i].position[3]);

		light[i].ambient_color[0] = 0.13f; light[i].ambient_color[1] = 0.13f;
		light[i].ambient_color[2] = 0.13f; light[i].ambient_color[3] = 1.0f;

		light[i].diffuse_color[0] = light[i].specular_color[0] = static_cast<float>(rand() % 100) / 100.0f;
		light[i].diffuse_color[1] = light[i].specular_color[1] = static_cast<float>(rand() % 100) / 100.0f;
		light[i].diffuse_color[2] = light[i].specular_color[2] = static_cast<float>(rand() % 100) / 100.0f;

		light[i].light_attenuation_factors[0] = 1.0f;
		light[i].light_attenuation_factors[1] = 0.014f;
		light[i].light_attenuation_factors[2] = 0.0007f;
		light[i].light_attenuation_factors[3] = 1.0f;
	}


	// spot_light_WC
	/*light[1].light_on = 1;
	light[1].position[0] = -200.0f; light[1].position[1] = 500.0f; // spot light position in WC
	light[1].position[2] = -200.0f; light[1].position[3] = 1.0f;

	light[1].ambient_color[0] = 0.152f; light[1].ambient_color[1] = 0.152f;
	light[1].ambient_color[2] = 0.152f; light[1].ambient_color[3] = 1.0f;

	light[1].diffuse_color[0] = 0.572f; light[1].diffuse_color[1] = 0.572f;
	light[1].diffuse_color[2] = 0.572f; light[1].diffuse_color[3] = 1.0f;

	light[1].specular_color[0] = 0.772f; light[1].specular_color[1] = 0.772f;
	light[1].specular_color[2] = 0.772f; light[1].specular_color[3] = 1.0f;

	light[1].spot_direction[0] = 0.0f; light[1].spot_direction[1] = -1.0f; // spot light direction in WC
	light[1].spot_direction[2] = 0.0f;
	light[1].spot_cutoff_angle = 20.0f;
	light[1].spot_exponent = 8.0f;*/


	glUseProgram(h_ShaderProgram_lighting);
	
	for (uint32_t i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		glUniform1i(loc_light_lighting[i].light_on, light[i].light_on);
		glUniform4fv(loc_light_lighting[i].position, 1, light[i].position);
		glUniform4fv(loc_light_lighting[i].ambient_color, 1, light[i].ambient_color);
		glUniform4fv(loc_light_lighting[i].diffuse_color, 1, light[i].diffuse_color);
		glUniform4fv(loc_light_lighting[i].specular_color, 1, light[i].specular_color);

		glUniform4fv(loc_light_lighting[i].light_attenuation_factors, 1, light[i].light_attenuation_factors);

		// then calculate radius of light volume/sphere
		const float maxBrightness = std::fmaxf(std::fmaxf(light[i].diffuse_color[0], light[i].diffuse_color[1]),
			light[i].diffuse_color[2]);	// diffuse only?

		float constant = light[i].light_attenuation_factors[0];
		float linear = light[i].light_attenuation_factors[1];
		float quadratic = light[i].light_attenuation_factors[2];

		float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (255.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
		glUniform1f(loc_light_lighting[i].radius, radius);
	}

	// spot light
	/*glUniform1i(loc_light[1].light_on, light[1].light_on);
	// need to supply position in EC for shading
	//glm::vec4 position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
	//											light[1].position[2], light[1].position[3]);
	//glUniform4fv(loc_light[1].position, 1, &position_EC[0]); 
	glUniform4fv(loc_light[1].position, 1, light[1].position);
	glUniform4fv(loc_light[1].ambient_color, 1, light[1].ambient_color);
	glUniform4fv(loc_light[1].diffuse_color, 1, light[1].diffuse_color);
	glUniform4fv(loc_light[1].specular_color, 1, light[1].specular_color);
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	//glm::vec3 direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0], light[1].spot_direction[1], 
	//															light[1].spot_direction[2]);
	//glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]); 
	glUniform3fv(loc_light[1].spot_direction, 1, light[1].spot_direction);
	glUniform1f(loc_light[1].spot_cutoff_angle, light[1].spot_cutoff_angle);
	glUniform1f(loc_light[1].spot_exponent, light[1].spot_exponent);*/

	glUseProgram(0);
}

void prepare_scene(void) {
	//prepare_axes();	
	prepare_floor();
	prepare_tiger();
	set_up_scene_lights();
}

void prepare_gbuffer(void) {
	glGenFramebuffers(1, &g_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);

	// position color buffer
	glGenTextures(1, &g_pos);
	glBindTexture(GL_TEXTURE_2D, g_pos);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_pos, 0);
	// normal color buffer
	glGenTextures(1, &g_norm);
	glBindTexture(GL_TEXTURE_2D, g_norm);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_norm, 0);
	// color + specular color buffer
	glGenTextures(1, &g_albedo_spec);
	glBindTexture(GL_TEXTURE_2D, g_albedo_spec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_albedo_spec, 0);

	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	unsigned int rbo_depth;
	glGenRenderbuffers(1, &rbo_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Framebuffer not complete!\n");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initialize_renderer(void) {
	register_callbacks();

	// for deferred shading
	prepare_gbuffer();

	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 1
void main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 5.4.Tiger_Texture_PS_GLSL";
	char messages[N_MESSAGE_LINES][256] = { "    - Keys used: '0', '1', 'a', 't', 'f', 'c', 'd', 'y', 'u', 'i', 'o', 'ESC'"  };

	glutInit(&argc, argv);
  	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutInitContextVersion(4, 6);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);

	//base_time = std::chrono::high_resolution_clock::now();
	setVSync(0);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}