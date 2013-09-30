#ifndef __RAS_MESH__
#define __RAS_MESH__

class RasMesh
{
private:
	struct RasVertex* verts;
	unsigned short *indices;
	int index_count;

	unsigned int buffers[2];  // 0 Vertex, 1 Index
public:
	RasMesh(class Mesh* mesh);
	~RasMesh();

	void draw();
};

#endif
