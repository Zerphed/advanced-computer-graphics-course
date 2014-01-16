#define _CRT_SECURE_NO_WARNINGS

#include "Box.hpp"
#include "RTTriangle.hpp"

namespace FW 
{

Box::Box (const Vec3f& min, const Vec3f& max) : min(min), max(max) 
{

}

Box::Box (const std::vector<RTTriangle>& triangles, int startPrim, int endPrim)
{
	Vec3f maxv(-FLT_MAX);
	Vec3f minv(FLT_MAX);
	const Vec3f* v;
	for (int i = startPrim; i <= endPrim; ++i) {
		for (int j = 0; j < 3; ++j) {
			v = ((triangles)[i]).m_vertices[j];
			for (int k = 0; k < 3; ++k) {
				if ((*v)[k] > maxv[k]) 
					maxv[k] = (*v)[k];
				if ((*v)[k] < minv[k]) 
					minv[k] = (*v)[k];
			}
		}
	}

	min = minv;
	max = maxv;
}

Box::Box (void) { }

Box::~Box (void) { }

} // namespace FW