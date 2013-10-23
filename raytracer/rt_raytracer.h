#ifndef __RT_RAYTRACER___
#define __RT_RAYTRACER__

#include "Eigen/Dense"
#include "data/data.h"

#include <vector>
#include <map>


class RayTracer
{
private:
	int bounces;

	bool do_shadows;

	std::vector<Mesh*> meshes;

	// Photon Mapping
	class PhotonMap* photon_map;
	int photon_count;
	int photon_estimate;
	float photon_radius;

	std::vector<Ray> result_vec;
	std::vector<ResultOffset> offset_vec;

	std::vector<Ray> rays;
	struct Tri {
		Eigen::Vector3f v0;
		Eigen::Vector3f v1;
		Eigen::Vector3f v2;
		Eigen::Vector3f n0;
		Eigen::Vector3f n1;
		Eigen::Vector3f n2;
		Eigen::Vector2f t0;
		Eigen::Vector2f t1;
		Eigen::Vector2f t2;
		Eigen::Vector3f e1;
		Eigen::Vector3f e2;
		Material* material;
	};

	typedef std::map<Material*, std::vector<Ray> > ResultMap;
	ResultMap result_map;
	std::vector<Tri> tris;
	void shade(const class Scene& scene, struct Ray *ray, struct Material* material, Eigen::Vector3f *color, int pass);
	void trace(int rstart, int rend, int tstart, int tend);

public:
	RayTracer();
	~RayTracer();

	void addMesh(struct Mesh* mesh);

	void renderScene(const class Scene& scene, int width, int height, unsigned char *color);
	void processRays(const class Camera& camera, int count, Eigen::Vector3f* positions, Eigen::Vector3f* normals, Ray **results, ResultOffset **result_offsets, int *out_result_count, int *out_material_count);
};

#endif
