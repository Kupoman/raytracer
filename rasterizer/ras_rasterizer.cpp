#include <stdio.h>
#include <stdlib.h>

#include "GL/glew.h"

#include "data/data.h"
#include "data/camera.h"

#include "shaders/shaders.h"

#include "ras_rasterizer.h"
#include "ras_mesh.h"

Rasterizer::Rasterizer()
{
	this->shader_programs["MESH"] = 0;
	this->shader_programs["PREPASS"] = 0;

	this->prepass_color0_target = 0;
	this->prepass_depth_target = 0;
}

Rasterizer::~Rasterizer()
{
	for (int i=0; i < this->meshes.size(); i++) {
		delete this->meshes[i];
	}
	this->meshes.clear();
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

void Rasterizer::addMesh(class Mesh* mesh)
{
	this->meshes.push_back(new RasMesh(mesh));
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
	if (this->camera)
		projectionFromCamera(this->camera, this->proj_mat);
}

void Rasterizer::drawMeshes()
{
	glEnable(GL_DEPTH_TEST);
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

	for (int i=0; i < this->meshes.size(); i++) {
		this->meshes[i]->draw();
	}
}

void Rasterizer::initPrepass()
{
	this->shader_programs["PREPASS"] = _initShader(shader_prepass_vs, shader_prepass_fs);

	glGenFramebuffers(1, &this->fbo_prepass);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_prepass);

	// Normals Buffer
	glGenTextures(1, &this->prepass_color0_target);
	glBindTexture(GL_TEXTURE_2D, this->prepass_color0_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->camera->getWidth(), this->camera->getHeight(), 0, GL_RGB, GL_HALF_FLOAT, NULL);
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

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glUseProgram(this->shader_programs["PREPASS"]);
	int loc = glGetUniformLocation(this->shader_programs["PREPASS"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

	glViewport(0, 0, this->camera->getWidth(), this->camera->getHeight());
	for (int i=0; i < this->meshes.size(); i++) {
		this->meshes[i]->draw();
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
