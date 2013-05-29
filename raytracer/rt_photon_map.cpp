#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <iostream>

#include "rt_photon_map.h"
#include "rt_ray.h"
#include "rt_iaccel.h"
#include "data/scene.h"
#include "Eigen/Dense"

//#define ALPHA 0.918
//#define BETA 1.953
//#define PI 3.141592653589793
//#define E 2.718281828459045
//#define K 5

const int X = 0;
const int Y = 1;
const int Z = 2;

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
	short flag;
};

class KDNode
{
public:
	Photon* data;
	class KDNode* left, *right;
};

void PhotonMap::emit_photon(Scene* scene, Ray* r, Eigen::Vector3f energy, float max_dist, int pass)
{
	Result result;

//	if (pass > 1) return;

	scene->mesh_structure->intersect(r, &result);
	if (!result.hit) return;

	float dist = 0;
//	float dist = (result.position - *(r->getOrigin())).norm();
//	if (dist > max_dist) return;
//	energy *=  (max_dist - dist) / max_dist;

//	if (((float)rand())/RAND_MAX < 0.8) {
		Photon* p = new Photon();
		p->position = result.position;
		p->energy = energy;
		p->direction = *r->getDirection();
		this->photons.push_back(p);
//	}
//	else {
		r->setOrigin(result.position(0), result.position(1), result.position(2));
		Eigen::Vector3f dir;
		dir(0) = rand() / rmax_2 - 1;
		dir(1) = rand() / rmax_2 - 1;
		dir(2) = rand() / rmax_2 - 1;
		if (dir.dot(dir) > 1) return;
		r->setDirection(dir(0), dir(1), dir(2));
		energy = result.material->diffuse_color * energy.norm();
		energy *= 0.0075;
//		this->emit_photon(scene, r, energy, max_dist-dist, pass++);
//	}

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

KDNode* PhotonMap::balance_photons(int begin, int end, int level)
{
	KDNode* node = new KDNode;
	if (end - begin == 1) {
		node->data = this->photons[begin];
		return node;
	}

	int axis = level % 3;

	if (axis == X)
		std::sort(this->photons.begin()+begin, this->photons.begin()+end, sort_x);
	else if (axis == Y)
		std::sort(this->photons.begin()+begin, this->photons.begin()+end, sort_y);
	else if (axis == Z)
		std::sort(this->photons.begin()+begin, this->photons.begin()+end, sort_z);

	Photon* median = this->photons[this->photons.size()/2];
	median->flag = axis;

	int mid = (begin+end)/2;
	this->balance_photons(begin, mid, level++);
	this->balance_photons(mid, end, level++);

	return NULL;
}

void PhotonMap::generate(Scene* scene, int count)
{
	Eigen::Vector3f dir;
	Eigen::Vector3f energy;
	Ray ray;
	Light* light;
	float max_dist = 20;
	float rmax_2 = RAND_MAX/2.0;
	srand(time(NULL));
	for (int i = 0; i < scene->lights.size(); ++i) {
		light = scene->lights[i];
		energy = light->color;
		for (int j = 0; j < count;) {
			dir(0) = rand() / rmax_2 - 1;
			dir(1) = rand() / rmax_2 - 1;
			dir(2) = rand() / rmax_2 - 1;
			
			if (dir.dot(dir) > 1) continue;
			j++;
			

			ray.setOrigin(light->position(0), light->position(1), light->position(2));
			ray.setDirection(dir(0), dir(1), dir(2));

			this->emit_photon(scene, &ray, energy, max_dist, 0);
			continue;
		}
	}

	float scale = 1.0 / this->photons.size();
	for (int i = 0; i < this->photons.size(); ++i)
	{
		this->photons[i]->energy *= scale;
	}

	this->balance_photons(0, this->photons.size(), 0);
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

Eigen::Vector3f PhotonMap::radiance_estimate(Eigen::Vector3f position, Eigen::Vector3f ray_dir, Eigen::Vector3f normal, float radius)
{
	float r2 = radius * radius;
	float l2, weight;
	float max_dist2 = 0;
	Photon* p;
	std::vector<Photon*> nearest_photons;
	Eigen::Vector3f result = Eigen::Vector3f(0, 0, 0);
	Eigen::Vector3f difference;
	return result;
	for (int i = 0; i < this->photons.size(); ++i) {
		p = this->photons[i];

		difference = p->position - position;
		l2 = difference.dot(difference);
		if (l2 < r2) {
			nearest_photons.push_back(p);
			if (l2 > max_dist2) max_dist2 = l2;
		}
	}

//	float max_dist = sqrt(max_dist2);
	for (int i = 0; i < nearest_photons.size(); ++i) {
		p = nearest_photons[i];

		difference = p->position - position;
		l2 = difference.dot(difference);

		weight = 1 - pow(E, -BETA * (l2/(2*max_dist2)));
		weight *= GAUSS_DENOM;
		weight = ALPHA * (1- weight);

//		difference = p->position - position;
//		weight = 1 - difference.norm()/(K*max_dist);
		float lambert = normal.dot(-p->direction);
		if (lambert < 0.1) lambert = 0.1;
		if (lambert > 0.9) lambert = 0.9;

		result += p->energy * weight;
	}

//	result = result/((1-(2/3*K))*PI*max_dist2);
//	result = result/(2*PI*max_dist2);

	return result;
}
