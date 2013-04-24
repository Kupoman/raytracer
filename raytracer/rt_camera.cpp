#include "rt_camera.h" 
#include "rt_ray.h"

Camera::Camera(float fovx, float fovy, int width, int height) :
fovx(fovx),
fovy(fovy),
width(width),
height(height)
{
	rays = new Ray[width*height];
}

Camera::~Camera()
{
	delete [] rays;
}

Ray* Camera::getScreenRays()
{
	int i;
	float dirX, dirY;
	float x_step = 2.0/this->width;
	float y_step = 2.0/this->height;
	
	for (int y = 0; y < this->height; y++) {
		for (int x = 0; x < this->width; x++) {
			i = this->width*y + x;
			rays[i].setOrigin(0, 0, 0);

			dirX = -1+x_step*0.5 + x * x_step;
			dirY = -1+y_step*0.5 + y * y_step;
			rays[i].setDirection(dirX, dirY, -1);
		}
	}
	return rays;
}