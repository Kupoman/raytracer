#include "data/data.h"
#include "data/scene.h"
#include "data/camera.h"

#include "rt_raytracer.h"
#include "rt_photon_map.h"
#include "rt_ray.h"
#include "rt_iaccel.h"

RayTracer::RayTracer()
{
	this->bounces = 2;

	this->do_shadows = false;

	this->photon_map = NULL;
	this->photon_count = 10000;
	this->photon_estimate = 100;
	this->photon_radius = 1.0;
}

RayTracer::~RayTracer()
{
	if (this->photon_map)
		delete this->photon_map;
}

void RayTracer::shade(Scene& scene, Ray *ray, Result* result, Eigen::Vector3f *color, int pass)
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
		Material* material = result->material;


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
					scene.mesh_structure->intersect(&light_ray, result);
					if (result->hit) {
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
			scene.mesh_structure->intersect(&ref_ray, result);
			if (result->hit)
				shade(scene, &ref_ray, result, &ref_color, pass+1);
		}

		/* Refraction */
		Eigen::Vector3f refraction_color = Eigen::Vector3f(0, 0, 0);
		if (material->alpha > 0.1) {
			float IoR = material->ior;
			Eigen::Vector3f T = (I - N * I.dot(N))/IoR;
			T -= N * sqrt(1 - ((1 - I.dot(N)*I.dot(N)))/IoR*IoR);
			Ray refract_ray = Ray(V, T);

			scene.mesh_structure->intersect(&refract_ray, result);
			if (result->hit)
				shade(scene, &refract_ray, result, &refraction_color, pass+1);
		}

		float ref = material->reflectivity;
		float alpha = material->alpha;
		Eigen::Vector3f diff_color = material->diffuse_color;
		Eigen::Vector3f spec_color = material->specular_color;
		if (material->texture) {
			diff_color = material->texture->lookup(texcoord(0), texcoord(1));
		}

		Eigen::Vector3f light = Eigen::Vector3f(lambert, lambert, lambert);
		if (this->photon_map) {
			light = photon_map->radiance_estimate(result->position, *ray->getDirection(), result->normal, this->photon_estimate, this->photon_radius);
		}
		*color = shadow * light.array() * ((1.0-ref-alpha)*diff_color + ref*ref_color + alpha*refraction_color).array();
	}
}

void RayTracer::renderScene(Scene& scene, int width, int height, unsigned char *color)
{

	if (this->photon_map) {
		this->photon_map->generate(&scene, this->photon_count);
	}

	Ray* screenRays = scene.camera->getScreenRays();
	for (int i = 0; i < width*height; i++) {
		Eigen::Vector3f result;
		Result hitResult;
		scene.mesh_structure->intersect(&screenRays[i], &hitResult);
		if (hitResult.hit) {
			shade(scene, &screenRays[i], &hitResult, &result, 0);
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
