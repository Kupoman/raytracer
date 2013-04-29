#ifndef __DATA_H__
#define __DATA_H__

#include "Eigen/Dense"
#include <vector>

typedef struct {
	Eigen::Vector3f color;
} Material;

typedef struct {
	bool hit;
	Eigen::Vector3f position;
	Eigen::Vector3f normal;
	Material *material;
} Result;

typedef struct {
	Eigen::Vector3f position;
	Eigen::Vector3f color;
	float energy;
} Light;

typedef struct {
	unsigned int v[3];
} Face;

class Mesh {
public:
	Eigen::Vector3f position;
	float radius;
	unsigned int num_verts;
	Eigen::Vector3f* verts;
	Eigen::Vector3f* normals;
	unsigned int num_faces;
	Face* faces;
	Material *material;

	Mesh();
	~Mesh();
};

#endif
