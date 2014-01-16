#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <iostream>
#include <algorithm>

#include "base/Defs.hpp"
#include "base/Math.hpp"

#include "RayTracer.hpp"
#include "Bvh.hpp"
#include "Hit.hpp"
#include "RTTriangle.hpp"
#include "rtIntersect.inl"

#define EPSILON 0.000001f
#define MAX_TRIANGLES_PER_LEAF 5

// Helper function for hashing scene data for caching BVHs
extern "C" void MD5Buffer( void* buffer, size_t bufLen, unsigned int* pDigest );

namespace FW
{


// --------------------------------------------------------------------------


String RayTracer::computeMD5( const std::vector<Vec3f>& vertices )
{
	unsigned char digest[16];
	MD5Buffer( (void*)&vertices[0], sizeof(Vec3f)*vertices.size(), (unsigned int*)digest );

	// turn into string
	char ad[33];
	for ( int i = 0; i < 16; ++i )
		::sprintf( ad+i*2, "%02x", digest[i] );
	ad[32] = 0;

	return FW::String( ad );
}
// --------------------------------------------------------------------------


RayTracer::RayTracer() : m_bvh(NULL)
{
}

RayTracer::~RayTracer()
{
	delete m_bvh;
}

void RayTracer::loadHierarchy( const char* filename, std::vector<RTTriangle>& triangles )
{

}

void RayTracer::saveHierarchy(const char* filename, const std::vector<RTTriangle>& triangles )
{

}

void RayTracer::constructHierarchy( std::vector<RTTriangle>& triangles )
{
	m_triangles = &triangles;
	delete m_bvh;				// Delete the possible old BVH
	m_bvh = new Bvh(triangles);	// Construct a new BVH
}

bool RayTracer::rayCastAny( const Vec3f& orig, const Vec3f& dir ) const
{
	return rayCast( orig, dir ).triangle != 0;
}

/** YOUR CODE HERE
	This is where you hierarchically traverse the tree you built!
	You can use the existing code for the leaf nodes.
**/
std::pair<bool, float> RayTracer::rayIntersectBoundingBox(const Vec3f& orig, const Vec3f& dir, const Vec3f& bbMin, const Vec3f& bbMax) const
{
	float min, max, ymin, ymax, zmin, zmax;

	// Calculate the maximum and minimum on the x-axis
	float divx = 1.0f/dir.x;
	if (divx >= 0) {
		min = (bbMin.x - orig.x) * divx;
		max = (bbMax.x - orig.x) * divx;
	}
	else {
		min = (bbMax.x - orig.x) * divx;
		max = (bbMin.x - orig.x) * divx;
	}

	// Calculate the maximum and minimum on the y-axis
	float divy = 1.0f/dir.y;
	if (divy >= 0) {
		ymin = (bbMin.y - orig.y) * divy;
		ymax = (bbMax.y - orig.y) * divy;
	}
	else {
		ymin = (bbMax.y - orig.y) * divy;
		ymax = (bbMin.y - orig.y) * divy;
	}

	if ( (min > ymax) || (ymin > max) )
		return std::make_pair(false, std::numeric_limits<float>::max());

	// Make sure the min and max hold the most binding values
	if (ymin > min)
		min = ymin;
	if (ymax < max)
		max = ymax;

	// Calculate the maximum and minimum on the z-axis
	float divz = 1.0f/dir.z;
	if (divz >= 0) {
		zmin = (bbMin.z - orig.z) * divz;
		zmax = (bbMax.z - orig.z) * divz;
	}
	else {
		zmin = (bbMax.z - orig.z) * divz;
		zmax = (bbMin.z - orig.z) * divz;
	}

	if ( (min > zmax) || (zmin > max) )
		return std::make_pair(false, std::numeric_limits<float>::max());
	
	// Make sure the min and max hold the most binding values
	if (zmin > min)
		min = zmin;
	if (zmax < max)
		max = zmax;

	if ( max > 0.0f && min < (dir-orig).length() )
		return std::make_pair(true, min);
	else
		return std::make_pair(false, std::numeric_limits<float>::max());
} 


Hit RayTracer::rayIntersectNode(const Vec3f& orig, const Vec3f& dir, const Node* node) const
{
	// Test whether the ray intersects the node's bounding box at all
	std::pair<bool, float> bbRet = rayIntersectBoundingBox(orig, dir, node->bbMin, node->bbMax);
	if (!bbRet.first) {
		return Hit(NULL);
	}

	// If the node is a leaf node
	if (!node->leftChild && !node->rightChild) {
		return rayIntersectTriangles(orig, dir, node->startPrim, node->endPrim);
	}

	Hit left = rayIntersectNode(orig, dir, node->leftChild);
	Hit right = rayIntersectNode(orig, dir, node->rightChild);

	if (!left.triangle && !right.triangle) {
		return Hit(NULL);
	}
	else if (left.tmin < right.tmin) {
		return left;
	}
	else {
		return right;
	}
}


Hit RayTracer::rayIntersectTriangles(const Vec3f& orig, const Vec3f& dir, int startPrim, int endPrim) const
{
	float umin = 0.0f, vmin = 0.0f, tmin = 1.0f;
	int imin = -1;

	// naive loop over all triangles
	for ( int i = startPrim; i <= endPrim; ++i )
	{
		float t = std::numeric_limits<float>::max(), u, v;
		if ( intersect_triangle1( &orig.x,
								  &dir.x,
								  &(*m_triangles)[i].m_vertices[0]->x,
								  &(*m_triangles)[i].m_vertices[1]->x,
								  &(*m_triangles)[i].m_vertices[2]->x,
								  t, u, v ) )
		{
			if ( t > 0.0f && t < tmin )
			{
				imin = i;
				tmin = t;
				umin = u;
				vmin = v;
			}
		}
	}

	if ( imin != -1 )
	{
		return Hit(&(*m_triangles)[imin], orig + tmin*dir, tmin, umin, vmin);
	}
	else
	{
		return Hit(NULL);
	}
}

Hit RayTracer::rayCast( const Vec3f& orig, const Vec3f& dir ) const
{
	return rayIntersectNode(orig, dir, m_bvh->getRoot());
}


} // namespace FW