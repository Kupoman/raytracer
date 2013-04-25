#ifndef __IACCEL_H__
#define __IACCEL_H__

#include "data/data.h"

class IAccel
{
public:
	virtual void addMesh(Mesh* mesh)=0;
	virtual void update()=0;
	virtual bool occlude(class Ray) = 0;
	virtual void intersect(class Ray*, Result*) = 0;
};

#endif
