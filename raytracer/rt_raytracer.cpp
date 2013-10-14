#include "data/data.h"
#include "data/scene.h"
#include "data/camera.h"

#include "rt_raytracer.h"
#include "rt_photon_map.h"
#include "rt_ray.h"
#include "rt_iaccel.h"

#include "rt_accel_array.h"

#include <stdio.h>
#include <map>
#include <vector>

RayTracer::RayTracer()
{
	this->bounces = 2;

	this->do_shadows = false;

	this->photon_map = NULL;
	this->photon_count = 10000;
	this->photon_estimate = 100;
	this->photon_radius = 1.0;

	this->meshes = new AccelArray();
}

RayTracer::~RayTracer()
{
	if (this->photon_map)
		delete this->photon_map;

	delete this->meshes;
}

void RayTracer::addMesh(Mesh *mesh)
{
	this->meshes->addMesh(mesh);
}

void RayTracer::shade(const Scene& scene, Ray *ray, Result* result, Material* material, Eigen::Vector3f *color, int pass)
{

	if (pass < this->bounces) {
		float bias = 0.1;
		float lambert = 0.0;
		float shadow = 1;
		float specular = 0;

		Eigen::Vector3f V = result->position;
		Eigen::Vector3f N = result->normal;
		Eigen::Vector2f texcoord = result->texcoord;
		Eigen::Vector3f I = *ray->getDirection();

		float ref = material->reflectivity;
		float alpha = material->alpha;
		Eigen::Vector3f diff_color = material->diffuse_color;
		Eigen::Vector3f spec_color = material->specular_color;
		if (material->texture) {
			diff_color = material->texture->lookup(texcoord(0), texcoord(1));
		}



		if (!this->photon_map) {
			for (int i = 0; i < scene.lights.size(); ++i) {
				Eigen::Vector3f light_pos = scene.lights[i]->position;
				Eigen::Vector3f L = light_pos-V;
				Eigen::Vector3f H = (L + I).normalized();

				/* Diffuse */
				lambert += N.dot(L.normalized());
				lambert = std::min(std::max(lambert, 0.0f), 1.0f);

				/* Shadow */
				if (this->do_shadows) {
					Ray light_ray = Ray(V+N*bias, L);
					if (this->meshes->intersect(&light_ray, result, NULL)) {
						float distance = (result->position - V).norm();
						if (distance < L.norm()) {
							shadow = std::max(shadow-0.4, 0.0);
						}
					}
				}

				/* Specular */
				float phong = H.dot(N);
				phong = std::max(phong, 0.0f);
				specular += pow(phong, material->shininess);
			}
		}

		/* Reflection */
		Eigen::Vector3f ref_color = Eigen::Vector3f(0, 0, 0);
		if (material->reflectivity > 0) {
			Eigen::Vector3f R = I - 2 * I.dot(N) * N;
			Ray ref_ray = Ray(V, R);
			if (this->meshes->intersect(&ref_ray, result, &material))
				shade(scene, &ref_ray, result, material, &ref_color, pass+1);
		}

		/* Refraction */
		Eigen::Vector3f refraction_color = Eigen::Vector3f(0, 0, 0);
		if (material->alpha > 0.1) {
			float IoR = material->ior;
			Eigen::Vector3f T = (I - N * I.dot(N))/IoR;
			T -= N * sqrt(1 - ((1 - I.dot(N)*I.dot(N)))/IoR*IoR);
			Ray refract_ray = Ray(V, T);

			if (this->meshes->intersect(&refract_ray, result, &material))
				shade(scene, &refract_ray, result, material, &refraction_color, pass+1);
		}

		Eigen::Vector3f light = Eigen::Vector3f(lambert, lambert, lambert);
		if (this->photon_map) {
			light = photon_map->radiance_estimate(result->position, *ray->getDirection(), result->normal, this->photon_estimate, this->photon_radius);
		}
		*color = shadow * light.array() * ((1.0-ref-alpha)*diff_color + ref*ref_color + alpha*refraction_color).array();
	}
}

void RayTracer::renderScene(const Scene& scene, int width, int height, unsigned char *color)
{

	if (this->photon_map) {
		this->photon_map->generate(&scene, this->meshes, this->photon_count);
	}

	Ray* screenRays = scene.camera->getScreenRays();
	Material* material;
	for (int i = 0; i < width*height; i++) {
		Eigen::Vector3f result;
		Result hitResult;
		if (this->meshes->intersect(&screenRays[i], &hitResult, &material)) {
			shade(scene, &screenRays[i], &hitResult, material, &result, 0);
			color[4*i+0] = (unsigned char)(std::min((int)result(0), 255));
			color[4*i+1] = (unsigned char)(std::min((int)result(1), 255));
			color[4*i+2] = (unsigned char)(std::min((int)result(2), 255));
			color[4*i+3] = 255;
		}
		else {
			color[4*i+0] = 0;
			color[4*i+1] = 0;
			color[4*i+2] = 0;
			color[4*i+3] = 0;
		}
	}
}

typedef std::map<Material*, std::vector<Result> > ResultMap;
void RayTracer::processRays(const Camera& camera, int count, Eigen::Vector3f *positions, Eigen::Vector3f *normals, Result **results, ResultOffset **result_offsets, int *out_count)
{
	Ray ray;
	Eigen::Vector3f direction;

	Result result;
	Material *material = NULL;

	ResultMap result_map;

	for (int i = 0; i < count; i++) {
		direction = positions[i] - 2 * positions[i].dot(normals[i]) * normals[i];

		// Needs to be multiplied by the inverse view matrix!
		ray.setOrigin(positions[i]);
		ray.setDirection(direction);

		if (this->meshes->intersect(&ray, &result, &material)) {
			result.position = *ray.getOrigin();
			result_map[material].push_back(Result(result));
		}
	}

	int offset = 0;
	for (ResultMap::iterator iter = result_map.begin(); iter != result_map.end(); iter++) {
		material = iter->first;
		std::vector<Result> points = iter->second;
		this->result_vec.insert(result_vec.end(), points.begin(), points.end());
		this->offset_vec.push_back(std::pair<Material*, int>(material, offset));

		offset += points.size();
	}

	*out_count = offset;
	*results = &this->result_vec[0];
	*result_offsets = &this->offset_vec[0];
}
