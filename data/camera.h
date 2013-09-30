#ifndef __DATA_CAMERA__
#define __DATA_CAMERA__

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
	void setFOV(float fov);
	void setWidth(float width);
	void setHeight(float height);
	float getFOV();
	int getWidth();
	int getHeight();
};

#endif
