#include "rt_accel_embree.h"
#include "rt_ray.h"

#include "data/data.h"
#include "Eigen/Dense"

#include "include/embree.h"

#include <vector>

struct EData {
    std::vector<embree::RTCVertex> verts;
    std::vector<embree::RTCTriangle> triangles;
    embree::RTCGeometry *scene;
    embree::RTCIntersector1 *intersector;
};

struct ERay {
    float org[4];
    float dir[4];
    float tnear;
    float tfar;
    float u;
    float v;
    int id0;
    int id1;
    float Ng[4];
    int mask;
    float time;
};

AccelEmbree::AccelEmbree()
{
    embree::rtcInit();
    this->data = new EData();
    this->data->scene = NULL;
    this->data->intersector = NULL;
}

AccelEmbree::~AccelEmbree()
{
    if (this->data->scene)
        embree::rtcDeleteGeometry(this->data->scene);

    if (this->data->intersector)
        embree::rtcDeleteIntersector1(this->data->intersector);

    if (this->data)
        delete this->data;
    embree::rtcExit();
}

void AccelEmbree::addMesh(Mesh* mesh)
{
	Eigen::Vector3f eigen_vert;
	int face[3];
	for (int i = 0; i < mesh->num_verts; i++) {
		eigen_vert = mesh->verts[i];
		embree::RTCVertex vert = embree::RTCVertex(eigen_vert[0], eigen_vert[1], eigen_vert[2]);
        this->data->verts.push_back(vert);
	}

	for (int i = 0; i < mesh->num_faces; i++) {
		embree::RTCTriangle tri = embree::RTCTriangle(
					mesh->faces[i].v[0],
					mesh->faces[i].v[1],
					mesh->faces[i].v[2]);
        this->data->triangles.push_back(tri);
	}
}

void AccelEmbree::update()
{
    if (this->data->scene)
        embree::rtcDeleteGeometry(this->data->scene);

    this->data->scene = embree::rtcNewTriangleMesh(this->data->triangles.size(), this->data->verts.size(), "default");

    embree::RTCVertex *verts = embree::rtcMapPositionBuffer(this->data->scene);
    for (int i = 0; i < this->data->verts.size(); i++){
        verts[i] = this->data->verts[i];
    }
    embree::rtcUnmapPositionBuffer(this->data->scene);

    embree::RTCTriangle *tris = embree::rtcMapTriangleBuffer(this->data->scene);
    for (int i = 0; i < this->data->triangles.size(); i++) {
        tris[i] = this->data->triangles[i];
	}
    embree::rtcUnmapTriangleBuffer(this->data->scene);

    embree::rtcBuildAccel(this->data->scene, "default");

    this->data->intersector = embree::rtcQueryIntersector1(this->data->scene, "default");
}

static void copy_ray(ERay& eray, Ray* ray)
{
    Eigen::Vector3f *orig, *dir;
    orig = ray->getOrigin();
    dir = ray->getDirection();

    eray.org[0] = (*orig)[0];
    eray.org[1] = (*orig)[1];
    eray.org[2] = (*orig)[2];

    eray.dir[0] = (*dir)[0];
    eray.dir[1] = (*dir)[1];
    eray.dir[2] = (*dir)[2];

    eray.tnear = 1000;
}

bool AccelEmbree::occlude(Ray* ray)
{
	return false;
}

void AccelEmbree::intersect(Ray* ray, Result *result)
{
    result->hit = false;

    ERay eray;
    copy_ray(eray, ray);

    this->data->intersector->intersect((embree::Ray&)(eray));
}
