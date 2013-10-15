#include "data/data.h"
#include "data/scene.h"
#include "data/camera.h"

#include "rt_raytracer.h"
#include "rt_photon_map.h"
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

void RayTracer::shade(const Scene& scene, Ray *ray, Material* material, Eigen::Vector3f *color, int pass)
{

	if (pass < this->bounces) {
		float bias = 0.1;
		float lambert = 0.0;
		float shadow = 1;
		float specular = 0;

		Eigen::Vector3f V = ray->position;
		Eigen::Vector3f N = ray->normal;
		Eigen::Vector2f texcoord = ray->texcoord;
		Eigen::Vector3f I = ray->direction;

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
					Ray light_ray;
					light_ray.origin = V+N*bias;
					light_ray.direction = L;
					if (this->meshes->intersect(&light_ray, NULL)) {
						float distance = (ray->position - V).norm();
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
			Ray ref_ray;
			ref_ray.origin = V;
			ref_ray.direction = R;
			if (this->meshes->intersect(&ref_ray, &material))
				shade(scene, &ref_ray, material, &ref_color, pass+1);
		}

		/* Refraction */
		Eigen::Vector3f refraction_color = Eigen::Vector3f(0, 0, 0);
		if (material->alpha > 0.1) {
			float IoR = material->ior;
			Eigen::Vector3f T = (I - N * I.dot(N))/IoR;
			T -= N * sqrt(1 - ((1 - I.dot(N)*I.dot(N)))/IoR*IoR);
			Ray refract_ray;
			refract_ray.origin = V;
			refract_ray.direction = T;

			if (this->meshes->intersect(&refract_ray, &material))
				shade(scene, &refract_ray, material, &refraction_color, pass+1);
		}

		Eigen::Vector3f light = Eigen::Vector3f(lambert, lambert, lambert);
		if (this->photon_map) {
			light = photon_map->radiance_estimate(ray->position, ray->direction, ray->normal, this->photon_estimate, this->photon_radius);
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
		if (this->meshes->intersect(&screenRays[i], &material)) {
			shade(scene, &screenRays[i], material, &result, 0);
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

typedef std::map<Material*, std::vector<Ray> > ResultMap;
void RayTracer::processRays(const Camera& camera, int count, Eigen::Vector3f *positions, Eigen::Vector3f *normals, Ray **results, ResultOffset **result_offsets, int *out_result_count, int *out_material_count)
{
	Ray ray;
	Eigen::Vector3f direction;

	Material *material = NULL;

	ResultMap result_map;

	this->result_vec.clear();
	this->offset_vec.clear();

	for (int i = 0; i < count; i++) {
		direction = positions[i] - 2 * positions[i].dot(normals[i]) * normals[i];

		// Needs to be multiplied by the inverse view matrix!
		ray.direction = direction;
		ray.origin = positions[i];

		if (!this->meshes->intersect(&ray, &material)) {
			material = NULL;
		}
		//ray.position = ray.origin;
		result_map[material].push_back(Ray(ray));
	}

	int offset = 0;
	for (ResultMap::iterator iter = result_map.begin(); iter != result_map.end(); iter++) {
		material = iter->first;
		std::vector<Ray> rays = iter->second;
		offset += rays.size();
		this->result_vec.insert(result_vec.end(), rays.begin(), rays.end());
		this->offset_vec.push_back(std::pair<Material*, int>(material, offset));
	}

	*out_result_count = this->result_vec.size();
	*out_material_count = this->offset_vec.size();
	*results = &this->result_vec[0];
	*result_offsets = &this->offset_vec[0];
}
