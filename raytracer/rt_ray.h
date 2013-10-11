#ifndef __RT_RAY__
#define __RT_RAY__

#include "Eigen/Dense"

class Ray
{
private:
	Eigen::Vector3f* origin;
	Eigen::Vector3f* direction;
public:
	Ray();
	~Ray();
	Ray(Eigen::Vector3f origin, Eigen::Vector3f direction);
	
	void setOrigin(float x, float y, float z);
	void setOrigin(Eigen::Vector3f vec);
	void setDirection(float x, float y, float z);
	void setDirection(Eigen::Vector3f vec);
	
	Eigen::Vector3f* getOrigin() {return origin;};
	Eigen::Vector3f* getDirection() {return direction;};
};

#endif
