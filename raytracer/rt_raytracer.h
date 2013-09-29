#ifndef __RT_RAYTRACER___
#define __RT_RAYTRACER__

#include "Eigen/Dense"
#include "data/data.h"

class RayTracer
{
private:
	int bounces;

	bool do_shadows;

	class IAccel* meshes;

	// Photon Mapping
	class PhotonMap* photon_map;
	int photon_count;
	int photon_estimate;
	float photon_radius;

	void shade(const class Scene& scene, struct Ray *ray, Result* result, Eigen::Vector3f *color, int pass);

public:
	RayTracer();
	~RayTracer();

	void addMesh(Mesh* mesh);

	void renderScene(const class Scene& scene, int width, int height, unsigned char *color);
};

#endif
