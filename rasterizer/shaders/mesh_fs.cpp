const char *shader_mesh_fs =
"\
#version 330 \n\
uniform sampler2D lightBuffer;\n\
out vec4 fragColor;\n\
void main()\n\
{\n\
	vec3 light = texelFetch(lightBuffer, ivec2(gl_FragCoord.st), 0).rgb;\
	fragColor = vec4(light, 1.0);\n\
}\n\
";
