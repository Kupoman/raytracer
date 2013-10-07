const char *shader_prepass_fs =
"\
#version 330 \n\
smooth in vec3 outNormal;\
smooth in vec3 outPosition;\
layout(location = 0) out vec4 normalBuffer;\
layout(location = 1) out vec3 positionBuffer;\
uniform int isReflective;\
void main()\
{\
	normalBuffer.rgb = normalize(outNormal) * 0.5 + 0.5;\
	normalBuffer.a = isReflective;\
	positionBuffer = outPosition;\
}\
";
