const char *shader_mesh_vs =
"\
#version 330 \n\
uniform mat4 proj_mat;\
layout(location=0) in vec3 position;\
layout(location=2) in vec2 texcoord;\
smooth out vec2 outCoord;\
void main()\
{\
	gl_Position = proj_mat * vec4(position, 1.0);\
	outCoord = texcoord;\
}\
";
