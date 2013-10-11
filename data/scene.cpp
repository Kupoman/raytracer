#include "scene.h"
#include "camera.h"
#include <stdio.h>

#include "raytracer/rt_raytracer.h"
#include "rasterizer/ras_rasterizer.h"

Scene::Scene()
{
	this->camera = new Camera(0.86, 480, 480);
	this->raytracer = new RayTracer();
	this->rasterizer = new Rasterizer();

	this->rasterizer->setCamera(this->camera);
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
	this->rasterizer->addMesh(mesh);
	this->meshes.push_back(mesh);
}

void Scene::draw(unsigned char *output)
{
	int count;
	Eigen::Vector3f *positions, *normals;
	Result *results;
	ResultOffset *result_offsets;

	this->rasterizer->beginFrame();
	this->rasterizer->drawPrepass();
	this->rasterizer->drawLights(this->lights);
	this->rasterizer->drawMeshes();
	this->rasterizer->getRayTraceData(&count, &positions, &normals);
	this->raytracer->processRays(*this->camera, count, positions, normals, &results, &result_offsets);
}
