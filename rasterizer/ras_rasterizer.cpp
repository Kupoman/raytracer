#include <stdio.h>
#include <stdlib.h>

#include "GL/glew.h"

#include "data/camera.h"

#include "ras_rasterizer.h"
#include "ras_mesh.h"

static const char *vertex_shader_string =
"\
#version 330 \n\
uniform mat4 proj_mat;\
layout(location=0) in vec3 position;\
out vec4 outColor;\
void main()\
{\
	gl_Position = proj_mat * vec4(position, 1.0);\
}\
";

static const char *fragment_shader_string =
"\
#version 330 \n\
out vec4 fragColor;\
void main()\
{\
	fragColor = vec4(0.0, 0.0, 1.0, 1.0);\
}\
";

Rasterizer::Rasterizer()
{
	this->shader_program = 0;
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

void Rasterizer::initShader()
{
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_string, NULL);
	glCompileShader(vertex_shader);
	printInfoLog(vertex_shader);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_string, NULL);
	glCompileShader(fragment_shader);
	printInfoLog(fragment_shader);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	printInfoLog(shader_program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
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

	glClearColor(0.3f,0.3f,0.3f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Rasterizer::drawMeshes()
{
	if (!this->shader_program)
		this->initShader();

	glUseProgram(this->shader_program);
	int loc = glGetUniformLocation(this->shader_program, "proj_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &this->proj_mat[0][0]);

	for (int i=0; i < this->meshes.size(); i++) {
		this->meshes[i]->draw();
	}
}
