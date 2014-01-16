#pragma once

#include "base/Math.hpp"
#include <limits>

namespace FW {


class RTTriangle;

class Hit
{
public:

	Hit						(const RTTriangle* triangle =(const RTTriangle*)NULL, Vec3f intersection =FLT_MAX, float tmin =FLT_MAX, float umin =0.0f, float vmin =0.0f);
	~Hit					(void);

	const RTTriangle*		triangle;
	Vec3f					intersection;
	float					tmin;
	float					umin;
	float					vmin;
};


}