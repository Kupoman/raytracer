#ifndef __RAS_RASTERIZER__
#define __RAS_RASTERIZER__

#include <vector>
#include <map>
#include <string>

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
	void initPrepass();

	unsigned int vbo_quad;
	void drawFullscreenQuad();

public:
	Rasterizer();
	~Rasterizer();

	void beginFrame();

	void setCamera(class Camera *camera);
	void addMesh(class Mesh* mesh);
	void drawPrepass();
	void drawMeshes();
};

#endif
