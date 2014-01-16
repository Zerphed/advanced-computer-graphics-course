#define _CRT_SECURE_NO_WARNINGS

#include "RTTriangle.hpp"

namespace FW
{

Vec3f RTTriangle::calculateCentroid(void) const
{
	Vec3f centroid(0.0f);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			centroid[j] += (*(m_vertices)[i])[j];
		}
	}
	centroid /= 3.0f;
	return centroid;
}

}