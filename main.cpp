#include <GL/glew.h>
#include <GL/freeglut.h>

#include <FreeImagePlus.h>

#include <algorithm>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "Eigen/Dense"
#include "raytracer/rt_raytracer.h"

#include "data/camera.h"
#include "data/data.h"
#include "data/scene.h"
#include "data/loader.h"


typedef Eigen::Matrix<unsigned char, 3, 1> EigenColor3;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

const unsigned int point_count = WINDOW_WIDTH*WINDOW_HEIGHT;

clock_t g_time = clock();

Scene scene = Scene();

GLuint vertex_buffer;
struct Points {
	float verts[point_count*2];
	unsigned char color[point_count*4];
} points;

GLuint shader_program;

const char *vertex_shader_string = 
"\
#version 330 \n\
layout(location=0) in vec2 position;\
layout(location=1) in vec4 color;\
out vec4 outColor;\
void main()\
{\
	gl_Position = vec4(position, 0.0, 1.0);\
	outColor = color;\
}\
";
const char *fragment_shader_string =
"\
#version 330 \n\
in vec4 outColor;\
out vec4 fragColor;\
void main()\
{\
	if (outColor.a == 0) discard;\
	fragColor = outColor;\
}\
";

void printInfoLog(GLhandleARB obj)
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

void glinit(void)
{
	float x_step = 2.0/WINDOW_WIDTH;
	float y_step = 2.0/WINDOW_HEIGHT;
	int i = 0, j = 0;

	for (int y = 0; y < WINDOW_HEIGHT; y++) {
		for (int x = 0; x < WINDOW_WIDTH; x++) {
			points.verts[i++] = -1+x_step/2.0 + x * x_step;
			points.verts[i++] = -1+y_step/2.0 + y * y_step;
			points.color[j++] = 127;
			points.color[j++] = 255;
			points.color[j++] = 0;
			points.color[j++] = 255;
		}
	}

//	glGenBuffers(1, &vertex_buffer);
//	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(points), &points, GL_STATIC_DRAW);
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
//	glEnableVertexAttribArray(1);
//	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)offsetof(struct Points, color));

//	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
//	glShaderSource(vertex_shader, 1, &vertex_shader_string, NULL);
//	glCompileShader(vertex_shader);
//	printInfoLog(vertex_shader);

//	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
//	glShaderSource(fragment_shader, 1, &fragment_shader_string, NULL);
//	glCompileShader(fragment_shader);
//	printInfoLog(fragment_shader);

//	shader_program = glCreateProgram();
//	glAttachShader(shader_program, vertex_shader);
//	glAttachShader(shader_program, fragment_shader);
//	glLinkProgram(shader_program);
//	printInfoLog(shader_program);

//	glDeleteShader(vertex_shader);
//	glDeleteShader(fragment_shader);

//	glUseProgram(shader_program);

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void glexit(void)
{
	glDeleteBuffers(1, &vertex_buffer);
	glDeleteProgram(shader_program);
}
 
void draw(void)
{
	scene.draw((unsigned char*)&points.color);


	//glBufferSubData(GL_ARRAY_BUFFER, offsetof(Points, color), sizeof(points.color), &points.color);

	//glDrawArrays(GL_POINTS, 0, point_count);

	glutSwapBuffers();
}

void idle(void)
{
	char title[100];

	float seconds = (float)(clock() - g_time) / CLOCKS_PER_SEC;
	glutPostRedisplay();
	sprintf(title, "Fafnir FPS:%.2f", 1.0 / seconds);
	glutSetWindowTitle(title);

	g_time = clock();
}

void exit(void)
{
	glexit();
}

int main(int argc, char **argv)
{
	char file[256];
	if (argc > 1) {
		strcpy(file, argv[1]);
	}
	else {
		printf("No file specified.\n");
		exit(1);
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE);

	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_CORE_PROFILE | GLUT_DEBUG);
 

	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	glutCreateWindow("Ray Tracer");


	glutDisplayFunc(draw);
	glutIdleFunc(idle);
	glutCloseFunc(exit);

	glewInit();
	glinit();

	loadFile(file, &scene);
	scene.camera->setHeight(WINDOW_HEIGHT);
	scene.camera->setWidth(WINDOW_WIDTH);

	glutMainLoop();

	return 0;
}
