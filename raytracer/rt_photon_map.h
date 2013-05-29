#include <vector>

#include "Eigen/Dense"

#ifndef __RT_PHOTON_MAP__
#define __RT_PHOTON_MAP__

class PhotonMap
{
private:
	void emit_photon(class Scene* scene, class Ray* r, Eigen::Vector3f energy, float max_dist, int pass);
	class KDNode* balance_photons(int begin, int end, int level);
	std::vector<class Photon*> photons;
	class KDNode* kdtree;
public:
	void generate(class Scene* scene, int count);
	Eigen::Vector3f lookup(Eigen::Vector3f position, float radius);
	Eigen::Vector3f radiance_estimate(Eigen::Vector3f position, Eigen::Vector3f ray_dir, Eigen::Vector3f normal, float radius);
};

#endif
