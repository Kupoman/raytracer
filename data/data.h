#ifndef __DATA_H__
#define __DATA_H__

#include "Eigen/Dense"
#include <vector>

class Texture {
private:
	unsigned char* pixels;
	unsigned int width, height;
	struct FIBITMAP* bitmap;
public:
	Texture(const char* filename = NULL);
	~Texture();
	Eigen::Vector3f lookup(float u, float v);
};

typedef struct {
	Eigen::Vector3f color;
	float reflectivity;
	float alpha;
	float ior;
	Texture *texture;
} Material;

typedef struct {
	bool hit;
	Eigen::Vector3f position;
	Eigen::Vector3f normal;
	Eigen::Vector2f texcoord;
	Material *material;
} Result;

typedef struct {
	Eigen::Vector3f position;
	Eigen::Vector3f color;
	float energy;
} Light;

typedef struct {
	unsigned int v[3];
} Face;

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
