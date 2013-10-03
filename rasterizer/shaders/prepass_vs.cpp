const char *shader_prepass_vs =
"\
#version 330 \n\
uniform mat4 proj_mat;\
layout(location=0) in vec3 position;\
layout(location=1) in vec3 normal;\
smooth out vec3 outNormal;\
smooth out vec3 outPosition;\
void main()\
{\
	outPosition = position;\
	gl_Position = proj_mat * vec4(position, 1.0);\
	outNormal = normal;\
}\
";
