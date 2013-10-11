#include "rt_ray.h"
#include "rt_iaccel.h"

Ray::Ray(Eigen::Vector3f origin, Eigen::Vector3f direction)
{
	this->origin = new Eigen::Vector3f(origin);
	this->direction = new Eigen::Vector3f(direction);
	this->direction->normalize();
}

Ray::Ray()
{
	origin = new Eigen::Vector3f(0, 0, 0);
	direction = new Eigen::Vector3f(0, 0, 0);
}

Ray::~Ray()
{
	delete this->origin;
	delete this->direction;
}

void Ray::setOrigin(float x, float y, float z)
{
	(*this->origin)(0) = x;
	(*this->origin)(1) = y;
	(*this->origin)(2) = z;
}

void Ray::setOrigin(Eigen::Vector3f vec)
{
	*this->origin = vec;
}

void Ray::setDirection(float x, float y, float z)
{
	(*this->direction)(0) = x;
	(*this->direction)(1) = y;
	(*this->direction)(2) = z;
	
	this->direction->normalize();
}

void Ray::setDirection(Eigen::Vector3f vec)
{
	*this->direction = vec;
}
