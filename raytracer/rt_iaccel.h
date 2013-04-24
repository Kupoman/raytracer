#include "data/data.h"

class IAccel
{
public:
	virtual bool occlude(class Ray) = 0;
	virtual void intersect(class Ray*, Result*) = 0;
};