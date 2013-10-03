const char *shader_prepass_fs =
"\
#version 330 \n\
smooth in vec3 outNormal;\
smooth in vec3 outPosition;\
layout(location = 0) out vec3 normalBuffer;\
layout(location = 1) out vec3 positionBuffer;\
void main()\
{\
	normalBuffer = outNormal;\
	positionBuffer = outPosition;\
}\
";
