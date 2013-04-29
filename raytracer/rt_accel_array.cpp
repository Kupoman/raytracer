#include "rt_accel_array.h"
#include "rt_ray.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

AccelArray::AccelArray()
{
	srand(420);
}

void AccelArray::addMesh(Mesh* mesh)
{
	for (int i = 0; i < mesh->num_faces; ++i) {
		this->v0.push_back(mesh->verts[mesh->faces[i].v[0]]);
		this->v1.push_back(mesh->verts[mesh->faces[i].v[1]]);
		this->v2.push_back(mesh->verts[mesh->faces[i].v[2]]);
		this->n0.push_back(mesh->normals[mesh->faces[i].v[0]]);
		this->n1.push_back(mesh->normals[mesh->faces[i].v[1]]);
		this->n2.push_back(mesh->normals[mesh->faces[i].v[2]]);
		this->e1.push_back(this->v1[this->v1.size()-1] - this->v0[this->v0.size()-1]);
		this->e2.push_back(this->v2[this->v2.size()-1] - this->v0[this->v0.size()-1]);
		this->material.push_back(this->materials.size());
	}

	Material material;
	material.color = Eigen::Vector3f(rand()%256, rand()%256, rand()%256);
//	material.color = Eigen::Vector3f(255, 0, 0);
	this->materials.push_back(material);
}

void AccelArray::update()
{

}

bool AccelArray::occlude(Ray* ray)
{
	Eigen::Vector3f E1, E2, T, P, Q, tuv;
	float det;

	Eigen::Vector3f D = *ray->getDirection();
	Eigen::Vector3f O = *ray->getOrigin();

	for (int i = 0; i < this->v0.size(); ++i) {

		E1 = this->e1[i];
		E2 = this->e2[i];
		P = D.cross(E2);

		det = P.dot(E1);
		if (det < 0.000001)
			continue;

		T = O - this->v0[i];

		Q = T.cross(E1);

		tuv = Eigen::Vector3f(Q.dot(E2), P.dot(T), Q.dot(D)) / det;

		if (tuv(1) >= 0 && tuv(2) >= 0 && tuv(1) + tuv(2) <= 1 && 1 >= tuv(1))
			return true;
	}
	return false;
}

void AccelArray::intersect(Ray* ray, Result *result)
{

	Eigen::Vector3f E1, E2, T, P, Q, tuv;
	float det, inv_det, t, u, v;

	Eigen::Vector3f D = *ray->getDirection();
	Eigen::Vector3f O = *ray->getOrigin();

	float min_t = 1000000, min_u, min_v;
	int min_index = -1;
	result->hit = false;

	for (int i = 0; i < this->v0.size(); ++i) {

		E1 = this->e1[i];
		E2 = this->e2[i];
		P = D.cross(E2);

		det = P.dot(E1);
		if (det < 0.000001)
			continue;

		T = O - this->v0[i];
		u = T.dot(P);

		if (u < 0 || u > det) continue;

		Q = T.cross(E1);
		v = D.dot(Q);

		if (v < 0 || u + v > det) continue;

		t = E2.dot(Q);
		inv_det = 1.0 / det;

		t *= inv_det;
		u *= inv_det;
		v *= inv_det;

		if (t < min_t) {
			min_t = t;
			min_u = u;
			min_v = v;
			min_index = i;
		}
	}

	if (min_index != -1) {
		u = min_u;
		v = min_v;
		result->position = (1 - u -v)*this->v0[min_index] + u*this->v1[min_index] + v*this->v2[min_index];
		result->normal = (1 - u -v)*this->n0[min_index] + u*this->n1[min_index] + v*this->n2[min_index];
		result->normal.normalize();
		result->hit = true;
		result->material = &this->materials[this->material[min_index]];
	}
}
