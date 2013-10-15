#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <iostream>

#include "rt_photon_map.h"
#include "rt_iaccel.h"
#include "data/scene.h"
#include "Eigen/Dense"

#define FILTER_GAUSS

const float K = 1.0;
const float ALPHA = 0.918;
const float BETA = 1.953;
const float PI = 3.141592653589793;
const float E = 2.718281828459045;
const float GAUSS_DENOM = 1 / (1 - pow(E, -BETA));

float rmax_2 = RAND_MAX/2.0;

class Photon
{
public:
	Eigen::Vector3f position;
	Eigen::Vector3f energy;
	Eigen::Vector3f direction;
	float distance2;
	short flag;
};

void PhotonMap::emit_photon(IAccel *meshes, Ray* r, Eigen::Vector3f energy, float max_dist, int pass)
{
	Material *material;

	if (pass > 1) return;

	if (!meshes->intersect(r, &material)) return;

//	float dist = 0;
	float dist = (r->position - r->origin).norm();
	if (dist > max_dist) return;
//	energy *=  (max_dist - dist) / max_dist;

	float absorb = (pass == 1) ? 1.0 : 0.6;

	if (((float)rand())/RAND_MAX < absorb) {
		Photon* p = new Photon();
		p->position = r->position;
		p->energy = energy;// * (1-diff_ref);
		p->direction = r->direction;
		this->photons.push_back(p);
	}
	else {
		r->origin = Eigen::Vector3f(r->position(0), r->position(1), r->position(2));
		Eigen::Vector3f dir = Eigen::Vector3f(1, 1, 1);
		while (dir.dot(dir) > 1 || dir.dot(r->normal) < 0) {
			dir(0) = rand() / rmax_2 - 1;
			dir(1) = rand() / rmax_2 - 1;
			dir(2) = rand() / rmax_2 - 1;
		}
		r->direction = Eigen::Vector3f(dir(0), dir(1), dir(2));
		energy = material->diffuse_color * (energy.norm()/255);
//		std::cout << energy.norm()/255 <<std::endl;
		//energy *= diff_ref;
		this->emit_photon(meshes, r, energy, max_dist-dist, pass++);
	}

}

static bool sort_x(Photon* a, Photon* b)
{
	return a->position[0] < b->position[0];
}

static bool sort_y(Photon* a, Photon* b)
{
	return a->position[1] < b->position[1];
}

static bool sort_z(Photon* a, Photon* b)
{
	return a->position[2] < b->position[2];
}

void PhotonMap::generate(const Scene* scene, IAccel *meshes, int count)
{
	Eigen::Vector3f dir;
	Eigen::Vector3f energy;
	Ray ray;
	Light* light;
	float max_dist = 12;
	float rmax_2 = RAND_MAX/2.0;
	srand(time(NULL));
	for (int i = 0; i < scene->lights.size(); ++i) {
		light = scene->lights[i];
		energy = 5*light->color/count;
		for (int j = 0; j < count;) {
			dir(0) = rand() / rmax_2 - 1;
			dir(1) = rand() / rmax_2 - 1;
			dir(2) = -rand() / (float)RAND_MAX;
			
			if (dir.dot(dir) > 1) continue;
			j++;
			

			ray.origin = Eigen::Vector3f(light->position(0), light->position(1), light->position(2));
			ray.direction = Eigen::Vector3f(dir(0), dir(1), dir(2));

			this->emit_photon(meshes, &ray, energy, max_dist, 0);
			continue;
		}
	}

//	float scale = 1.0 / this->photons.size();
	std::cout << this->photons.size() << std::endl;
	Eigen::Vector3f esum = Eigen::Vector3f(0, 0, 0);
	for (int i = 0; i < this->photons.size(); ++i)
	{
		esum += this->photons[i]->energy;
//		this->photons[i]->energy *= scale;
	}
	std::cout << "Total energy: " << esum << std::endl;
}

Eigen::Vector3f PhotonMap::lookup(Eigen::Vector3f position, float radius)
{
	float r2 = radius * radius;
	Photon* p;
	Eigen::Vector3f result = Eigen::Vector3f(0, 0, 0);
	Eigen::Vector3f difference;
	for (int i = 0; i < this->photons.size(); ++i) {
		p = this->photons[i];

		difference = p->position - position;
		if (difference.dot(difference) < r2) {
			result += p->energy;
		}
	}

	return result;
}

static bool _photon_sort(Photon*a, Photon*b)
{
	return a->distance2 < b->distance2;
}

Eigen::Vector3f PhotonMap::radiance_estimate(Eigen::Vector3f position, Eigen::Vector3f ray_dir, Eigen::Vector3f normal, int count, float radius)
{
	float r2 = radius * radius;
	float l2, weight;
	float max_dist2 = 0;
	Photon* p;
	std::vector<Photon*> nearest_photons;
	Eigen::Vector3f result = Eigen::Vector3f(0, 0, 0);
	Eigen::Vector3f difference;

	for (int i = 0; i < this->photons.size(); ++i) {
		p = this->photons[i];

		difference = p->position - position;
		l2 = difference.dot(difference);
		p->distance2 = l2;
		if (l2 < r2) {
			nearest_photons.push_back(p);
			if (l2 > max_dist2) max_dist2 = l2;
		}
	}

//	std::cout << (float)nearest_photons.size()/count << std::endl;
	std::sort(nearest_photons.begin(), nearest_photons.end(), _photon_sort);

#ifdef FILTER_CONE
	float max_dist = std::sqrt(max_dist2);
#endif

	for (int i = 0; i < nearest_photons.size() && i < count; ++i) {
		p = nearest_photons[i];

		difference = p->position - position;
		l2 = difference.dot(difference);

#ifdef FILTER_GAUSS
		weight = 1 - pow(E, -BETA * (l2/(2*max_dist2)));
		weight *= GAUSS_DENOM;
		weight = ALPHA * (1- weight);
#elif defined(FILTER_CONE)
		weight = 1 - difference.norm()/(K*max_dist);
#else
		weight = 1;
#endif
		float brdf = normal.dot(-p->direction);
		if (brdf > 1.0) brdf = 1.0;
		if (brdf < 0.0) brdf = 0.0;
		result += brdf * p->energy * weight;
	}

#ifdef FILTER_GUASS
	result = result;
#elif defined(FILTER_CONE)
	result = result/((1-(2/3*K))*PI*max_dist2);
#else
	result = result/(2*PI*max_dist2);
#endif

	return result;
}
