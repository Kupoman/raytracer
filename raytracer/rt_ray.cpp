#include "rt_ray.h"
#include "rt_iaccel.h"

Ray::Ray(Eigen::Vector3f origin, Eigen::Vector3f direction)
{
	this->origin = origin;
	this->direction = direction;
	this->direction.normalize();
}

Ray::Ray()
{
	origin = Eigen::Vector3f(0, 0, 0);
	direction = Eigen::Vector3f(0, 0, 0);
}

Ray::~Ray()
{
}

void Ray::setOrigin(float x, float y, float z)
{
	this->origin(0) = x;
	this->origin(1) = y;
	this->origin(2) = z;
}

void Ray::setDirection(float x, float y, float z)
{
	this->direction(0) = x;
	this->direction(1) = y;
	this->direction(2) = z;
	
	this->direction.normalize();
}
