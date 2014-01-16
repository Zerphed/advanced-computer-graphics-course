#pragma once

#include "base/Math.hpp"
#include <vector>
#include "Box.hpp"
#include "RTTriangle.hpp"

namespace FW
{

class RTTriangle;

// The BVH node class
class Node
{
public:
	Node					(int startPrim =0, int endPrim =0, const Vec3f& bbMin =Vec3f(0.0f), const Vec3f& bbMax =Vec3f(0.0f));
	~Node					(void);

	Vec3f					bbMin;		// Axis-aligned BB
	Vec3f					bbMax;

	int						startPrim;	// Indices in the global list
	int						endPrim;

	Node*					leftChild;  // The child nodes, NULL if node is leaf
	Node*					rightChild;
};


// The BVH class
class Bvh
{
public:
	enum BvhMode
	{
		BvhMode_Spatial = 0,
		BvhMode_SAH,
	};

	Bvh						(std::vector<RTTriangle>& triangles);
	~Bvh					(void);

	const Node*				getRoot					(void) const;

	int						getDepth				(void) const;
	int						getNumOfNodes			(void) const;
	int						getNumOfLeafNodes		(void) const;

	static void				setBvhMode				(BvhMode m)
	{
		Bvh::mode = m;
	}

	std::pair<Vec3f, Vec3f>	computeBB				(int startPrim, int endPrim) const;
	float					calculateBBoxArea		(const std::pair<Vec3f, Vec3f>& bb) const;
	__forceinline float		calculateBBoxArea		(int startPrim, int endPrim) const 
	{
		Vec3f max(-FLT_MAX);
		Vec3f min(FLT_MAX);

		//#pragma omp parallel for shared(max,min)
		for (int i = startPrim; i <= endPrim; ++i) {
			const Box& bbox = ((*m_triangles)[i]).m_bbox;
			if (bbox.max[0] > max[0])
				max[0] = bbox.max[0];
			if (bbox.min[0] < min[0])
				min[0] = bbox.min[0];

			if (bbox.max[1] > max[1])
				max[1] = bbox.max[1];
			if (bbox.min[1] < min[1])
				min[1] = bbox.min[1];

			if (bbox.max[2] > max[2])
				max[2] = bbox.max[2];
			if (bbox.min[2] < min[2])
				min[2] = bbox.min[2];
		}

		Vec3f whd = FW::abs(max - min);
		return 2.0f * (whd.x*whd.z + whd.x*whd.y + whd.z*whd.y);
	}


	int						partitionPrimitives		(int startPrim, int endPrim);
	int						partitionPrimitivesSAH	(int startPrim, int endPrim);

private:
	static BvhMode									mode;

	std::vector<RTTriangle>*						m_triangles;
	Node*											m_root;

	void					constructTree			(Node* n);

	__forceinline float		calculateCostSAH		(int splitPrim, int startPrim, int endPrim) const
	{
		int leftCount = splitPrim - startPrim;
		int rightCount = endPrim - splitPrim + 1;

		float leftArea = calculateBBoxArea(startPrim, splitPrim-1);
		float rightArea = calculateBBoxArea(splitPrim, endPrim);

		return (leftArea*leftCount + rightArea*rightCount);
	}

	int						depthRecurse			(const Node* n) const;
	int						numOfNodesRecurse		(const Node* n) const;
	int						numOfLeafNodesRecurse	(const Node* n) const;

};

}