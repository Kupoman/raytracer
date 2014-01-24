const char *shader_prepass_vs =
"\
#version 330 \n\
uniform mat4 proj_mat;\
uniform mat4 model_mat;\
layout(location=0) in vec3 position;\
layout(location=1) in vec3 normal;\
smooth out vec3 outNormal;\
smooth out vec3 outPosition;\
void main()\
{\
	vec4 pos = model_mat * vec4(position, 1.0);\
	outPosition = pos.xyz;\
	gl_Position = proj_mat * pos;\
	outNormal = mat3(model_mat) * normal;\
}\
";
