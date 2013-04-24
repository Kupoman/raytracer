#ifndef RT_RESULT__
#define RT_RESULT__

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
	Eigen::Vector3f position;
	Material *material;
} Mesh;

typedef struct {
	std::vector<Light> lights;
} Scene;

#endif