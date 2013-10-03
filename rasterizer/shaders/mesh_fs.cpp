const char *shader_mesh_fs =
"\
#version 330 \n\
uniform sampler2D normal;\n\
out vec4 fragColor;\n\
void main()\n\
{\n\
	vec3 N = texelFetch(normal, ivec2(gl_FragCoord.st), 0).rgb;\
	fragColor = vec4(N, 1.0);\n\
}\n\
";
