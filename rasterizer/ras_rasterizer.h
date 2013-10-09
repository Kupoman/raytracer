#ifndef __RAS_RASTERIZER__
#define __RAS_RASTERIZER__

#include <vector>
#include <map>
#include <string>

#include "Eigen/Dense"

class Camera;
class Mesh;
struct Light;

class Rasterizer
{
private:
	class Camera* camera;
	int frame_width, frame_height;

	float proj_mat[4][4];

	std::vector<class RasMesh*> meshes;

	std::map<std::string, unsigned int> shader_programs;

	std::map<class Texture*, unsigned int> textures;

	unsigned int prepass_color0_target;
	float *prepass_color0_buffer;
	unsigned int prepass_color1_target;
	float *prepass_color1_buffer;
	unsigned int prepass_depth_target;
	unsigned int fbo_prepass;

	unsigned int lpass_color0_target;
	unsigned int fbo_lpass;

	Eigen::Vector3f *position_transfer_buffer;
	Eigen::Vector3f *normal_transfer_buffer;

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

	void getRayTraceData(int *count, Eigen::Vector3f **positions, Eigen::Vector3f **normals);
};

#endif
