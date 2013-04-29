#ifndef __RT_ACCEL_ARRAY__
#define __RT_ACCEL_ARRAY__

#include <vector>
#include "Eigen/Dense"
#include "rt_iaccel.h"

typedef struct {
	Eigen::Vector3f position;
	float radius;
	Material *material;
} Sphere;

class AccelArray : public IAccel
{
	std::vector<Eigen::Vector3f> v0;
	std::vector<Eigen::Vector3f> v1;
	std::vector<Eigen::Vector3f> v2;
	std::vector<Eigen::Vector3f> e1;
	std::vector<Eigen::Vector3f> e2;
public:
	AccelArray();
	virtual void addMesh(Mesh* mesh);
	virtual void update();
	virtual bool occlude(class Ray* ray);
	virtual void intersect(class Ray* ray, Result* result);
};

#endif
