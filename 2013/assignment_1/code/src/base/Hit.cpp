#define _CRT_SECURE_NO_WARNINGS

#include "Hit.hpp"
#include "RTTriangle.hpp"

namespace FW {


Hit::Hit	(const RTTriangle* triangle, Vec3f intersection, float tmin, float umin, float vmin) :
	triangle(triangle), intersection(intersection), tmin(tmin), umin(umin), vmin(vmin)
{
}

Hit::~Hit (void) 
{
}


}