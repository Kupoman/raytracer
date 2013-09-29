#include "scene.h"
#include "camera.h"
#include <stdio.h>

#include "raytracer/rt_raytracer.h"

Scene::Scene()
{
	this->camera = new Camera(0.86, 480, 480);
	this->raytracer = new RayTracer();
}

Scene::~Scene()
{
	for(int i = 0; i < this->meshes.size(); ++i) {
		delete this->meshes[i];
	}

	for (int i = 0; i < this->lights.size(); ++i) {
		delete this->lights[i];
	}

	for (int i = 0; i < this->materials.size(); ++i) {
		delete this->materials[i];
	}
	delete this->camera;
}

void Scene::addMesh(Mesh *mesh)
{
	this->raytracer->addMesh(mesh);
	this->meshes.push_back(mesh);
}

void Scene::draw(unsigned char *output)
{
	this->raytracer->renderScene(*this, this->camera->getWidth(), this->camera->getHeight(), output);
}
