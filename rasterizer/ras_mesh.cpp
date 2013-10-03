#include "GL/glew.h"

#include "stdio.h"

#include "data/data.h"

#include "ras_mesh.h"
#include "ras_vertex.h"

RasMesh::RasMesh(Mesh *mesh)
{
	glGenVertexArrays(1, &this->vao);
	glBindVertexArray(this->vao);

	glGenBuffers(2, this->buffers);

	this->verts = new RasVertex[mesh->num_verts];
	this->indices = new unsigned short[mesh->num_faces * 3];

	for (int i = 0; i < mesh->num_verts; i++) {
		this->verts[i].position[0] = mesh->verts[i][0];
		this->verts[i].position[1] = mesh->verts[i][1];
		this->verts[i].position[2] = mesh->verts[i][2];

		this->verts[i].normal[0] = mesh->normals[i][0];
		this->verts[i].normal[1] = mesh->normals[i][1];
		this->verts[i].normal[2] = mesh->normals[i][2];

		this->verts[i].uv[0] = mesh->texcoords[i][0];
		this->verts[i].uv[1] = mesh->texcoords[i][1];
	}

	for (int i = 0; i < mesh->num_faces; i++) {
		this->indices[(3 * i) + 0] = mesh->faces[i].v[0];
		this->indices[(3 * i) + 1] = mesh->faces[i].v[1];
		this->indices[(3 * i) + 2] = mesh->faces[i].v[2];
	}

	glBindBuffer(GL_ARRAY_BUFFER, this->buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, mesh->num_verts*sizeof(RasVertex), this->verts, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_faces*sizeof(unsigned short)*3, this->indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RasVertex), (void*)offsetof(RasVertex, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(RasVertex), (void*)offsetof(RasVertex, normal));

	glBindVertexArray(0);

	this->index_count = mesh->num_faces * 3;
}

RasMesh::~RasMesh()
{
	glDeleteBuffers(2, this->buffers);

	delete [] this->verts;
	delete [] this->indices;
}

void RasMesh::draw()
{
	glBindVertexArray(this->vao);

	glDrawElements(GL_TRIANGLES, this->index_count, GL_UNSIGNED_SHORT, (void*)0);

	glBindVertexArray(0);
}
