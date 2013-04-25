#include "scene.h"
#include <stdio.h>

#include "raytracer/rt_accel_spheres.h"

Scene::Scene()
{
	this->mesh_structure = new AccelSpheres();
}

Scene::~Scene()
{
//	fprintf(stderr, "Destroying scene %d\n", this->meshes.size());
	for(int i = 0; i < this->meshes.size(); ++i) {
//		fprintf(stderr, "%.2f, %.2f, %.2f\n", this->meshes[i]->position(0),
//				this->meshes[i]->position(1),this->meshes[i]->position(2));
		delete this->meshes[i];
	}

	delete this->mesh_structure;
}

void Scene::addMesh(Mesh *mesh)
{
	this->mesh_structure->addMesh(mesh);
	this->meshes.push_back(mesh);
}
