#include <stdio.h>
#include <stdlib.h>

#include "GL/glew.h"

#include "data/data.h"
#include "data/camera.h"

#include "shaders/shaders.h"

#include "ras_rasterizer.h"
#include "ras_mesh.h"
#include "ras_vertex.h"

Rasterizer::Rasterizer()
{
	this->shader_programs["MESH"] = 0;
	this->shader_programs["PREPASS"] = 0;

	this->prepass_color0_target = 0;
	this->prepass_depth_target = 0;
	this->vao_raydata = 0;

	this->default_mat = new Material();
	this->default_mat->diffuse_color = Eigen::Vector3f(0.5, 1.0, 0.0);
	this->default_mat->texture = NULL;
}

Rasterizer::~Rasterizer()
{
	for (int i=0; i < this->meshes.size(); i++) {
		delete this->meshes[i];
	}
	this->meshes.clear();

	delete [] this->position_transfer_buffer;
	delete [] this->normal_transfer_buffer;
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
	//glGetError();
	if (this->camera) {
		projectionFromCamera(this->camera, this->proj_mat);

		if (this->frame_width != this->camera->getWidth() ||
			this->frame_height != this->camera->getHeight())
		{
			this->frame_width = this->camera->getWidth();
			this->frame_height = this->camera->getHeight();
			this->prepass_color0_buffer = new float[camera->getHeight() * camera->getWidth() * 4];
			this->prepass_color1_buffer = new float[camera->getHeight() * camera->getWidth() * 3];

			this->position_transfer_buffer = new Eigen::Vector3f[this->frame_width * this->frame_height];
			this->normal_transfer_buffer = new Eigen::Vector3f[this->frame_width * this->frame_height];
		}
	}
//	fprintf(stderr, "%s\n", gluErrorString(glGetError()));
}

void Rasterizer::drawMeshes()
{
	glClearColor(0.3f,0.3f,0.3f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!this->shader_programs["MESH"])
		this->shader_programs["MESH"] = _initShader(shader_mesh_vs, shader_mesh_fs);

	glUseProgram(this->shader_programs["MESH"]);
	int loc = glGetUniformLocation(this->shader_programs["MESH"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->lpass_color0_target);
	loc = glGetUniformLocation(this->shader_programs["MESH"], "lightBuffer");
	glUniform1i(loc, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, this->raypass_target);
	loc = glGetUniformLocation(this->shader_programs["MESH"], "raypassBuffer");
	glUniform1i(loc, 1);

	// Lights
	char light_name[64];
	Eigen::Vector3f color;

	loc = glGetUniformLocation(this->shader_programs["MESH"], "lightCount");
	glUniform1i(loc, this->lights.size());

	for (int i = 0; i < this->lights.size(); i++) {
		sprintf(light_name, "lights[%d].position", i);
		loc = glGetUniformLocation(this->shader_programs["MESH"], light_name);
		if (loc < 0) {
			fprintf(stderr, "Out of Lights for %s\n", light_name);
			break;
		}
		glUniform3fv(loc, 1, &this->lights[i]->position[0]);


		sprintf(light_name, "lights[%d].energy", i);
		loc = glGetUniformLocation(this->shader_programs["MESH"], light_name);
		if (loc < 0) {
			fprintf(stderr, "Out of Lights for %s\n", light_name);
			break;
		}
		color = this->lights[i]->color * 1.0/255;
		glUniform3fv(loc, 1, &color[0]);
	}

	RasMesh *mesh;
	for (int i=0; i < this->meshes.size(); i++) {
		mesh = this->meshes[i];

		loc = glGetUniformLocation(this->shader_programs["MESH"], "material_color");
		float color[3];
		mesh->getMaterialDiffColor(color);
		glUniform3fv(loc, 1, color);

		loc = glGetUniformLocation(this->shader_programs["MESH"], "material_reflectivity");
		glUniform1f(loc, mesh->getMaterialReflectivity());

		loc = glGetUniformLocation(this->shader_programs["MESH"], "material_textured");
		glUniform1i(loc, mesh->getMaterialTexture() != NULL);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, this->textures[mesh->getMaterialTexture()]);
		loc = glGetUniformLocation(this->shader_programs["MESH"], "texture_diffuse");
		glUniform1i(loc, 4);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, this->camera->getWidth(), this->camera->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->prepass_color0_target, 0);

	// Position Buffer
	glGenTextures(1, &this->prepass_color1_target);
	glBindTexture(GL_TEXTURE_2D, this->prepass_color1_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->camera->getWidth(), this->camera->getHeight(), 0, GL_RGB, GL_HALF_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, this->prepass_color1_target, 0);

	// Depth Buffer
	glGenTextures(1, &this->prepass_depth_target);
	glBindTexture(GL_TEXTURE_2D, this->prepass_depth_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, this->camera->getWidth(), this->camera->getHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glUseProgram(this->shader_programs["PREPASS"]);
	int loc = glGetUniformLocation(this->shader_programs["PREPASS"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

	glViewport(0, 0, this->camera->getWidth(), this->camera->getHeight());

	RasMesh *mesh;
	for (int i=0; i < this->meshes.size(); i++) {
		mesh = this->meshes[i];
		loc = glGetUniformLocation(this->shader_programs["PREPASS"], "isReflective");
		glUniform1i(loc, mesh->getMaterialIsReflective());

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
	for (int i = 0; i < lights.size(); i++) {
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

	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_prepass);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, this->frame_width, this->frame_height, GL_RGBA, GL_FLOAT, this->prepass_color0_buffer);
	for (int i = 0; i < this->frame_width * this->frame_height; i++) {
		if (this->prepass_color0_buffer[i*4 +3] > 0.0) {
			frag_idx.push_back(i);
		}
	}
	*count = frag_idx.size();

	glReadBuffer(GL_COLOR_ATTACHMENT1);
	glReadPixels(0, 0, this->frame_width, this->frame_height, GL_RGB, GL_FLOAT, this->prepass_color1_buffer);

	*positions = this->position_transfer_buffer;
	*normals = this->normal_transfer_buffer;

	for (int i = 0; i < *count; i++) {
		int idx = frag_idx[i];

		(*normals)[i][0] = this->prepass_color0_buffer[idx*4 + 0] * 2.0 - 1.0;
		(*normals)[i][1] = this->prepass_color0_buffer[idx*4 + 1] * 2.0 - 1.0;
		(*normals)[i][2] = this->prepass_color0_buffer[idx*4 + 2] * 2.0 - 1.0;

		(*positions)[i][0] = this->prepass_color1_buffer[idx*3 + 0];
		(*positions)[i][1] = this->prepass_color1_buffer[idx*3 + 1];
		(*positions)[i][2] = this->prepass_color1_buffer[idx*3 + 2];
	}

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

	glBufferData(GL_ARRAY_BUFFER, this->frame_height*this->frame_width*sizeof(Ray), NULL, GL_STREAM_DRAW);

	glGenFramebuffers(1, &this->fbo_raypass);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_raypass);

	// Reflection Buffer
	glGenTextures(1, &this->raypass_target);
	glBindTexture(GL_TEXTURE_2D, this->raypass_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, this->frame_width, this->frame_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
	if (!this->vao_raydata) {
		this->initRayDataPass();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_raypass);

	glViewport(0, 0, this->frame_width, this->frame_height);
	glBindVertexArray(this->vao_raydata);

	glClearColor(0.5, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(this->shader_programs["MESH"]);

	glBufferSubData(GL_ARRAY_BUFFER, 0, result_count * sizeof(Ray), results);

	int loc = glGetUniformLocation(this->shader_programs["MESH"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

	// Lights
	char light_name[64];
	Eigen::Vector3f color;

	loc = glGetUniformLocation(this->shader_programs["MESH"], "lightCount");
	glUniform1i(loc, this->lights.size());

	for (int i = 0; i < this->lights.size(); i++) {
		sprintf(light_name, "lights[%d].position", i);
		loc = glGetUniformLocation(this->shader_programs["MESH"], light_name);
		if (loc < 0) {
			fprintf(stderr, "Out of Lights for %s\n", light_name);
			break;
		}
		glUniform3fv(loc, 1, &this->lights[i]->position[0]);


		sprintf(light_name, "lights[%d].energy", i);
		loc = glGetUniformLocation(this->shader_programs["MESH"], light_name);
		if (loc < 0) {
			fprintf(stderr, "Out of Lights for %s\n", light_name);
			break;
		}
		color = this->lights[i]->color * 1.0/255;
		glUniform3fv(loc, 1, &color[0]);
	}

	Material *material;
	float mat_color[3];
	int start = 0;
	for (int i = 0; i < material_count; i++) {
		material = result_offsets[i].first;

		if (!material) material = this->default_mat;

		loc = glGetUniformLocation(this->shader_programs["MESH"], "material_color");
		mat_color[0] = material->diffuse_color[0] / 255;
		mat_color[1] = material->diffuse_color[1] / 255;
		mat_color[2] = material->diffuse_color[2] / 255;
		glUniform3fv(loc, 1, mat_color);

		loc = glGetUniformLocation(this->shader_programs["MESH"], "material_reflectivity");
		glUniform1f(loc, 0.0);

		loc = glGetUniformLocation(this->shader_programs["MESH"], "material_textured");
		glUniform1i(loc, material->texture != NULL);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, this->textures[material->texture]);
		loc = glGetUniformLocation(this->shader_programs["MESH"], "texture_diffuse");
		glUniform1i(loc, 4);

		glDrawArrays(GL_POINTS, start, result_offsets[i].second);
		start = result_offsets[i].second;
	}

	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
}
