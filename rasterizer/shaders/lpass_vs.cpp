const char *shader_light_pass_vs =
"\
#version 330 \n\
layout(location=0) in vec2 position;\
smooth out vec2 outPosition;\
void main()\
{\
	outPosition = position;\
	gl_Position = vec4(position, 0.0, 1.0);\
}\
";
