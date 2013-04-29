#include "rt_accel_array.h"
#include "rt_ray.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

AccelArray::AccelArray()
{
}

void AccelArray::addMesh(Mesh* mesh)
{
	for (int i = 0; i < mesh->num_faces; ++i) {
		this->v0.push_back(mesh->verts[mesh->faces[i].v[0]]);
		this->v1.push_back(mesh->verts[mesh->faces[i].v[1]]);
		this->v2.push_back(mesh->verts[mesh->faces[i].v[2]]);
		this->e1.push_back(this->v1[i] - this->v0[i]);
		this->e2.push_back(this->v2[i] - this->v0[i]);
	}
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

}
