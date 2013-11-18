#include "data/data.h"
#include "data/scene.h"
#include "data/camera.h"

#include "rt_raytracer.h"
#include "rt_photon_map.h"
#include "rt_iaccel.h"

#include "rt_accel_array.h"

#include <stdio.h>
#include <map>
#include <vector>
#include <algorithm>
#include <limits>


const float EPSILON = 0.00001;

struct DACTri{
	Eigen::Vector3f min;
	Eigen::Vector3f max;
	unsigned int id;
	int pad;
};

std::vector<DACTri> dac_tris;
std::vector<Tri>* trilookup;

struct DACRay{
	Eigen::Vector3f origin;
	Eigen::Vector3f direction;
	float t;
	unsigned int id;
};
std::vector<DACRay> dac_rays;

struct DACRayResult{
	float u;
	float v;
	unsigned int tidx;
	Eigen::Vector3f origin;
	Eigen::Vector3f direction;
	float t;

};

std::vector<DACRayResult> dac_results;

RayTracer::RayTracer()
{
	this->bounces = 2;

	this->do_shadows = false;

	this->photon_map = NULL;
	this->photon_count = 10000;
	this->photon_estimate = 100;
	this->photon_radius = 1.0;
	trilookup = &this->tris;
}

RayTracer::~RayTracer()
{
	if (this->photon_map)
		delete this->photon_map;
}

void RayTracer::addMesh(Mesh *mesh)
{
	this->meshes.push_back(mesh);
}

#if 0
void RayTracer::shade(const Scene& scene, Ray *ray, Material* material, Eigen::Vector3f *color, int pass)
{

	if (pass < this->bounces) {
		float bias = 0.1;
		float lambert = 0.0;
		float shadow = 1;
		float specular = 0;

		Eigen::Vector3f V = ray->position;
		Eigen::Vector3f N = ray->normal;
		Eigen::Vector2f texcoord = ray->texcoord;
		Eigen::Vector3f I = ray->direction;

		float ref = material->reflectivity;
		float alpha = material->alpha;
		Eigen::Vector3f diff_color = material->diffuse_color;
		Eigen::Vector3f spec_color = material->specular_color;
		if (material->texture) {
			diff_color = material->texture->lookup(texcoord(0), texcoord(1));
		}



		if (!this->photon_map) {
			for (int i = 0; i < scene.lights.size(); ++i) {
				Eigen::Vector3f light_pos = scene.lights[i]->position;
				Eigen::Vector3f L = light_pos-V;
				Eigen::Vector3f H = (L + I).normalized();

				/* Diffuse */
				lambert += N.dot(L.normalized());
				lambert = std::min(std::max(lambert, 0.0f), 1.0f);

				/* Shadow */
				if (this->do_shadows) {
					Ray light_ray;
					light_ray.origin = V+N*bias;
					light_ray.direction = L;
					if (this->meshes->intersect(&light_ray, NULL)) {
						float distance = (ray->position - V).norm();
						if (distance < L.norm()) {
							shadow = std::max(shadow-0.4, 0.0);
						}
					}
				}

				/* Specular */
				float phong = H.dot(N);
				phong = std::max(phong, 0.0f);
				specular += pow(phong, material->shininess);
			}
		}

		/* Reflection */
		Eigen::Vector3f ref_color = Eigen::Vector3f(0, 0, 0);
		if (material->reflectivity > 0) {
			Eigen::Vector3f R = I - 2 * I.dot(N) * N;
			Ray ref_ray;
			ref_ray.origin = V;
			ref_ray.direction = R;
			if (this->meshes->intersect(&ref_ray, &material))
				shade(scene, &ref_ray, material, &ref_color, pass+1);
		}

		/* Refraction */
		Eigen::Vector3f refraction_color = Eigen::Vector3f(0, 0, 0);
		if (material->alpha > 0.1) {
			float IoR = material->ior;
			Eigen::Vector3f T = (I - N * I.dot(N))/IoR;
			T -= N * sqrt(1 - ((1 - I.dot(N)*I.dot(N)))/IoR*IoR);
			Ray refract_ray;
			refract_ray.origin = V;
			refract_ray.direction = T;

			if (this->meshes->intersect(&refract_ray, &material))
				shade(scene, &refract_ray, material, &refraction_color, pass+1);
		}

		Eigen::Vector3f light = Eigen::Vector3f(lambert, lambert, lambert);
		if (this->photon_map) {
			light = photon_map->radiance_estimate(ray->position, ray->direction, ray->normal, this->photon_estimate, this->photon_radius);
		}
		*color = shadow * light.array() * ((1.0-ref-alpha)*diff_color + ref*ref_color + alpha*refraction_color).array();
	}
}

void RayTracer::renderScene(const Scene& scene, int width, int height, unsigned char *color)
{

	if (this->photon_map) {
		this->photon_map->generate(&scene, this->meshes, this->photon_count);
	}

	Ray* screenRays = scene.camera->getScreenRays();
	Material* material;
	for (int i = 0; i < width*height; i++) {
		Eigen::Vector3f result;
		if (this->meshes->intersect(&screenRays[i], &material)) {
			shade(scene, &screenRays[i], material, &result, 0);
			color[4*i+0] = (unsigned char)(std::min((int)result(0), 255));
			color[4*i+1] = (unsigned char)(std::min((int)result(1), 255));
			color[4*i+2] = (unsigned char)(std::min((int)result(2), 255));
			color[4*i+3] = 255;
		}
		else {
			color[4*i+0] = 0;
			color[4*i+1] = 0;
			color[4*i+2] = 0;
			color[4*i+3] = 0;
		}
	}
}
#endif

static bool _triCmpX(DACTri a, DACTri b) {return (*trilookup)[a.id].centroid[0] < (*trilookup)[b.id].centroid[0];}
static bool _triCmpY(DACTri a, DACTri b) {return (*trilookup)[a.id].centroid[1] < (*trilookup)[b.id].centroid[1];}
static bool _triCmpZ(DACTri a, DACTri b) {return (*trilookup)[a.id].centroid[2] < (*trilookup)[b.id].centroid[2];}

static bool _ray_aabb_intersect(DACRay ray, Eigen::Vector3f min, Eigen::Vector3f max)
#if 0
{
	Eigen::Vector3f tmin = (min - ray.origin).array() / ray.direction.array();
	Eigen::Vector3f tmax = (max - ray.origin).array() / ray.direction.array();

	if (tmin[0] > tmax[0]) std::swap(tmin[0], tmax[0]);
	if (tmin[1] > tmax[1]) std::swap(tmin[1], tmax[1]);

	if ((tmin[0] > tmax[1]) || (tmin[1] > tmax[0]))
		return false;

	if (tmin[2] > tmax[2]) std::swap(tmin[2], tmax[2]);
	if ((tmin[0] > tmax[2]) || (tmin[2] > tmax[0]))
		return false;

	return true;
}
#else
{
	const char RIGHT = 0;
	const char LEFT = 1;
	const char MIDDLE = 2;

	bool inside = true;
	char quadrant[3];
	int i;
	int plane = 0;
	float maxt[3];
	float cand_plane[3];

	for (i = 0; i < 3; i++) {
		if (ray.origin[i] < min[i]) {
			quadrant[i] = LEFT;
			cand_plane[i] = min[i];
			inside = false;
		}
		else if (ray.origin[i] > max[i]) {
			quadrant[i] = RIGHT;
			cand_plane[i] = max[i];
			inside = false;
		}
		else {
			quadrant[i] = MIDDLE;
		}
	}

	if (inside) return true;

	for (i = 0; i < 3; i++) {
		if (quadrant[i] != MIDDLE && ray.direction[i] != 0.0)
			maxt[i] = (cand_plane[i] - ray.origin[i]) / ray.direction[i];
		else
			maxt[i] = -1.0;
	}

	for (i = 1; i < 3; i++) {
		if (maxt[plane] < maxt[i])
			plane = i;
	}

	if (maxt[plane] < 0.0) return false;

	for (i = 0; i < 3; i++) {
		if (plane != i) {
			float coord = ray.origin[i] + maxt[plane] * ray.direction[i];
			if (coord < min[i] || coord > max[i])
				return false;
		}
	}

	return true;
}
#endif

void RayTracer::trace(int rstart, int rend, int tstart, int tend, Eigen::Vector3f min_bound, Eigen::Vector3f max_bound)
{
	if (rend-rstart < 8 || tend-tstart < 8)
	{
		naiveTrace(rstart, rend, tstart, tend);
		return;
	}

	// Find largest axis to split
	Eigen::Vector3f dims = max_bound - min_bound;
	float max_dim = std::max(std::max(dims[0], dims[1]), dims[2]);
	bool (*cmp_func)(DACTri, DACTri);
	int split_axis;
	if (dims[0] == max_dim) {
		split_axis = 0;
		cmp_func = &_triCmpX;
	}
	else if (dims[1] == max_dim) {
		split_axis = 1;
		cmp_func = &_triCmpY;
	}
	else {
		split_axis = 2;
		cmp_func = &_triCmpZ;
	}

	// Split triangles along split plane
	int tmid = (tstart + tend) / 2;
	std::nth_element(dac_tris.begin()+tstart, dac_tris.begin()+tmid, dac_tris.begin()+tend, cmp_func);

	// Split space
	float inf = std::numeric_limits<float>::infinity();
	Eigen::Vector3f lmid_bound, umid_bound;
	min_bound = Eigen::Vector3f(inf, inf, inf);
	lmid_bound = Eigen::Vector3f(-inf, -inf, -inf);
	umid_bound = Eigen::Vector3f(inf, inf, inf);
	max_bound = Eigen::Vector3f(-inf, -inf, -inf);
	DACTri dtri;
	for (int t = tstart; t < tmid; t++) {
		dtri = dac_tris[t];
		min_bound[0] = std::min(min_bound[0], dtri.min[0]);
		min_bound[1] = std::min(min_bound[1], dtri.min[1]);
		min_bound[2] = std::min(min_bound[2], dtri.min[2]);

		lmid_bound[0] = std::max(lmid_bound[0], dtri.max[0]);
		lmid_bound[1] = std::max(lmid_bound[1], dtri.max[1]);
		lmid_bound[2] = std::max(lmid_bound[2], dtri.max[2]);
	}
	for (int t = tmid; t < tend; t++) {
		dtri = dac_tris[t];
		umid_bound[0] = std::min(umid_bound[0], dtri.min[0]);
		umid_bound[1] = std::min(umid_bound[1], dtri.min[1]);
		umid_bound[2] = std::min(umid_bound[2], dtri.min[2]);

		max_bound[0] = std::max(max_bound[0], dtri.max[0]);
		max_bound[1] = std::max(max_bound[1], dtri.max[1]);
		max_bound[2] = std::max(max_bound[2], dtri.max[2]);
	}

	// Filter Rays and continue tracing
	int pivot;
	Eigen::Vector3f min, max;
	Eigen::Vector3f bounds[2][2] = {{min_bound, lmid_bound}, {umid_bound, max_bound}};
	int tidx[] = {tstart, tmid, tend};
	for (int space = 0; space < 2; space++) {
		min = bounds[space][0];
		max = bounds[space][1];
		pivot = rend - 1;
		for (int i = rstart; i < rend; i++) {
			if (!_ray_aabb_intersect(dac_rays[i], min, max)) {
				//Swap to rmid
				std::swap(dac_rays[pivot], dac_rays[i]);
				pivot--;
			}
		}
		pivot = std::max(pivot, 0);
		trace(rstart, pivot, tidx[space], tidx[space+1], min, max);
//		naiveTrace(rstart, pivot, tidx[space], tidx[space+1]);
	}
}

void RayTracer::naiveTrace(int rstart, int rend, int tstart, int tend)
{
	Eigen::Vector3f E1, E2, T, P, Q, D, O;
	float det, inv_det, t, u, v;
	unsigned int min_index;
	Tri tri;
	Ray ray;

	for (unsigned int tidx = tstart; tidx < tend; tidx++) {
		tri = this->tris[dac_tris[tidx].id];

		for (unsigned int ridx = rstart; ridx < rend; ridx++) {
			D = dac_rays[ridx].direction;
			O = dac_rays[ridx].origin;

			tri = this->tris[dac_tris[tidx].id];
			E1 = tri.e1;
			E2 = tri.e2;
			P = D.cross(E2);

			det = P.dot(E1);
			if (det < EPSILON)
				continue;

			T = O - tri.v0;
			u = T.dot(P);

			if (u < 0 || u > det) continue;

			Q = T.cross(E1);
			v = D.dot(Q);

			if (v < 0 || u + v > det) continue;

			t = E2.dot(Q);
			inv_det = 1.0 / det;

			t *= inv_det;
			u *= inv_det;
			v *= inv_det;

			if (t < EPSILON) continue;

			if (t < dac_rays[ridx].t) {
				int result_id = dac_rays[ridx].id;
				dac_rays[ridx].t = t;
				dac_results[result_id].u = u;
				dac_results[result_id].v = v;
				dac_results[result_id].tidx = dac_tris[tidx].id;
				dac_results[result_id].origin = O;
				dac_results[result_id].direction = D;
				dac_results[result_id].t = t;
			}
		}
	}
}

void RayTracer::processRays(const Camera& camera, int count, Eigen::Vector3f *positions, Eigen::Vector3f *normals, Ray **results, ResultOffset **result_offsets, int *out_result_count, int *out_material_count)
{
	Eigen::Vector3f direction;

	Material *material = NULL;

	this->result_vec.clear();
	this->offset_vec.clear();
	this->result_map.clear();

	dac_rays.clear();
	dac_results.clear();

	DACRay ray;
	DACRayResult result;
	result.tidx = -1;
	for (int i = 0; i < count; i++) {
		direction = positions[i] - 2 * positions[i].dot(normals[i]) * normals[i];

		// Needs to be multiplied by the inverse view matrix!
		ray.direction = direction;
		ray.origin = positions[i];

		ray.t = std::numeric_limits<float>::infinity();
		ray.id = i;
		dac_rays.push_back(ray);
		dac_results.push_back(result);
	}

	this->tris.clear();
	dac_tris.clear();
	Mesh* mesh;
	Tri tri;
	DACTri dtri;
	Eigen::Vector3f min_bounds, max_bounds;
	min_bounds = max_bounds = this->meshes[0]->verts[0];
	for (unsigned int i = 0; i < this->meshes.size(); i++) {
		mesh = this->meshes[i];
		for (unsigned int j = 0; j < mesh->num_faces; j++) {
			tri.v0 = mesh->verts[mesh->faces[j].v[0]];
			tri.v1 = mesh->verts[mesh->faces[j].v[1]];
			tri.v2 = mesh->verts[mesh->faces[j].v[2]];
			tri.n0 = mesh->normals[mesh->faces[j].v[0]];
			tri.n1 = mesh->normals[mesh->faces[j].v[1]];
			tri.n2 = mesh->normals[mesh->faces[j].v[2]];
			tri.t0 = mesh->texcoords[mesh->faces[j].v[0]];
			tri.t1 = mesh->texcoords[mesh->faces[j].v[1]];
			tri.t2 = mesh->texcoords[mesh->faces[j].v[2]];
			tri.e1 = tri.v1 - tri.v0;
			tri.e2 = tri.v2 - tri.v0;
			tri.material = mesh->material;
			tri.centroid = (tri.v0 + tri.v1 + tri.v2) / 3.0;
			this->tris.push_back(tri);

			dtri.id = this->tris.size()-1;
			dtri.min[0] = std::min(std::min(tri.v0[0], tri.v1[0]), tri.v2[0]);
			dtri.min[1] = std::min(std::min(tri.v0[1], tri.v1[1]), tri.v2[1]);
			dtri.min[2] = std::min(std::min(tri.v0[2], tri.v1[2]), tri.v2[2]);

			dtri.max[0] = std::max(std::max(tri.v0[0], tri.v1[0]), tri.v2[0]);
			dtri.max[1] = std::max(std::max(tri.v0[1], tri.v1[1]), tri.v2[1]);
			dtri.max[2] = std::max(std::max(tri.v0[2], tri.v1[2]), tri.v2[2]);
			dac_tris.push_back(dtri);

			min_bounds[0] = std::min(min_bounds[0], dtri.min[0]);
			min_bounds[1] = std::min(min_bounds[1], dtri.min[1]);
			min_bounds[2] = std::min(min_bounds[2], dtri.min[2]);

			max_bounds[0] = std::max(max_bounds[0], dtri.max[0]);
			max_bounds[1] = std::max(max_bounds[1], dtri.max[1]);
			max_bounds[2] = std::max(max_bounds[2], dtri.max[2]);
		}
	}

	this->trace(0, count, 0, this->tris.size(), min_bounds, max_bounds);
//	this->naiveTrace(0, count, 0, this->tris.size());

	float u, v;
	int min_index;
	Ray result_ray;
	for (int i = 0; i < count; i++) {
		if (dac_results[i].tidx == -1) continue;
		u = dac_results[i].u;
		v = dac_results[i].v;
		min_index = dac_results[i].tidx;
		result_ray.origin = dac_results[i].origin;
		result_ray.direction = dac_results[i].direction;
		result_ray.position = (1 - u -v)*this->tris[min_index].v0 + u*this->tris[min_index].v1 + v*this->tris[min_index].v2;
		result_ray.normal = (1 - u -v)*this->tris[min_index].n0 + u*this->tris[min_index].n1 + v*this->tris[min_index].n2;
		result_ray.normal.normalize();
		result_ray.texcoord = (1 - u -v)*this->tris[min_index].t0 + u*this->tris[min_index].t1 + v*this->tris[min_index].t2;
		this->result_map[this->tris[min_index].material].push_back(result_ray);
	}

	int offset = 0;
	for (ResultMap::iterator iter = result_map.begin(); iter != result_map.end(); iter++) {
		material = iter->first;
		std::vector<Ray> result_rays = iter->second;
		offset += result_rays.size();
		this->result_vec.insert(result_vec.end(), result_rays.begin(), result_rays.end());
		this->offset_vec.push_back(std::pair<Material*, int>(material, offset));
	}

	*out_result_count = this->result_vec.size();
	*out_material_count = this->offset_vec.size();
	*results = &this->result_vec[0];
	*result_offsets = &this->offset_vec[0];
}
