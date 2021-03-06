#include "camera.h"
#include "data/data.h"

#include <math.h>

Camera::Camera(float fov, int width, int height) :
width(width),
height(height)
{
	this->setFOV(fov);
	this->rays = new Ray[width*height];
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
			rays[i].origin = Eigen::Vector3f(0, 0, 0);

			dirX = 2 * ((x+0.5) / this->width) - 1;
			dirY = 2 * ((y+0.5) / this->height) - 1;
			dirX *= aspect;
			rays[i].direction = Eigen::Vector3f(dirX*this->fov, dirY*this->fov, -1);
		}
	}
	return rays;
}

void Camera::setFOV(float fov)
{
	this->fov = tan(fov);
}

void Camera::setWidth(float width)
{
	this->width = width;
	delete [] rays;
	this->rays = new Ray[this->width*this->height];
}

void Camera::setHeight(float height)
{
	this->height = height;
	delete [] rays;
	this->rays = new Ray[this->width*this->height];
}

float Camera::getFOV()
{
	return this->fov;
}

int Camera::getWidth()
{
	return this->width;
}

int Camera::getHeight()
{
	return this->height;
}
