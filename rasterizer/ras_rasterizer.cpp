#include <stdio.h>
#include <stdlib.h>

#include "GL/glew.h"

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
	printf("%s\n",infoLog);
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

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f,0.3f,0.3f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Rasterizer::drawMeshes()
{
	if (!this->shader_programs["MESH"])
		this->shader_programs["MESH"] = _initShader(shader_mesh_vs, shader_mesh_fs);

	glUseProgram(this->shader_programs["MESH"]);
	int loc = glGetUniformLocation(this->shader_programs["MESH"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16_SNORM, this->camera->getWidth(), this->camera->getHeight(), 0, GL_RGB, GL_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->prepass_color0_target, 0);

	// Position Buffer
	glGenTextures(1, &this->prepass_color1_target);
	glBindTexture(GL_TEXTURE_2D, this->prepass_color1_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->camera->getWidth(), this->camera->getHeight(), 0, GL_RGB, GL_HALF_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, this->prepass_color1_target, 0);

	// Depth Buffer
	glGenTextures(1, &this->prepass_depth_target);
	glBindTexture(GL_TEXTURE_2D, this->prepass_depth_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, this->camera->getWidth(), this->camera->getHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->prepass_depth_target, 0);

	// Make sure everything is fine
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		fprintf(stderr, "Prepass FBO incomplete\n");

	// Clean up state
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Rasterizer::drawPrepass()
{
	if (!this->prepass_color0_target)
		this->initPrepass();

	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_prepass);

	glUseProgram(this->shader_programs["PREPASS"]);
	int loc = glGetUniformLocation(this->shader_programs["PREPASS"], "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

	for (int i=0; i < this->meshes.size(); i++) {
		this->meshes[i]->draw();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
