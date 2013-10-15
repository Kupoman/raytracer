const char *shader_mesh_vs =
"\
#version 330 \n\
uniform mat4 proj_mat;\
layout(location=0) in vec3 drawposition;\
layout(location=1) in vec3 normal;\
layout(location=2) in vec2 texcoord;\
layout(location=3) in vec3 fragposition;\
smooth out vec2 outCoord;\
smooth out vec3 outNorm;\
smooth out vec3 outPos;\
void main()\
{\
	gl_Position = proj_mat * vec4(drawposition, 1.0);\
	outCoord = texcoord;\
	outNorm = normal;\
	outPos = fragposition;\
}\
";
