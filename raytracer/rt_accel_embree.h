#ifndef __RT_ACCEL_EMBREE__
#define __RT_ACCEL_EMBREE__

#include "rt_iaccel.h"

class AccelEmbree : public IAccel
{
private:
    struct EData* data;
public:
	AccelEmbree();
	~AccelEmbree();
	virtual void addMesh(struct Mesh* mesh);
	virtual void update();
	virtual bool occlude(class Ray* ray);
	virtual void intersect(class Ray* ray, Result* result);
};

#endif
