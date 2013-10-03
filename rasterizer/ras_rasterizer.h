#ifndef __RAS_RASTERIZER__
#define __RAS_RASTERIZER__

#include <vector>
#include <map>
#include <string>

class Camera;
class Mesh;
struct Light;

class Rasterizer
{
private:
	class Camera* camera;
	float proj_mat[4][4];

	std::vector<class RasMesh*> meshes;

	std::map<std::string, unsigned int> shader_programs;

	unsigned int prepass_color0_target;
	unsigned int prepass_color1_target;
	unsigned int prepass_depth_target;
	unsigned int fbo_prepass;

	unsigned int lpass_color0_target;
	unsigned int fbo_lpass;

	void initPrepass();
	void initLightPass();

	unsigned int vbo_quad;
	unsigned int vao_quad;
	void drawFullscreenQuad();

public:
	Rasterizer();
	~Rasterizer();

	void beginFrame();

	void setCamera(Camera *camera);
	void addMesh(Mesh* mesh);
	void drawPrepass();
	void drawLights(std::vector<Light*> lights);
	void drawMeshes();
};

#endif
