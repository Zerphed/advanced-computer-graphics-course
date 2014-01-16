#define _CRT_SECURE_NO_WARNINGS

#include "Bvh.hpp"
#include "rtIntersect.inl"
#include "base/Timer.hpp"

#include <map>
#include <iostream>
#include <algorithm>

#define EPSILON 0.000001f
#define MAX_TRIANGLES_PER_LEAF 5

namespace FW
{

// --------------------------------------------------------------------------


Node::Node(int startPrim, int endPrim, const Vec3f& bbMin, const Vec3f& bbMax) : 
	startPrim(startPrim), endPrim(endPrim), bbMin(bbMin), bbMax(bbMax), leftChild(NULL), rightChild(NULL)
{
}

Node::~Node(void)
{
	delete this->leftChild;
	delete this->rightChild;
}


// --------------------------------------------------------------------------
Bvh::BvhMode Bvh::mode = Bvh::BvhMode_Spatial;

Bvh::Bvh(std::vector<RTTriangle>& triangles) : m_triangles(&triangles), m_root(new Node(0, (int)(triangles.size()-1)))
{
	Timer stopwatch;
	stopwatch.start();
	constructTree(m_root);
	stopwatch.end();

	std::cout << "BVH construction complete:" << std::endl;
	std::cout << "--------------------------" << std::endl;
	std::cout << "BVH mode.....: " << (mode ? "SAH" : "Spatial") << std::endl;
	std::cout << "Build time...: " << stopwatch.getTotal() << " sec" << std::endl;
	std::cout << "Total nodes..: " << getNumOfNodes() << std::endl;
	std::cout << "Leaf nodes...: " << getNumOfLeafNodes() << std::endl;
	std::cout << "Maximum depth: " << getDepth() << std::endl << std::endl;
}

Bvh::~Bvh(void)
{
	delete m_root;
}

const Node*	Bvh::getRoot(void) const {
	return m_root;
}

int	Bvh::getDepth(void) const 
{
	return depthRecurse(m_root);
}

int Bvh::depthRecurse(const Node* n) const
{
	if (!n) return 0;
	return std::max(depthRecurse(n->leftChild)+1, depthRecurse(n->rightChild)+1);
}

int Bvh::getNumOfNodes(void) const 
{
	return numOfNodesRecurse(m_root);
}

int Bvh::numOfNodesRecurse(const Node* n) const
{
	if (!n) return 0;
	return 1 + numOfNodesRecurse(n->leftChild) + numOfNodesRecurse(n->rightChild);
}

int Bvh::getNumOfLeafNodes(void) const 
{
	return numOfLeafNodesRecurse(m_root);
}

int Bvh::numOfLeafNodesRecurse(const Node* n) const
{
	if (!n) return 0;
	if (!n->leftChild && !n->rightChild)
		return 1;
	else
		return numOfLeafNodesRecurse(n->leftChild) + numOfLeafNodesRecurse(n->rightChild);
}
/*
int Bvh::partitionPrimitivesSAH(int startPrim, int endPrim)
{
	int bestPos = -1;
	int bestAxis = -1;
	float bestCost = FLT_MAX;

	// For each axis do
	for (int i = 0; i < 3; ++i) {
		std::sort(m_triangles->begin()+startPrim, m_triangles->begin()+endPrim+1, [&](const RTTriangle& t1, const RTTriangle& t2) -> bool 
		{
			return t1.m_centroid[i] < t2.m_centroid[i];
		});

		// For each primitive in the node do
		#pragma omp parallel for shared(bestCost,bestPos,bestAxis)
		for (int it = startPrim+1; it <= endPrim; ++it) {
			float cost;
			if ( (cost = calculateCostSAH(it, startPrim, endPrim)) < bestCost ) 
			{
				bestCost = cost;
				bestPos = it;
				bestAxis = i;
			}
		}
	}

	std::sort(m_triangles->begin()+startPrim, m_triangles->begin()+endPrim+1, [&](const RTTriangle& t1, const RTTriangle& t2) -> bool 
	{
		return t1.m_centroid[bestAxis] < t2.m_centroid[bestAxis];
	});

	return bestPos;
}
*/

int Bvh::partitionPrimitivesSAH(int startPrim, int endPrim)
{
	int size = endPrim-startPrim+1;
	float* costs = new float[size];
	
	float bestCost = FLT_MAX;
	int bestAxis = -1;
	int bestPos = -1;

	// For each axis do
	for (int i = 0; i < 3; ++i) {
		std::sort(m_triangles->begin()+startPrim, m_triangles->begin()+endPrim+1, [&](const RTTriangle& t1, const RTTriangle& t2) -> bool 
		{
			return t1.m_centroid[i] < t2.m_centroid[i];
		});

		// For each primitive in the node do
		//#pragma omp parallel for shared(costs)
		for (int it = startPrim+1; it <= endPrim; ++it) {
			costs[it-(startPrim+1)] = calculateCostSAH(it, startPrim, endPrim);
		}

		auto minCostOnAxis = std::min_element(costs, costs+(size-1));
		
		if (*minCostOnAxis < bestCost) {
			bestCost = *minCostOnAxis;
			bestAxis = i;
			bestPos = minCostOnAxis-costs + startPrim+1;
		}
	}

	std::sort(m_triangles->begin()+startPrim, m_triangles->begin()+endPrim+1, [&](const RTTriangle& t1, const RTTriangle& t2) -> bool 
	{
		return t1.m_centroid[bestAxis] < t2.m_centroid[bestAxis];
	});

	delete [] costs;

	return bestPos;
}

void Bvh::constructTree(Node* n)
{
	if (n->endPrim < n->startPrim) {
		std::cout << "ERROR: endPrim is smaller than startPrim in constructTree" << std::endl;
		return;
	}

	// Compute the node's bounding box and assign the start and endPrim to the node
	std::pair<Vec3f, Vec3f> bb = computeBB(n->startPrim, n->endPrim);
	n->bbMin = bb.first;
	n->bbMax = bb.second;

	if (n->endPrim - n->startPrim + 1 > MAX_TRIANGLES_PER_LEAF)
	{
		// Decide how to split the primitives and
		// perform the actual split: shuffle the indices in the global list
		// so that they're split into two intervals; return the index that
		// separates the two intervals
		int splitPrim;
		switch (Bvh::mode) {
			case BvhMode_Spatial:
				splitPrim = partitionPrimitives(n->startPrim, n->endPrim);
				break;
			case BvhMode_SAH:
				splitPrim = partitionPrimitivesSAH(n->startPrim, n->endPrim);
				break;
		}

		// Create the left child and recursively call this
		// function with the left triangle interval
		n->leftChild = new Node(n->startPrim, splitPrim-1);
		constructTree(n->leftChild);

		// Create the right child and recursively call this
		// function with the right triangle interval
		n->rightChild = new Node(splitPrim, n->endPrim);
		constructTree(n->rightChild);
	}
}

float Bvh::calculateBBoxArea(const std::pair<Vec3f, Vec3f>& bb) const 
{
	Vec3f whd = FW::abs(bb.second - bb.first);
	return 2.0f * (whd.x*whd.z + whd.x*whd.y + whd.z*whd.y);
}

std::pair<Vec3f, Vec3f>	Bvh::computeBB(int startPrim, int endPrim) const
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
	
	return std::make_pair(min, max);
}

int Bvh::partitionPrimitives(int startPrim, int endPrim)
{
	// Use the centroids to split the plane, i.e. find the longest axis 
	// defined by the max and min triangle centroids
	Vec3f maximum(-FLT_MAX);
	Vec3f minimum(FLT_MAX);

	//#pragma omp parallel for shared(maximum,minimum)
	for (int i = startPrim; i <= endPrim; ++i) {
		const Vec3f& centroid = ((*m_triangles)[i]).m_centroid;
		if (centroid[0] > maximum[0])	// X-axis 
			maximum[0] = centroid[0];
		if (centroid[0] < minimum[0])
			minimum[0] = centroid[0];

		if (centroid[1] > maximum[1])   // Y-axis
			maximum[1] = centroid[1];
		if (centroid[1] < minimum[1])
			minimum[1] = centroid[1];

		if (centroid[2] > maximum[2])	// Z-axis
			maximum[2] = centroid[2];
		if (centroid[2] < minimum[2])
			minimum[2] = centroid[2];
	}

	// Calculate the axis lengths (max-min)
	Vec3f axisLengths = maximum - minimum;

	// Pick the longest axis amongst x, y, z
	int axis;
	float longestAxis;
	FINDMAX2(axisLengths[0], axisLengths[1], axisLengths[2], longestAxis, axis);

	// Distribute the primitives to positive/negative based on where
	// the primitive's centroid lies
	float mid = longestAxis*0.5f + minimum[axis];

	// Sort the interval according to where the triangles centroid lies in 
	// the longest axis
	int splitPrim = std::partition(m_triangles->begin()+startPrim, m_triangles->begin()+endPrim+1, [&](const RTTriangle& t) -> bool {
						return ( ( (*(t.m_vertices[0]))[axis] + (*(t.m_vertices[1]))[axis] + (*(t.m_vertices[2]))[axis] ) / 3.0f) > mid; }) - m_triangles->begin();

	return splitPrim;
}

}