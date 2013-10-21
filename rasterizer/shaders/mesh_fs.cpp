const char *shader_mesh_fs =
"\
#version 330 \n\
#define LIGHT_COUNT 8\n\
struct Light {\n\
	vec3 position;\n\
	vec3 energy;\n\
};\n\
uniform Light lights[LIGHT_COUNT];\n\
uniform int lightCount;\n\
uniform sampler2D raypassBuffer;\n\
uniform sampler2D lightBuffer;\n\
smooth in vec2 outCoord;\n\
smooth in vec3 outNorm;\n\
smooth in vec3 outPos;\n\
out vec4 fragColor;\n\
\
uniform vec3 material_color;\n\
uniform int material_textured;\n\
uniform float material_reflectivity;\n\
uniform sampler2D texture_diffuse;\n\
void main()\n\
{\n\
	/*vec3 light = texelFetch(lightBuffer, ivec2(gl_FragCoord.st), 0).rgb;*/\n\
	vec3 light = vec3(0.0);\n\
	vec3 L;\n\
	float lambert;\n\
	for (int i = 0; i < lightCount; i++) {\n\
			L = normalize(lights[i].position - outPos);\n\
			lambert = dot(outNorm, L);\n\
			light = lights[i].energy * lambert + light;\n\
	}\n\
	vec3 texcolor = texture2D(texture_diffuse, outCoord).rgb;\n\
	vec3 albedo = (material_textured==1) ? texcolor : material_color;\n\
	vec3 reflection = texelFetch(raypassBuffer, ivec2(gl_FragCoord.st), 0).rgb;\n\
	vec3 diffuse = material_reflectivity*reflection + (1.0-material_reflectivity)*albedo;\n\
	diffuse *= light;\n\
	fragColor = vec4(diffuse, 1.0);\n\
}\n\
";
