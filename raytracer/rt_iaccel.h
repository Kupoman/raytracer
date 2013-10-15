#ifndef __IACCEL_H__
#define __IACCEL_H__

class IAccel
{
public:
	virtual void addMesh(class Mesh* mesh)=0;
	virtual void update()=0;
	virtual bool occlude(class Ray*) = 0;
	virtual bool intersect(class Ray*, struct Material**) = 0;
};

#endif
