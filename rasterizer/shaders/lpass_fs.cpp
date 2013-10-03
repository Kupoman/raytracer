const char *shader_light_pass_fs =
"\
#version 330 \n\
uniform sampler2D normalBuffer;\
uniform sampler2D positionBuffer;\
\
uniform vec3 light_position = vec3(0.0, 0.0, 0.0);\
smooth in vec2 outTexcoord;\
layout(location = 0) out vec4 lightBuffer;\
void main()\
{\
	vec3 N = texture2D(normalBuffer, outTexcoord).rgb;\
	vec3 V = texture2D(positionBuffer, outTexcoord).rgb;\
	vec3 L = normalize(light_position - V);\
	float lambert = dot(N, L);\
	vec3 diffuse = lambert * vec3(1.0, 1.0, 1.0);\
	lightBuffer = vec4(diffuse, 1.0);\
}\
";
