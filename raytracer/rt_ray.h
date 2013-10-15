#ifndef __RT_RAY__
#define __RT_RAY__

#include "Eigen/Dense"

class Ray
{
public:
	Eigen::Vector3f origin;
	Eigen::Vector3f direction;
	Eigen::Vector3f position;
	Eigen::Vector3f normal;
	Eigen::Vector2f texcoord;

	Ray();
	~Ray();
	Ray(Eigen::Vector3f origin, Eigen::Vector3f direction);
	
	void setOrigin(float x, float y, float z);
	void setDirection(float x, float y, float z);
private:
	float pad[2];
};

#endif
