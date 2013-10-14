#ifndef __RT_RAYTRACER___
#define __RT_RAYTRACER__

#include "Eigen/Dense"
#include "data/data.h"

#include <vector>


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


	std::vector<struct Result> result_vec;
	std::vector<ResultOffset> offset_vec;

	void shade(const class Scene& scene, struct Ray *ray, struct Result* result, struct Material* material, Eigen::Vector3f *color, int pass);

public:
	RayTracer();
	~RayTracer();

	void addMesh(struct Mesh* mesh);

	void renderScene(const class Scene& scene, int width, int height, unsigned char *color);
	void processRays(const class Camera& camera, int count, Eigen::Vector3f* positions, Eigen::Vector3f* normals, Result **results, ResultOffset **result_offsets, int *out_count);
};

#endif
