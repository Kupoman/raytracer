const char *shader_light_pass_fs =
"\
#version 330 \n\
smooth in vec2 outPosition;\
layout(location = 0) out vec4 lightBuffer;\
void main()\
{\
	lightBuffer = vec4(0.5, 1.0, 0.0, 1.0);\
}\
";
