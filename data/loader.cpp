#include <stdio.h>


#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/postprocess.h> // Post processing flags
 
 
#include "loader.h"
 
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
	if( !scene)
	{
		fprintf(stderr, "%s\n", importer.GetErrorString());
		return false;
	}
	// Now we can access the file's contents.
	//DoTheSceneProcessing( scene);
	// We're done. Everything will be cleaned up by the importer destructor
	return true;
}