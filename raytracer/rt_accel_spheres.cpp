#include "rt_accel_spheres.h"
#include "rt_ray.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

AccelSpheres::AccelSpheres()
{
	srand(420);//time(NULL));
}

void AccelSpheres::addSphere(float posX, float posY, float posZ, float radius)
{
// 	Sphere s = {Eigen::Vector3f(posX, posY, posZ), radius};
// 	this->spheres.push_back(s);
	
	Sphere* s = new Sphere;
	s->position = Eigen::Vector3f(posX, posY, posZ);
	s->radius = radius;
	s->material = new Material;
	//s->material->color = Eigen::Vector3f(255, 0, 0);
	s->material->diffuse_color = Eigen::Vector3f(rand()%256, rand()%256, rand()%256);
	this->spheres.push_back(s);
}
void AccelSpheres::addMesh(Mesh* mesh)
{
	Eigen::Vector3f pos = mesh->position;
	this->addSphere(pos(0), pos(1), pos(2), 1);
}

void AccelSpheres::update()
{

}

bool AccelSpheres::occlude(Ray* ray)
{
	Sphere *s;
	Eigen::Vector3f *Rd, *Ro;
	float a, b, c;
	for (int i = 0; i < this->spheres.size(); i++) {
		s = this->spheres[i];
		Rd = ray->getDirection();
		Ro = ray->getOrigin();
		
		a = 1;
		b = (2 * *Rd).dot(*Ro - s->position);
		c = (*Ro - s->position).dot(*Ro-s->position) - s->radius*s->radius;
		//return (b*b - 4*a*c) < 0;
		if ((b*b - 4*a*c) > 0)
			return true;
	}
	return false;
}

void AccelSpheres::intersect(Ray* ray, Result *result)
{
	Sphere* s;
	float min_distance = 1000000;
	Eigen::Vector3f *Rd, *Ro, X;
	float a, b, c, disc, t0, t1, distance;
	
	result->hit = false;
	

	for (unsigned int i = 0; i < this->spheres.size(); i++) {
		s = this->spheres[i];
		Rd = ray->getDirection();
		Ro = ray->getOrigin();
		
		a = 1;
		b = (2 * *Rd).dot(*Ro - s->position);
		c = (*Ro - s->position).dot(*Ro-s->position) - s->radius*s->radius;
		
		disc = (b*b - 4*a*c);
		if (disc < 0) continue;
		
		disc = sqrt(disc);
		
		t0 = (-b - disc) * 0.5;
		t1 = (-b + disc) * 0.5;
		
		/* Don't render if inside a sphere */
		if (t0 < 0 && t1 > 0) continue;
		
		t0 = std::min(t0, t1);
		
		if (t0 <= 0.0) continue;
		
		X = (*Ro) + (*Rd)*t0;
		distance = X.squaredNorm();
		if (distance < min_distance) {
			result->position = X;
			result->normal = (X - s->position) / s->radius;
			//result->normal.normalize();
			//result->position = s->position + result->normal * s->radius;
			min_distance = distance;
			result->hit = true;
			result->material = s->material;
		}	
	}
}
