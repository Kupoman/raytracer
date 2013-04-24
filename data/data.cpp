#include "data.h"
#include <stdio.h>
Scene::~Scene()
{
//	fprintf(stderr, "Destroying scene %d\n", this->meshes.size());
	for(int i = 0; i < this->meshes.size(); ++i) {
//		fprintf(stderr, "%.2f, %.2f, %.2f\n", this->meshes[i]->position(0),
				this->meshes[i]->position(1),this->meshes[i]->position(2));
		delete this->meshes[i];
	}
}
