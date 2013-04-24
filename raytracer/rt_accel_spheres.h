#ifndef __RT_ACCEL_SPHERES__
#define __RT_ACCEL_SHPERES__

#include <vector>
#include "Eigen/Dense"
#include "rt_iaccel.h"

typedef struct {
	Eigen::Vector3f position;
	float radius;
	Material *material;
} Sphere;

class AccelSpheres : IAccel
{
	class std::vector<Sphere*> spheres;
	Material material;
public:
	AccelSpheres();
	void addSphere(float posX, float posY, float posZ, float radius);
	virtual bool occlude(class Ray ray);
	virtual void intersect(class Ray* ray, Result* result);
};

#endif