#ifndef __DATA_SCENE_H__
#define __DATA_SCENE_H__

#include "data.h"

class Scene {
public:
	std::vector<Light*> lights;
	std::vector<Mesh*> meshes;
	class IAccel* mesh_structure;
	void addMesh(Mesh *mesh);

	Scene();
	~Scene();

};

#endif
