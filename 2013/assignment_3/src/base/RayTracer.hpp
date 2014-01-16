#pragma once

#include "base/String.hpp"
#include <vector>

//------------------------------------------------------------------------

namespace RT
{
	class BHTempNode;
}


namespace FW
{

// Forward declarations
class RTTriangle;
class Bvh;
class Node;
class Hit;

//
// The ray tracer uses its own, extremely simple interface for vertices and triangles.
// This will make it extremely simple for you to rip it out and reuse in subsequent projects!
// Here's what you do:
//  1. put all vertices in a big vector,
//  2. put all triangles in a big vector,
//  3. make the triangle structs vertex pointers point to the correct places in the big vertex chunk.
//

// this struct helps to map the linear indices of the ray tracer triangles
// back to the (submesh, tri) indexing of BaseMesh. Helps when getting colors,
// normals, etc.
struct RTToMesh
{
	int submesh;
	int tri_idx;
};

// Main class for tracing rays using BVHs
class RayTracer
{
public:
						RayTracer				(void);
						~RayTracer				(void);

	void				constructHierarchy		(std::vector<RTTriangle>& triangles);

	// not required, but HIGHLY recommended!
	void				saveHierarchy			(const char* filename, const std::vector<RTTriangle>& triangles);
	void				loadHierarchy			(const char* filename, const std::vector<RTTriangle>& triangles);

	bool				rayCastShadow			( const Vec3f& orig, const Vec3f& dir ) const;
	bool				rayIntersectNodeShadow	(const Vec3f& orig, const Vec3f& dir, const float maxval, const Node* node) const;

	// intersection functions
	Hit					rayIntersectNode		(const Vec3f& orig, const Vec3f& dir, const float maxval, const Node* node) const;
	Hit					rayIntersectTriangles	(const Vec3f& orig, const Vec3f& dir, int startPrim, int endPrim) const;

	Hit					rayCast					(const Vec3f& orig, const Vec3f& dir) const;
	bool			    rayCastAny				(const Vec3f& orig, const Vec3f& dir);


	// This function computes an MD5 checksum of the input scene data,
	// WITH the assumption all vertices are allocated in one big chunk.
	static FW::String	computeMD5				(const std::vector<Vec3f>& vertices);

	RT::BHTempNode*				m_hierarchy;
	std::vector<RTTriangle>*	m_triangles;
	unsigned int*				m_list;
	int							m_treeDepth;
	float						m_mn[3];
	float						m_mx[3];
	unsigned int				m_listSize;
	void*						m_impl;

	// The root node of the BVH
	Bvh*						m_bvh;
};

} // namespace FW