#include <stdio.h>
#include <iostream>


#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/mesh.h>
#include <assimp/postprocess.h> // Post processing flags
 
#include "camera.h"
#include "loader.h"
#include "scene.h"
#include "data.h"

static Eigen::Vector3f _convertVector(aiVector3D avec)
{
	Eigen::Vector3f vec;
	vec(0) = avec[0];
	vec(1) = avec[1];
	vec(2) = avec[2];

	return vec;
}

static Eigen::Vector3f _convertColor(aiColor3D color)
{
	Eigen::Vector3f vec;
	vec(0) = color[0]*255;
	vec(1) = color[1]*255;
	vec(2) = color[2]*255;

	return vec;
}

static void _traverseNodes(aiNode* node, const aiScene* ascene, Scene* scene)
{
//	fprintf(stderr, "%s\n", node->mName.C_Str());
	for (int i = 0; i < node->mNumMeshes; ++i) {
		Mesh* mesh = new Mesh;
		aiMesh* amesh = ascene->mMeshes[node->mMeshes[i]];
		mesh->position = Eigen::Vector3f(node->mTransformation.a4,
												   node->mTransformation.b4,
												   node->mTransformation.c4);
		mesh->num_verts = amesh->mNumVertices;
		mesh->num_faces = amesh->mNumFaces;
		mesh->verts = new Eigen::Vector3f[mesh->num_verts];
		mesh->normals = new Eigen::Vector3f[mesh->num_verts];
		mesh->texcoords = new Eigen::Vector2f[mesh->num_verts];
		mesh->faces = new Face[mesh->num_faces];
		aiMatrix4x4 normal_mat = aiMatrix4x4(node->mTransformation);
		normal_mat.Inverse().Transpose();
		Eigen::Vector3f temp_texcoord = Eigen::Vector3f();
		for (int j = 0; j < mesh->num_verts; ++j) {
			mesh->verts[j] = _convertVector(node->mTransformation * amesh->mVertices[j]);
			mesh->normals[j] = _convertVector(normal_mat * amesh->mNormals[j]);
			if (amesh->HasTextureCoords(0))
				temp_texcoord = _convertVector(amesh->mTextureCoords[0][j]);
			mesh->texcoords[j] = Eigen::Vector2f(temp_texcoord(0), temp_texcoord(1));
//			fprintf(stderr, "vert: %.2f, %.2f, %.2f\n", mesh->verts[j](0), mesh->verts[j](1), mesh->verts[j](2));
//			fprintf(stderr, "normal: %.2f, %.2f, %.2f\n", mesh->normals[j](0), mesh->normals[j](1), mesh->normals[j](2));
		}
		for (int j = 0; j < mesh->num_faces; ++j) {
//			fprintf(stderr, "Indices: %d, %d, %d\n", amesh->mFaces[j].mIndices[0], amesh->mFaces[j].mIndices[1], amesh->mFaces[j].mIndices[2]);
			mesh->faces[j].v[0] = amesh->mFaces[j].mIndices[0];
			mesh->faces[j].v[1] = amesh->mFaces[j].mIndices[1];
			mesh->faces[j].v[2] = amesh->mFaces[j].mIndices[2];

//			fprintf(stderr, "saved: %d, %d, %d\n", mesh->faces[j].v[0], mesh->faces[j].v[1], mesh->faces[j].v[2]);
		}
		mesh->material = scene->materials[amesh->mMaterialIndex];
		scene->addMesh(mesh);
	}

	for (int i = 0; i < node->mNumChildren; ++i) {
		_traverseNodes(node->mChildren[i], ascene, scene);
	}
}

static void _mergeScene(const aiScene* ascene, Scene* scene)
{
	for (int i = 0; i < ascene->mNumMaterials; ++i) {
		aiMaterial* amat = ascene->mMaterials[i];
		Material* mat = new Material;
		aiColor3D color;
		if (AI_SUCCESS != amat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
			fprintf(stderr, "Error reading color\n");
		}
		mat->color = _convertColor(color);
		amat->Get(AI_MATKEY_COLOR_REFLECTIVE, color);
		mat->reflectivity = color[0];
		amat->Get(AI_MATKEY_OPACITY, mat->alpha);
		mat->alpha = 1.0f - mat->alpha;
		amat->Get(AI_MATKEY_REFRACTI, mat->ior);
//		std::cout << mat->ior << std::endl;
		scene->materials.push_back(mat);

		/* Texture */
		mat->texture = NULL;
		unsigned int texcount = amat->GetTextureCount(aiTextureType_DIFFUSE);
		if (texcount) {
			aiString path;
			if (AI_SUCCESS != amat->GetTexture(aiTextureType_DIFFUSE, 0, &path)) {
				fprintf(stderr, "Error loading texture\n");
			}
			else {
				mat->texture = new Texture(path.C_Str());
				scene->textures.push_back(mat->texture);
//				std::cout << "Filepath: " << path.C_Str() << std::endl;
			}
		}
//		fprintf(stderr, "Color: %.2f, %.2f, %.2f\n", color[0], color[1], color[2]);
	}

	_traverseNodes(ascene->mRootNode, ascene, scene);

	if (ascene->HasCameras()) {
		aiCamera* acamera = ascene->mCameras[0];
		scene->camera->setFOV(acamera->mHorizontalFOV/2);
	}

	for (int i = 0; i < ascene->mNumLights; ++i) {
		Light* light = new Light;
		aiLight* alight = ascene->mLights[i];
		light->position = _convertVector(ascene->mRootNode->FindNode(alight->mName)->mTransformation * alight->mPosition);
		//fprintf(stderr, "LightPos: %.2f, %.2f, %.2f\n", light->position(0), light->position(1), light->position(2));
		light->color = _convertColor(alight->mColorDiffuse);
		scene->lights.push_back(light);
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
