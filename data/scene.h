#ifndef __DATA_SCENE_H__
#define __DATA_SCENE_H__

#include "data.h"

class Scene {
public:
	std::vector<Light*> lights;
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	std::vector<Texture*> textures;
	class IAccel* mesh_structure;
	class Camera* camera;
	void addMesh(Mesh *mesh);
	Scene();
	~Scene();

};

#endif
