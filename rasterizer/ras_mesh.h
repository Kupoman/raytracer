#ifndef __RAS_MESH__
#define __RAS_MESH__

class RasMesh
{
private:
	struct Material* material;
	struct RasVertex* verts;
	unsigned short *indices;
	int index_count;

	unsigned int vao;
	unsigned int buffers[2];  // 0 Vertex, 1 Index
public:
	RasMesh(class Mesh* mesh);
	~RasMesh();

	// Material Access
	void getMaterialDiffColor(float data[3]);
	class Texture* getMaterialTexture();
	bool getMaterialIsReflective();
	float getMaterialReflectivity();

	void draw();
};

#endif
