#include <vector>

#include "Eigen/Dense"

#ifndef __RT_PHOTON_MAP__
#define __RT_PHOTON_MAP__

class PhotonMap
{
private:
	void emit_photon(class IAccel* meshes, class Ray* r, Eigen::Vector3f energy, float max_dist, int pass);
	std::vector<class Photon*> photons;
public:
	void generate(const class Scene *scene, class IAccel* meshes, int count);
	Eigen::Vector3f lookup(Eigen::Vector3f position, float radius);
	Eigen::Vector3f radiance_estimate(Eigen::Vector3f position, Eigen::Vector3f ray_dir, Eigen::Vector3f normal, int count, float radius);
};

#endif
