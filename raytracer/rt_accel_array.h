#ifndef __RT_ACCEL_ARRAY__
#define __RT_ACCEL_ARRAY__

#include <vector>
#include "Eigen/Dense"
#include "rt_iaccel.h"

class AccelArray : public IAccel
{
	std::vector<Eigen::Vector3f> v0;
	std::vector<Eigen::Vector3f> v1;
	std::vector<Eigen::Vector3f> v2;
	std::vector<Eigen::Vector3f> n0;
	std::vector<Eigen::Vector3f> n1;
	std::vector<Eigen::Vector3f> n2;
	std::vector<Eigen::Vector2f> t0;
	std::vector<Eigen::Vector2f> t1;
	std::vector<Eigen::Vector2f> t2;
	std::vector<Eigen::Vector3f> e1;
	std::vector<Eigen::Vector3f> e2;
	std::vector<unsigned int> material;

	std::vector<struct Material*> materials;
public:
	AccelArray();
	virtual void addMesh(Mesh* mesh);
	virtual void update();
	virtual bool occlude(class Ray* ray);
	virtual bool intersect(class Ray* ray, Material **material);
};

#endif
