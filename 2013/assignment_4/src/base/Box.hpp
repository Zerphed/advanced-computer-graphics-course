#pragma once

#include "base/Math.hpp"
#include <vector>

namespace FW
{

class RTTriangle;

class Box
{
public:
	Box			(const Vec3f& min, const Vec3f& max);
	Box			(const std::vector<RTTriangle>& triangles, int startPrim, int endPrim);
	Box			(void);

	~Box		(void);

	__forceinline float	area () 
	{
		Vec3f whd = FW::abs(max - min);
		return 2.0f * (whd.x*whd.z + whd.x*whd.y + whd.z*whd.y);
	}

	Vec3f		min;
	Vec3f		max;
};

}