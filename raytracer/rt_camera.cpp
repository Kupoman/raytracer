#include "rt_camera.h" 
#include "rt_ray.h"

#include <math.h>

Camera::Camera(float fov, int width, int height) :
width(width),
height(height)
{
	this->fov = tan(fov);
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
	float aspect = float(this->width)/this->height;

	for (int y = 0; y < this->height; y++) {
		for (int x = 0; x < this->width; x++) {
			i = this->width*y + x;
			rays[i].setOrigin(0, 0, 0);

			dirX = 2 * ((x+0.5) / this->width) - 1;
			dirY = 1 - 2 * ((y+0.5) / this->height);
			dirX *= aspect;
			rays[i].setDirection(dirX*this->fov, dirY*this->fov, -1);
		}
	}
	return rays;
}
