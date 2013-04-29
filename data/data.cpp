#include "data.h"

Mesh::Mesh()
{
	this->num_faces = 0;
	this->num_verts = 0;
	this->verts = NULL;
	this->faces = NULL;
	this->normals = NULL;
}

Mesh::~Mesh()
{
	if (this->verts)
		delete [] this->verts;
	if (this->faces)
		delete [] this->faces;
	if (this->normals)
		delete [] this->normals;
}
