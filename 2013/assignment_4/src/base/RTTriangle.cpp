#define _CRT_SECURE_NO_WARNINGS

#include "RTTriangle.hpp"
#include "rtIntersect.inl"

namespace FW
{

RTTriangle::RTTriangle (void) 
{

}

RTTriangle::RTTriangle (const Vec3f* v0, const Vec3f* v1, const Vec3f* v2) {
	m_vertices[0] = v0;
	m_vertices[1] = v1;
	m_vertices[2] = v2;

	m_userPointer = 0;

	// Calculate the triangle normal from the vertices
	m_normal = FW::cross((*m_vertices[1]) - (*m_vertices[0]),
		(*m_vertices[2]) - (*m_vertices[0])).normalized();

	// Calculate the centroid from the vertices
	m_centroid = Vec3f(0.0f);
	for (int i = 0; i < 3; i++) {
		m_centroid[0] += (*(m_vertices)[i])[0];
		m_centroid[1] += (*(m_vertices)[i])[1];
		m_centroid[2] += (*(m_vertices)[i])[2];
	}
	m_centroid *= A3;

	// Calculate the bounding box for the triangle
	float minx, maxx, miny, maxy, minz, maxz;
	FINDMINMAX((*v0)[0], (*v1)[0], (*v2)[0], minx, maxx);
	FINDMINMAX((*v0)[1], (*v1)[1], (*v2)[1], miny, maxy);
	FINDMINMAX((*v0)[2], (*v1)[2], (*v2)[2], minz, maxz);
	m_bbox = Box(Vec3f(minx, miny, minz), Vec3f(maxx, maxy, maxz));
}

RTTriangle::~RTTriangle (void)
{

}

}