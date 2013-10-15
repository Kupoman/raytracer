#ifndef __DATA_H__
#define __DATA_H__

#include "Eigen/Dense"
#include <vector>

class Texture {
private:
	struct FIBITMAP* bitmap;
public:
	unsigned char* pixels;
	unsigned int width, height;
	Texture(const char* filename = NULL);
	~Texture();
	Eigen::Vector3f lookup(float u, float v);
};

struct Material{
	Eigen::Vector3f diffuse_color;
	Eigen::Vector3f specular_color;
	float shininess;
	float reflectivity;
	float alpha;
	float ior;
	Texture *texture;
};

typedef std::pair<struct Material*, int> ResultOffset;
//struct Result{
//	Eigen::Vector3f position;
//	Eigen::Vector3f normal;
//	Eigen::Vector2f texcoord;
//};
struct Ray
{
	Eigen::Vector3f origin;
	Eigen::Vector3f direction;
	Eigen::Vector3f position;
	Eigen::Vector3f normal;
	Eigen::Vector2f texcoord;
	float pad[2];
};

struct Light{
	Eigen::Vector3f position;
	Eigen::Vector3f color;
	float energy;
};

struct Face{
	unsigned int v[3];
};

class Mesh {
public:
	Eigen::Vector3f position;
	float radius;
	unsigned int num_verts;
	Eigen::Vector3f* verts;
	Eigen::Vector3f* normals;
	Eigen::Vector2f* texcoords;
	unsigned int num_faces;
	Face* faces;
	Material *material;

	Mesh();
	~Mesh();
};

#endif
