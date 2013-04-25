#ifndef __RT_CAMERA__
#define __RT_CAMERA__

#include "Eigen/Dense"

class Camera
{
private:
	float fov;
	int width, height;
	class Ray* rays;
public:
	Camera(float fov, int width, int height);
	~Camera();
	class Ray* getScreenRays();
};

#endif
