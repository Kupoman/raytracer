#ifndef __DATA_SCENE_H__
#define __DATA_SCENE_H__

#include "data.h"

class Scene {
public:
	std::vector<Light*> lights;
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	std::vector<Texture*> textures;
	class Camera* camera;
	class RayTracer* raytracer;
	void addMesh(Mesh *mesh);
	Scene();
	~Scene();

	void draw(unsigned char* output);

};

#endif
