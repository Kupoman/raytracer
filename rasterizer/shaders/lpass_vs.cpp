const char *shader_light_pass_vs =
"\
#version 330 \n\
layout(location=0) in vec2 position;\
smooth out vec2 outTexcoord;\
void main()\
{\
	vec2 remap = vec2(0.5);\
	outTexcoord = position * remap + remap;\
	gl_Position = vec4(position, -1.0, 1.0);\
}\
";
