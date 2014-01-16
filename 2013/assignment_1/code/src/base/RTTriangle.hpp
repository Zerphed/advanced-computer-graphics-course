#pragma once

#include "base/Math.hpp"

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

	Vec3f		calculateCentroid	(void) const;

	Vec3f*		m_vertices[3];
	void*	    m_userPointer;
};

}