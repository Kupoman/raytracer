const char *shader_mesh_vs =
"\
#version 330 \n\
uniform mat4 proj_mat;\
layout(location=0) in vec3 position;\
out vec4 outColor;\
void main()\
{\
	gl_Position = proj_mat * vec4(position, 1.0);\
}\
";
