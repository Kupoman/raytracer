const char *shader_mesh_fs =
"\
#version 330 \n\
uniform sampler2D prepassBuffer0;\n\
uniform sampler2D lightBuffer;\n\
smooth in vec2 outCoord;\n\
out vec4 fragColor;\n\
\
uniform vec3 material_color;\n\
uniform int material_textured;\n\
uniform sampler2D texture_diffuse;\n\
void main()\n\
{\n\
	vec3 light = texelFetch(lightBuffer, ivec2(gl_FragCoord.st), 0).rgb;\n\
	vec3 texcolor = texture2D(texture_diffuse, outCoord).rgb;\n\
	vec3 albedo = (material_textured==1) ? texcolor : material_color;\n\
	vec3 diffuse = albedo * light;\n\
	fragColor = vec4(diffuse, 1.0);\n\
}\n\
";
