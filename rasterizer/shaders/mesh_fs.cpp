const char *shader_mesh_fs =
"\
#version 330 \n\
#line 5\n\
#define LIGHT_COUNT 8\n\
struct Light {\n\
	vec3 position;\n\
	vec3 energy;\n\
};\n\
uniform Light lights[LIGHT_COUNT];\n\
uniform int lightCount;\n\
uniform sampler2D raypassBuffer;\n\
uniform sampler2D lightBuffer;\n\
uniform vec2 frame_size;\n\
smooth in vec2 outCoord;\n\
smooth in vec3 outNorm;\n\
smooth in vec3 outPos;\n\
out vec4 fragColor;\n\
\n\
uniform vec3 material_color;\n\
uniform vec3 material_scolor;\n\
uniform float material_shininess;\n\
uniform int material_textured;\n\
uniform float material_reflectivity;\n\
uniform sampler2D texture_diffuse;\n\
void main()\n\
{\n\
	/*vec3 light = texelFetch(lightBuffer, ivec2(gl_FragCoord.st), 0).rgb;*/\n\
	vec3 texcolor = texture2D(texture_diffuse, outCoord).rgb;\n\
	vec3 albedo = (material_textured==1) ? texcolor : material_color;\n\
	vec3 N = normalize(outNorm);\n\
	vec3 E = normalize(-outPos);\n\
	vec3 direct = vec3(0.0);\n\
	vec3 L, H, energy;\n\
	float lambert, phong;\n\
	for (int i = 0; i < lightCount; i++) {\n\
			L = lights[i].position - outPos;\n\
			energy = lights[i].energy;\n\
			energy *= 50.0 / (50.0 + length(L)*length(L));\n\
			L = normalize(L);\n\
			H = normalize(L+E);\n\
			lambert = clamp(dot(N, L), 0.0, 1.0);\n\
			phong = pow(clamp(dot(N, H), 0.0, 1.0), material_shininess);\n\
			direct = energy * albedo * lambert + energy * material_scolor * phong + direct;\n\
	}\n\
	vec3 reflection = texture2D(raypassBuffer, gl_FragCoord.st/frame_size).rgb;\n\
	vec3 color = material_reflectivity*reflection + (1.0-material_reflectivity)*direct;\n\
	fragColor = vec4(color, 1.0);\n\
}\n\
";
