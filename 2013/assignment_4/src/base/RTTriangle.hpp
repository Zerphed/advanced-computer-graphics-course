#pragma once

#include "base/Math.hpp"
#include "Box.hpp"
#define A3 1.0f/3.0f

namespace FW
{
//
// The ray tracer uses its own, extremely simple interface for vertices and triangles.
// This will make it extremely simple for you to rip it out and reuse in subsequent projects!
// Here's what you do:
//  1. put all vertices in a big vector,
//  2. put all triangles in a big vector,
//  3. make the triangle structs vertex pointers point to the correct places in the big vertex chunk.
//

// The userpointer member can be used for identifying the triangle in the "parent" mesh representation.
class RTTriangle
{
public:

	RTTriangle (void);
	
	RTTriangle (const Vec3f* v0, const Vec3f* v1, const Vec3f* v2);
	
	~RTTriangle (void);

	__forceinline const Vec3f&	getNormal			(void) const 
	{
		return m_normal;
	}

	__forceinline Vec3f			getNormal			(const Vec3f& point) const 
	{
		Vec3f bc = getBarycentrics(point);
		return ( (bc[0] * (*m_vertexNormals[0])) + 
				 (bc[1] * (*m_vertexNormals[1])) + 
				 (bc[2] * (*m_vertexNormals[2])) ).normalized();
	}

	__forceinline Vec3f			getNormal			(const Vec3f& point, const Vec3f& bc) const 
	{
		(void)point;
		return ( (bc[0] * (*m_vertexNormals[0])) + 
				 (bc[1] * (*m_vertexNormals[1])) + 
				 (bc[2] * (*m_vertexNormals[2])) ).normalized();
	}

	__forceinline Vec3f		calculateCentroid	(void) const
	{
		return m_centroid;
	}

	__forceinline Vec3f		getBarycentrics (const Vec3f& point) const 
	{
		float area = (((*m_vertices[1])-(*m_vertices[0])).cross((*m_vertices[2])-(*m_vertices[0]))).length();
		float area_inv = 1.0f / area;

		float a = FW::cross( ((*m_vertices[2])-(*m_vertices[1])), (point-(*m_vertices[1])) ).length() * area_inv;
		float b = FW::cross( ((*m_vertices[0])-(*m_vertices[2])), (point-(*m_vertices[2])) ).length() * area_inv;
		float c = 1.0f - a - b;

		return Vec3f(a, b, c);
	}

	Vec3f const*			m_vertices[3];
	Vec3f const*			m_vertexNormals[3];
	void*					m_userPointer;
	Box						m_bbox;
	Vec3f					m_normal;
	Vec3f					m_centroid;
};

}