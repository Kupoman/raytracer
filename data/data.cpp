#include <iostream>
#include <math.h>


#include <FreeImage.h>


#include "data.h"

Texture::Texture(const char *filename)
{
	FREE_IMAGE_FORMAT format = FreeImage_GetFileType(filename);
	this->bitmap = FreeImage_Load(format, filename);
	this->width = FreeImage_GetWidth(this->bitmap);
	this->height = FreeImage_GetHeight(this->bitmap);
	unsigned char * temp = (unsigned char*) FreeImage_GetBits(this->bitmap);
	this->pixels = new unsigned char[4*this->width*this->height];

	/* Convert from BGRA to RGBA */
	for (unsigned int i = 0; i < this->width * this->height; ++i) {
		this->pixels[i* 4 + 0] = temp[i * 4 + 2];
		this->pixels[i* 4 + 1] = temp[i * 4 + 1];
		this->pixels[i* 4 + 2] = temp[i * 4 + 0];
		this->pixels[i* 4 + 3] = temp[i * 4 + 3];
		//std::cout << (int)temp[i * 4 + 0] << (int)temp[i * 4 + 1] << (int)temp[i * 4 + 2] << (int)temp[i * 4 + 3] << std::endl;
	}
}

Texture::~Texture()
{
	FreeImage_Unload(this->bitmap);
	delete [] this->pixels;
}

Eigen::Vector3f Texture::lookup(float u, float v)
{
	if (fabs(u) > 1) u = fmod(u, 1.0f);
	if (fabs(v) > 1) v = fmod(v, 1.0f);
	if (u < 0.0) u += 1.0;
	if (v < 0.0) v += 1.0;

//	u = 1.0f;
//	v = 0.0f;

	int x = (int)(u * this->width);
	int y = (int)(v * this->height);

	RGBQUAD color;
	if (!FreeImage_GetPixelColor(this->bitmap, x, y, &color))
		std::cout << "Error reading color" << std::endl;
	int index = y*this->width + x;
	index *= 4;

	//std::cout << (unsigned int)color.rgbRed << (unsigned int)color.rgbGreen << (unsigned int)color.rgbBlue << std::endl;
	//std::cout << (unsigned int)this->pixels[4*(this->height*this->width - 1)] << std::endl;
	return Eigen::Vector3f(color.rgbRed, color.rgbGreen, color.rgbBlue);
}

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

