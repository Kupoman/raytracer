#ifndef __RAS_RASTERIZER__
#define __RAS_RASTERIZER__

#include <vector>

class Rasterizer
{
private:
	class Camera* camera;
	float proj_mat[4][4];

	std::vector<class RasMesh*> meshes;

	unsigned int shader_program;

	void initShader();

public:
	Rasterizer();
	~Rasterizer();

	void beginFrame();

	void setCamera(class Camera *camera);
	void addMesh(class Mesh* mesh);
	void drawMeshes();
};

#endif
