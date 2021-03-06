#include <stdio.h>
#include <stdlib.h>

#include "GL/glew.h"

#include "data/data.h"
#include "data/camera.h"

#include "shaders/shaders.h"

#include "ras_rasterizer.h"
#include "ras_mesh.h"
#include "ras_vertex.h"

#define USE_PBOS
#define INTERLACE 16

Rasterizer::Rasterizer()
{
	this->camera = NULL;

	this->shader_programs["MESH"] = 0;
	this->shader_programs["PREPASS"] = 0;

	this->prepass_color0_target = 0;
	this->prepass_depth_target = 0;
	this->vao_raydata = 0;

	this->frame_toggle = 0;
	this->frame_interlace = 0;
	this->pbo_positions[0] = 0;
	this->pbo_normals[0] = 0;

	this->default_mat = new Material();
	this->default_mat->diffuse_color = Eigen::Vector3f(0.5, 1.0, 0.0);
	this->default_mat->texture = NULL;

	this->frame_width = this->frame_height= -1;
	this->prepass_resolution = 0.75;
}

Rasterizer::~Rasterizer()
{
	for (unsigned int i=0; i < this->meshes.size(); i++) {
		delete this->meshes[i];
	}
	this->meshes.clear();

	delete [] this->position_transfer_buffer;
	delete [] this->normal_transfer_buffer;

#ifndef USE_PBOS
	delete [] this->prepass_color0_buffer;
	delete [] this->prepass_color1_buffer;
#endif

	glDeleteTextures(1, &this->prepass_color0_target);
	glDeleteTextures(1, &this->prepass_color1_target);
	glDeleteTextures(1, &this->prepass_depth_target);
	glDeleteTextures(1, &this->raypass_target);

	glDeleteFramebuffers(1, &this->fbo_prepass);
	glDeleteFramebuffers(1, &this->fbo_raypass);

	glDeleteBuffers(2, this->pbo_normals);
	glDeleteBuffers(2, this->pbo_positions);
}

static void printInfoLog(GLhandleARB obj)
{
	//This function was found at:
	//http://www.lighthouse3d.com/tutorials/glsl-tutorial/troubleshooting-the-infolog/

	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB,
					 &infologLength);

	if (infologLength > 0)
	{

	infoLog = (char *)malloc(infologLength);
	glGetInfoLogARB(obj, infologLength, &charsWritten, infoLog);
	fprintf(stderr,"%s\n",infoLog);
	free(infoLog);
	}
}

static unsigned int _initShader(const char *vs_source, const char *fs_source)
{
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vs_source, NULL);
	glCompileShader(vertex_shader);
	printInfoLog(vertex_shader);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fs_source, NULL);
	glCompileShader(fragment_shader);
	printInfoLog(fragment_shader);

	unsigned int shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	printInfoLog(shader_program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_program;
}

void Rasterizer::setCamera(class Camera *camera)
{
	this->camera = camera;
}

void Rasterizer::addMesh(Mesh* mesh)
{
	this->meshes.push_back(new RasMesh(mesh));

	Texture* tex = mesh->material->texture;

	if (tex && !this->textures[tex]) {
		unsigned int texid;


		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		this->textures[tex] = texid;
	}
}

static void projectionFromCamera(Camera* camera, float mat[4][4])
{
	float n = 1.0, f = 100.0;
	float scale = tan(camera->getFOV()) * n;
	float r = ((float)camera->getWidth())/camera->getHeight() * scale;
	float t = scale;

	memset(mat, 0, sizeof(float[4][4]));
	mat[0][0] = n / r;
	mat[1][1] = n / t;
	mat[2][2] = -(f + n) / (f - n);
	mat[2][3] = -1.0;
	mat[3][2] = -2 * f * n / (f - n);
}

void Rasterizer::beginFrame()
{
//	glGetError();
	this->frame_toggle ^= 1;
	if (this->camera) {
		projectionFromCamera(this->camera, this->proj_mat);

		if (this->frame_width != this->camera->getWidth() ||
			this->frame_height != this->camera->getHeight())
		{
			this->frame_width = this->camera->getWidth();
			this->frame_height = this->camera->getHeight();
			this->prepass_width = this->prepass_resolution * this->frame_width;
			this->prepass_height = this->prepass_resolution * this->frame_height;
//			this->prepass_color0_buffer = new float[this->prepass_width * this->prepass_height * 4];
//			this->prepass_color1_buffer = new float[this->prepass_width * this->prepass_height * 3];

			this->position_transfer_buffer = new Eigen::Vector3f[this->prepass_width * this->prepass_height];
			this->normal_transfer_buffer = new Eigen::Vector3f[this->prepass_width * this->prepass_height];
		}
	}
//	fprintf(stderr, "%s\n", gluErrorString(glGetError()));
}

void Rasterizer::bindMaterial(Material* material, int program)
{
	int loc;

	loc = glGetUniformLocation(program, "material_color");
	Eigen::Vector3f color = material->diffuse_color /255;
	glUniform3fv(loc, 1, &color(0));

	loc = glGetUniformLocation(program, "material_scolor");
	color = material->specular_color /255;
	glUniform3fv(loc, 1, &color(0));

	loc = glGetUniformLocation(program, "material_shininess");
	glUniform1f(loc, material->shininess);

	loc = glGetUniformLocation(program, "material_reflectivity");
	glUniform1f(loc, material->reflectivity);

	loc = glGetUniformLocation(program, "material_textured");
	glUniform1i(loc, material->texture != NULL);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, this->textures[material->texture]);
	loc = glGetUniformLocation(program, "texture_diffuse");
	glUniform1i(loc, 4);
}

void Rasterizer::bindLights(int program)
{

	char light_name[64];
	Eigen::Vector3f color;
	int loc;

	loc = glGetUniformLocation(program, "lightCount");
	glUniform1i(loc, this->lights.size());

	for (unsigned int i = 0; i < this->lights.size(); i++) {
		sprintf(light_name, "lights[%d].position", i);
		loc = glGetUniformLocation(program, light_name);
		if (loc < 0) {
//			fprintf(stderr, "Out of Lights for %s\n", light_name);
			break;
		}
		glUniform3fv(loc, 1, &this->lights[i]->position[0]);


		sprintf(light_name, "lights[%d].energy", i);
		loc = glGetUniformLocation(program, light_name);
		if (loc < 0) {
//			fprintf(stderr, "Out of Lights for %s\n", light_name);
			break;
		}
		color = this->lights[i]->color * 1.0/255;
		glUniform3fv(loc, 1, &color[0]);
	}
}

void Rasterizer::drawMeshes()
{
	glClearColor(0.3f,0.3f,0.3f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, this->frame_width, this->frame_height);

	if (!this->shader_programs["MESH"])
		this->shader_programs["MESH"] = _initShader(shader_mesh_vs, shader_mesh_fs);

	glUseProgram(this->shader_programs["MESH"]);
	int loc = glGetUniformLocation(this->shader_programs["MESH"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

	loc = glGetUniformLocation(this->shader_programs["MESH"], "transform_draw");
	glUniform1i(loc, 1);

	loc = glGetUniformLocation(this->shader_programs["MESH"], "frame_size");
	glUniform2f(loc, this->frame_width, this->frame_height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->lpass_color0_target);
	loc = glGetUniformLocation(this->shader_programs["MESH"], "lightBuffer");
	glUniform1i(loc, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, this->raypass_target);
	loc = glGetUniformLocation(this->shader_programs["MESH"], "raypassBuffer");
	glUniform1i(loc, 1);

	this->bindLights(this->shader_programs["MESH"]);

	RasMesh *mesh;
	for (unsigned int i=0; i < this->meshes.size(); i++) {
		mesh = this->meshes[i];

		loc = glGetUniformLocation(this->shader_programs["MESH"], "model_mat");
		glUniformMatrix4fv(loc, 1, GL_FALSE, &mesh->getModelMat()(0, 0));

		this->bindMaterial(mesh->getMaterial(), this->shader_programs["MESH"]);
		this->meshes[i]->draw();
	}

	glActiveTexture(GL_TEXTURE0);
}

void Rasterizer::initPrepass()
{
	this->shader_programs["PREPASS"] = _initShader(shader_prepass_vs, shader_prepass_fs);

	glGenFramebuffers(1, &this->fbo_prepass);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_prepass);

	// Normals Buffer
	glGenTextures(1, &this->prepass_color0_target);
	glBindTexture(GL_TEXTURE_2D, this->prepass_color0_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, this->prepass_width, this->prepass_height, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->prepass_color0_target, 0);

	// Position Buffer
	glGenTextures(1, &this->prepass_color1_target);
	glBindTexture(GL_TEXTURE_2D, this->prepass_color1_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->prepass_width, this->prepass_height, 0, GL_RGB, GL_HALF_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, this->prepass_color1_target, 0);

	// Depth Buffer
	glGenTextures(1, &this->prepass_depth_target);
	glBindTexture(GL_TEXTURE_2D, this->prepass_depth_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, this->prepass_width, this->prepass_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->prepass_depth_target, 0);

	// Make sure everything is fine
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		fprintf(stderr, "Prepass FBO incomplete\n");


	GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(2, buffers);

	// Clean up state
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Rasterizer::drawPrepass()
{
	if (!this->prepass_color0_target)
		this->initPrepass();

	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_prepass);

	glViewport(0, 0, this->prepass_width, this->prepass_height);

	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glUseProgram(this->shader_programs["PREPASS"]);
	int loc = glGetUniformLocation(this->shader_programs["PREPASS"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

//	glViewport(0, 0, this->camera->getWidth(), this->camera->getHeight());

	RasMesh *mesh;
	for (unsigned int i=0; i < this->meshes.size(); i++) {
		mesh = this->meshes[i];
		loc = glGetUniformLocation(this->shader_programs["PREPASS"], "isReflective");
		glUniform1i(loc, mesh->getMaterialIsReflective());

		loc = glGetUniformLocation(this->shader_programs["PREPASS"], "model_mat");
		glUniformMatrix4fv(loc, 1, GL_FALSE, &mesh->getModelMat()(0,0));

		mesh->draw();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Rasterizer::initLightPass()
{
	this->shader_programs["LIGHTPASS"] = _initShader(shader_light_pass_vs, shader_light_pass_fs);

	glGenFramebuffers(1, &this->fbo_lpass);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_lpass);

	// Light Buffer
	glGenTextures(1, &this->lpass_color0_target);
	glBindTexture(GL_TEXTURE_2D, this->lpass_color0_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, this->camera->getWidth(), this->camera->getHeight(), 0, GL_RGB, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->lpass_color0_target, 0);

	// Depth Buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->prepass_depth_target, 0);

	// Make sure everything is fine
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		fprintf(stderr, "Prepass FBO incomplete\n");


	GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(1, buffers);

	// Clean up state
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
void Rasterizer::drawLights(std::vector<Light*> lights)
{
	this->lights = lights;
	return;

	if (!this->fbo_lpass)
		this->initLightPass();

	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_lpass);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(this->shader_programs["LIGHTPASS"]);

	int loc;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->prepass_color0_target);
	loc = glGetUniformLocation(this->shader_programs["LIGHTPASS"], "normalBuffer");
	glUniform1i(loc, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, this->prepass_color1_target);
	loc = glGetUniformLocation(this->shader_programs["LIGHTPASS"], "positionBuffer");
	glUniform1i(loc, 1);

	Light* light;
	for (unsigned int i = 0; i < lights.size(); i++) {
		light = lights[i];

		loc = glGetUniformLocation(this->shader_programs["LIGHTPASS"], "light_position");
		glUniform3fv(loc, 1, light->position.data());
		this->drawFullscreenQuad();
	}

	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Rasterizer::drawFullscreenQuad()
{
	float verts[] = {
						1, -1,
						1, 1,
						-1, -1,
						-1, 1
					};

	if (!this->vao_quad) {
		glGenVertexArrays(1, &this->vao_quad);
		glBindVertexArray(this->vao_quad);

		glGenBuffers(1, &this->vbo_quad);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo_quad);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}
	else {
		glBindVertexArray(this->vao_quad);
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
}

void Rasterizer::displayImageData(unsigned char *pixels)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawPixels(this->frame_width, this->frame_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

void Rasterizer::getRayTraceData(int *count, Eigen::Vector3f **positions, Eigen::Vector3f **normals)
{
	*count = 0;
	std::vector<int> frag_idx;
	if (!this->pbo_positions[0]) {
		int size = this->prepass_height * this->prepass_width;

#ifdef USE_PBOS
		size *= sizeof(float);
		glGenBuffers(2, this->pbo_positions);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_positions[0]);
		glBufferData(GL_PIXEL_PACK_BUFFER, size*3, NULL, GL_STREAM_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_positions[1]);
		glBufferData(GL_PIXEL_PACK_BUFFER, size*3, NULL, GL_STREAM_READ);

		glGenBuffers(2, this->pbo_normals);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_normals[0]);
		glBufferData(GL_PIXEL_PACK_BUFFER, size*4, NULL, GL_STREAM_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_normals[1]);
		glBufferData(GL_PIXEL_PACK_BUFFER, size*4, NULL, GL_STREAM_READ);
#else
		this->prepass_color0_buffer = new float[size*4];
		this->prepass_color1_buffer = new float[size*3];
#endif
	}

	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_prepass);

#ifdef USE_PBOS
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_normals[this->frame_toggle]);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, this->prepass_width, this->prepass_height, GL_RGBA, GL_FLOAT, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_normals[this->frame_toggle ^ 1]);
	this->prepass_color0_buffer = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (!this->prepass_color0_buffer) {
		fprintf(stderr, "Error mapping Color0 Buffer\n");
		return;
	}
#else
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, this->prepass_width, this->prepass_height, GL_RGBA, GL_FLOAT, this->prepass_color0_buffer);
#endif

	int line_number = 0;
	for (int i = 0; i < this->prepass_width * this->prepass_height; i++) {
#ifdef INTERLACE
			if (i%INTERLACE != this->frame_interlace) {
				continue;
			}
#endif
		if (this->prepass_color0_buffer[i*4 +3] > 0.0f) {
			frag_idx.push_back(i);
		}
	}

#ifdef INTERLACE
	if (++this->frame_interlace == INTERLACE)
		this->frame_interlace = 0;
#endif

	*count = frag_idx.size();
	*positions = this->position_transfer_buffer;
	*normals = this->normal_transfer_buffer;

	int idx;
	for (int i = 0; i < *count; i++) {
		idx = frag_idx[i];
		(*normals)[i][0] = this->prepass_color0_buffer[idx*4 + 0] * 2.0 - 1.0;
		(*normals)[i][1] = this->prepass_color0_buffer[idx*4 + 1] * 2.0 - 1.0;
		(*normals)[i][2] = this->prepass_color0_buffer[idx*4 + 2] * 2.0 - 1.0;
	}

#ifdef USE_PBOS
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_positions[this->frame_toggle]);
	glReadBuffer(GL_COLOR_ATTACHMENT1);
	glReadPixels(0, 0, this->prepass_width, this->prepass_height, GL_RGB, GL_FLOAT, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_positions[this->frame_toggle^1]);
	this->prepass_color1_buffer = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
#else
	glReadBuffer(GL_COLOR_ATTACHMENT1);
	glReadPixels(0, 0, this->prepass_width, this->prepass_height, GL_RGB, GL_FLOAT, this->prepass_color1_buffer);
#endif

	if (!this->prepass_color1_buffer) {
		fprintf(stderr, "Error mapping Color1 Buffer\n");
		return;
	}

	for (int i = 0; i < *count; i++) {
		idx = frag_idx[i];
		(*positions)[i][0] = this->prepass_color1_buffer[idx*3 + 0];
		(*positions)[i][1] = this->prepass_color1_buffer[idx*3 + 1];
		(*positions)[i][2] = this->prepass_color1_buffer[idx*3 + 2];
	}
#ifdef USE_PBOS
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#endif
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Rasterizer::initRayDataPass()
{
	if (!this->shader_programs["MESH"])
		this->shader_programs["MESH"] = _initShader(shader_mesh_vs, shader_mesh_fs);

	glGenVertexArrays(1, &this->vao_raydata);
	glBindVertexArray(this->vao_raydata);

	glGenBuffers(1, &this->vbo_raydata);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_raydata);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Ray), (void*)offsetof(Ray, origin));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Ray), (void*)offsetof(Ray, normal));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, sizeof(Ray), (void*)offsetof(Ray, texcoord));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Ray), (void*)offsetof(Ray, position));

	glBufferData(GL_ARRAY_BUFFER, this->prepass_height*this->prepass_width*sizeof(Ray), NULL, GL_STREAM_DRAW);

	glGenFramebuffers(1, &this->fbo_raypass);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_raypass);

	// Reflection Buffer
	glGenTextures(1, &this->raypass_target);
	glBindTexture(GL_TEXTURE_2D, this->raypass_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, this->prepass_width, this->prepass_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->raypass_target, 0);

	// Make sure everything is fine
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		fprintf(stderr, "RayPass FBO incomplete\n");


	GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(1, buffers);

	// Clean up state
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Rasterizer::drawRayData(Ray *results, ResultOffset *result_offsets, int result_count, int material_count)
{
	bool first_pass = false;
	if (!this->vao_raydata) {
		this->initRayDataPass();
		first_pass = true;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_raypass);

	glViewport(0, 0, this->prepass_width, this->prepass_height);
	glBindVertexArray(this->vao_raydata);

	glClearColor(1.0, 1.0, 1.0, 1.0);
	if (first_pass)
		glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(this->shader_programs["MESH"]);

	glBufferSubData(GL_ARRAY_BUFFER, 0, result_count * sizeof(Ray), results);

	int loc = glGetUniformLocation(this->shader_programs["MESH"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

	loc = glGetUniformLocation(this->shader_programs["MESH"], "frame_size");
	glUniform2f(loc, this->prepass_width, this->prepass_height);

	loc = glGetUniformLocation(this->shader_programs["MESH"], "transform_draw");
	glUniform1i(loc, 0);

	this->bindLights(this->shader_programs["MESH"]);

	Material *material;
	int start = 0;
	for (int i = 0; i < material_count; i++) {
		material = result_offsets[i].first;

		if (!material) material = this->default_mat;

		this->bindMaterial(material, this->shader_programs["MESH"]);

		glDrawArrays(GL_POINTS, start, result_offsets[i].second);
		start = result_offsets[i].second;
	}

	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
}
