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
	// TODO
}

void RayTracer::saveHierarchy(const char* filename, const std::vector<RTTriangle>& triangles )
{
	// TODO
}

void RayTracer::constructHierarchy( std::vector<RTTriangle>& triangles )
{
	m_triangles = &triangles;
	delete m_bvh;				// Delete the possible old BVH
	m_bvh = new Bvh(triangles);	// Construct a new BVH
}

bool RayTracer::rayCastShadow( const Vec3f& orig, const Vec3f& dir ) const
{
	return rayIntersectNodeShadow(orig, dir, (dir-orig).length(), m_bvh->getRoot());
}

bool RayTracer::rayIntersectNodeShadow(const Vec3f& orig, const Vec3f& dir, const float maxval, const Node* node) const
{
	// Test whether the ray intersects the node's bounding box at all
	if (!intersect_bbox(&orig.x, &dir.x, &(node->bbMin).x, &(node->bbMax).x, maxval)) {
		return false;
	}

	// If the node is a leaf node
	if (!node->leftChild && !node->rightChild) {
		return rayIntersectTriangles(orig, dir, node->startPrim, node->endPrim).triangle ? true : false;
	}

	if (rayIntersectNodeShadow(orig, dir, maxval, node->leftChild))
		return true;
	if (rayIntersectNodeShadow(orig, dir, maxval, node->rightChild))
		return true;

	return false;
}


bool RayTracer::rayCastAny( const Vec3f& orig, const Vec3f& dir ) const
{
	return rayCast( orig, dir ).triangle != 0;
}

/** YOUR CODE HERE
	This is where you hierarchically traverse the tree you built!
	You can use the existing code for the leaf nodes.
**/

Hit RayTracer::rayIntersectNode(const Vec3f& orig, const Vec3f& dir, const float maxval, const Node* node) const
{
	// Test whether the ray intersects the node's bounding box at all
	if (!intersect_bbox(&orig.x, &dir.x, &(node->bbMin).x, &(node->bbMax).x, maxval)) {
		return Hit(NULL);
	}

	// If the node is a leaf node
	if (!node->leftChild && !node->rightChild) {
		return rayIntersectTriangles(orig, dir, node->startPrim, node->endPrim);
	}

	Hit left = rayIntersectNode(orig, dir, maxval, node->leftChild);
	Hit right = rayIntersectNode(orig, dir, maxval, node->rightChild);

	if (!left.triangle && !right.triangle)
		return Hit(NULL);
	else if (left.tmin < right.tmin)
		return left;
	else
		return right;
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
		return Hit(&(*m_triangles)[imin], orig + tmin*dir, tmin, umin, vmin);
	else
		return Hit(NULL);
}

Hit RayTracer::rayCast( const Vec3f& orig, const Vec3f& dir ) const
{
	return rayIntersectNode(orig, dir, (dir-orig).length(), m_bvh->getRoot());
}


} // namespace FW