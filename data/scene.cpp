#include "scene.h"
#include "camera.h"
#include <stdio.h>

#include "raytracer/rt_accel_spheres.h"

Scene::Scene()
{
	this->mesh_structure = new AccelSpheres();
	this->camera = new Camera(0.86, 480, 480);
}

Scene::~Scene()
{
	for(int i = 0; i < this->meshes.size(); ++i) {
		delete this->meshes[i];
	}

	delete this->mesh_structure;
	delete this->camera;
}

void Scene::addMesh(Mesh *mesh)
{
	this->mesh_structure->addMesh(mesh);
	this->meshes.push_back(mesh);
}
