const char *shader_mesh_fs =
"\
#version 330 \n\
uniform sampler2D lightBuffer;\n\
out vec4 fragColor;\n\
\
uniform vec3 material_color;\
void main()\n\
{\n\
	vec3 light = texelFetch(lightBuffer, ivec2(gl_FragCoord.st), 0).rgb;\
	vec3 diffuse = material_color * light;\
	fragColor = vec4(diffuse, 1.0);\n\
}\n\
";
