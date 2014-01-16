#pragma once

#include "base/String.hpp"
#include <vector>

namespace FW
{

class RTTriangle;
class Bvh;
class Node;
class Hit;

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

	// The BVH functions
	void					constructHierarchy		(std::vector<RTTriangle>& triangles);

	// not required, but HIGHLY recommended!
	void					saveHierarchy			(const char* filename, const std::vector<RTTriangle>& triangles);
	void					loadHierarchy			(const char* filename, std::vector<RTTriangle>& triangles);

	bool					rayCastShadow			( const Vec3f& orig, const Vec3f& dir ) const;
	bool					rayIntersectNodeShadow	(const Vec3f& orig, const Vec3f& dir, const float maxval, const Node* node) const;

	// intersection functions
	Hit						rayIntersectNode		(const Vec3f& orig, const Vec3f& dir, const float maxval, const Node* node) const;
	Hit						rayIntersectTriangles	(const Vec3f& orig, const Vec3f& dir, int startPrim, int endPrim) const;

	Hit						rayCast					(const Vec3f& orig, const Vec3f& dir) const;
	bool					rayCastAny				(const Vec3f& orig, const Vec3f& dir) const;


	// This function computes an MD5 checksum of the input scene data,
	// WITH the assumption all vertices are allocated in one big chunk.
	static FW::String		computeMD5				(const std::vector<Vec3f>& vertices);

	std::vector<RTTriangle>*						m_triangles;

	// The root node of the BVH
	Bvh*											m_bvh;
};

} // namespace FW