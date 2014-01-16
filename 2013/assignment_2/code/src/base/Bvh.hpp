#pragma once

#include "base/Math.hpp"
#include <vector>

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
	float					calculateBBoxArea		(int startPrim, int endPrim) const;
	int						partitionPrimitives		(int startPrim, int endPrim);
	int						partitionPrimitivesSAH	(int startPrim, int endPrim);

private:
	static BvhMode									mode;

	std::vector<RTTriangle>*						m_triangles;
	Node*											m_root;

	void					constructTree			(Node* n);
	float					calculateCostSAH		(int splitPrim, int axis, int startPrim, int endPrim) const; 

	int						depthRecurse			(const Node* n) const;
	int						numOfNodesRecurse		(const Node* n) const;
	int						numOfLeafNodesRecurse	(const Node* n) const;

};

}