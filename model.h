#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<std::vector<int> > faces_;
	std::vector<Vec3f> uv_verts_;
	std::vector<Vec3f> vn_verts_;
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	int nuv_verts();
	int nvn_verts();
	Vec3f vert(int i);
	std::vector<int> face(int idx);
	Vec3f uv_vert(int i);
	Vec3f vn_vert(int i);
};

#endif //__MODEL_H__