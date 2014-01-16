#define _CRT_SECURE_NO_WARNINGS

#include "Bvh.hpp"
#include "RTTriangle.hpp"

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


Bvh::Bvh(std::vector<RTTriangle>& triangles) : m_triangles(&triangles), m_root(new Node(0, (int)(triangles.size()-1)))
{
	constructTree(m_root);
	std::cout << "DEBUG: BVH construction complete." << std::endl;
	std::cout << "Total nodes: " << getNumOfNodes() << std::endl;
	std::cout << "Leaf nodes: " << getNumOfLeafNodes() << std::endl;
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

void Bvh::constructTree(Node* n) 
{
	if (n->endPrim < n->startPrim) {
		std::cout << "ERROR: endPrim is smaller than startPrim in constructTree" << std::endl;
		return;
	}

	// Compute the node's bounding box and assign the start and endPrim to the node
	std::pair<Vec3f, Vec3f> bb = Bvh::computeBB(n->startPrim, n->endPrim);
	n->bbMin = bb.first;
	n->bbMax = bb.second;

	if (n->endPrim - n->startPrim + 1 > MAX_TRIANGLES_PER_LEAF)
	{
		// Decide how to split the primitives and
		// perform the actual split: shuffle the indices in the global list
		// so that they're split into two intervals; return the index that
		// separates the two intervals
		int splitPrim = Bvh::partitionPrimitives(n->startPrim, n->endPrim);		

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

std::pair<Vec3f, Vec3f>	Bvh::computeBB(int startPrim, int endPrim) const
{
	Vec3f max(-std::numeric_limits<float>::max());
	Vec3f min(std::numeric_limits<float>::max());

	Vec3f* v;
	for (int i = startPrim; i <= endPrim; ++i) {
		for (int j = 0; j < 3; ++j) {
			v = ((*m_triangles)[i]).m_vertices[j];
			for (int k = 0; k < 3; ++k) {
				if ((*v)[k] > max[k]) 
					max[k] = (*v)[k];
				if ((*v)[k] < min[k]) 
					min[k] = (*v)[k];
			}
		}
	}
	
	return std::make_pair(min, max);
}

int Bvh::partitionPrimitives(int startPrim, int endPrim) const
{
	// Use the centroids to split the plane, i.e. find the longest axis 
	// defined by the max and min triangle centroids
	Vec3f maximum(-std::numeric_limits<float>::max());
	Vec3f minimum(std::numeric_limits<float>::max());

	Vec3f centroid;
	for (int i = startPrim; i <= endPrim; ++i) {
		centroid = ((*m_triangles)[i]).calculateCentroid();
		for (int j = 0; j < 3; ++j) {
			if (centroid[j] > maximum[j]) 
				maximum[j] = centroid[j];
			if (centroid[j] < minimum[j])
				minimum[j] = centroid[j];
		}
	}

	// Calculate the axis lengths (max-min)
	Vec3f axisLengths(maximum.x-minimum.x, maximum.y-minimum.y, maximum.z-minimum.z);

	// Pick the longest axis amongst x, y, z
	int axis = 0;
	float longestAxis = max(max(axisLengths.x, axisLengths.y), axisLengths.z);
	
	if (abs(longestAxis - axisLengths.x) < EPSILON)
		axis = 0;
	else if (abs(longestAxis - axisLengths.y) < EPSILON)
		axis = 1;
	else if (abs(longestAxis - axisLengths.z) < EPSILON)
		axis = 2;
	else
		std::cout << "ERROR: No longest axis in partitionPrimitives" << std::endl;

	// Distribute the primitives to positive/negative based on where
	// the primitive's centroid lies
	float mid = longestAxis/2.0f + minimum[axis];

	// Sort the interval according to where the triangles centroid lies in 
	// the longest axis
	int splitPrim = std::partition(m_triangles->begin()+startPrim, m_triangles->begin()+endPrim+1, [&](const RTTriangle& t) -> bool {
						return ( ( (*(t.m_vertices[0]))[axis] + (*(t.m_vertices[1]))[axis] + (*(t.m_vertices[2]))[axis] ) / 3.0f) > mid; }) - m_triangles->begin();

	return splitPrim;
}

}