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

class AccelSpheres : public IAccel
{
	class std::vector<Sphere*> spheres;
public:
	AccelSpheres();
	void addSphere(float posX, float posY, float posZ, float radius);
	virtual void addMesh(Mesh* mesh);
	virtual void update();
	virtual bool occlude(class Ray* ray);
	virtual bool intersect(class Ray* ray, Material **material);
};

#endif
