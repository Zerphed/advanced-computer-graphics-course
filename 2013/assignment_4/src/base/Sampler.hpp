#pragma once

#include "Sequence.hpp"
#include "base/Random.hpp"
#include <vector>

namespace FW
{

class RayTracer;
class Hit;
class RTTriangle;
class Image;
class ShadingMode;

class Sampler
{

public:

	Sampler();
	~Sampler() {	}

	__forceinline static void setSequenceMode(Sequence::SequenceType sm) 
	{
		// Set the sequence type
		m_sequenceType = sm;
	}

	__forceinline static Sequence* getSequenceInstance(Sequence::SequenceType sm = m_sequenceType) 
	{
		// Construct a new sequence
		switch (sm) {
			case Sequence::SequenceType_Halton:
				return new HaltonSequence(m_nSamples);
			case Sequence::SequenceType_Random:
				return new RandomSequence(m_nSamples);
			case Sequence::SequenceType_Stratified:
				return new StratifiedSequence(m_nSamples);
			case Sequence::SequenceType_Regular:
				return new RegularSequence(m_nSamples);
		}
	}

	__forceinline static char* getSequenceInstanceStr(Sequence::SequenceType sm = m_sequenceType) 
	{
		// Return the string instance of the sequence mode
		switch (sm) {
			case Sequence::SequenceType_Halton:
				return "Halton";
			case Sequence::SequenceType_Random:
				return "Random";
			case Sequence::SequenceType_Stratified:
				return "Stratified";
			case Sequence::SequenceType_Regular:
				return "Regular";
		}
	}

	__forceinline static Vec2f uniformSample(Sequence* sequence, int sampleIdx, const Vec2f& min =Vec2f(-1.0f), const Vec2f& max =Vec2f(1.0f))
	{
		Vec2f u = sequence->getSample(sampleIdx);
		return min+u*(max-min);
	}


	__forceinline static Vec3f uniformSampleHemisphere(Sequence* sequence, int sampleIdx)
	{
		Vec2f u = sequence->getSample(sampleIdx);

		float r = sqrt(1.0f - u.x * u.x);
		float phi = 2.0f * FW_PI * u.y;

		return Vec3f(cos(phi) * r, sin(phi) * r, u.x);
	}


	__forceinline static Vec3f cosineSampleHemisphere(Sequence* sequence, int sampleIdx)
	{
		Vec2f u = sequence->getSample(sampleIdx);

		float r = sqrt(u.x);
		float theta = 2.0f * FW_PI * u.y;

		float x = r * cos(theta);
		float y = r * sin(theta);

		return Vec3f(x, y, sqrt(max(0.0f, 1.0f - u.x)));
	}

	__forceinline static Vec3f cosineSampleHemisphere(Vec2f u)
	{
		float r = sqrt(u.x);
		float theta = 2.0f * FW_PI * u.y;

		float x = r * cos(theta);
		float y = r * sin(theta);

		return Vec3f(x, y, sqrt(max(0.0f, 1.0f - u.x)));
	}

private:	

	static	int						m_nSamples;
	static	Sequence::SequenceType	m_sequenceType;
};


};	// namespace FW