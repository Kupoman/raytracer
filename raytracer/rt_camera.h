#ifndef __RT_CAMERA__
#define __RT_CAMERA__

#include "Eigen/Dense"

class Camera
{
private:
	float fovx, fovy;
	int width, height;
	class Ray* rays;
public:
	Camera(float fovx, float fovy, int width, int height);
	~Camera();
	class Ray* getScreenRays();
};

#endif
