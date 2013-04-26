#include <stdio.h>


#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/mesh.h>
#include <assimp/postprocess.h> // Post processing flags
 
#include "camera.h"
#include "loader.h"
#include "scene.h"
#include "data.h"

static void _traverseNodes(aiNode* node, Scene* scene)
{
//	fprintf(stderr, "%s\n", node->mName.C_Str());
	if (node->mNumMeshes) {
		Mesh *mesh = new Mesh;
		mesh->position = Eigen::Vector3f(node->mTransformation.a4,
												   node->mTransformation.b4,
												   node->mTransformation.c4);
		scene->addMesh(mesh);
	}

	for (int i = 0; i < node->mNumChildren; ++i) {
		_traverseNodes(node->mChildren[i], scene);
	}
}

static void _mergeScene(const aiScene* ascene, Scene* scene)
{
	_traverseNodes(ascene->mRootNode, scene);

	if (ascene->HasCameras()) {
		aiCamera* acamera = ascene->mCameras[0];
		scene->camera->setFOV(acamera->mHorizontalFOV);
	}
}

bool loadFile(const std::string filename, Scene* scene)
{
	// Create an instance of the Importer class
	Assimp::Importer importer;
	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// propably to request more postprocessing than we do in this example.
	const aiScene* ascene = importer.ReadFile( filename,
											aiProcess_CalcTangentSpace |
											aiProcess_Triangulate |
											aiProcess_JoinIdenticalVertices |
											aiProcess_SortByPType);
	// If the import failed, report it
	if( !ascene)
	{
		fprintf(stderr, "%s\n", importer.GetErrorString());
		return false;
	}
	// Now we can access the file's contents.
	_mergeScene(ascene, scene);
	// We're done. Everything will be cleaned up by the importer destructor
	return true;
}
