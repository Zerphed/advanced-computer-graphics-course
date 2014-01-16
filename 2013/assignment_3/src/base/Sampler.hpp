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

	__forceinline static void setSequenceMode(Sequence::SequenceType sm) 
	{
		// Delete the old sequence
		delete m_sequence;

		// Set the sequence type
		m_sequenceType = sm;

		// Construct a new sequence
		switch (sm) {
			case Sequence::SequenceType_Halton:
				m_sequence = new HaltonSequence(m_nSamples);
				break;
			case Sequence::SequenceType_Random:
				m_sequence = new RandomSequence(m_nSamples);
				break;
			case Sequence::SequenceType_Stratified:
				m_sequence = new StratifiedSequence(m_nSamples);
				break;
			case Sequence::SequenceType_Regular:
				m_sequence = new RegularSequence(m_nSamples);
				break;
		}
	}

	__forceinline static Vec2f uniformSample(int sampleIdx, const Vec2f& min =Vec2f(-1.0f), const Vec2f& max =Vec2f(1.0f))
	{
		Vec2f u = m_sequence->getSample(sampleIdx);//sampleSequence(m_rand, idx, maxUSamples, mode);
		return min+u*(max-min);
	}


	__forceinline static Vec3f uniformSampleHemisphere(int sampleIdx)
	{
		Vec2f u = m_sequence->getSample(sampleIdx);//sampleSequence(m_rand, idx, maxUSamples, mode);

		float r = sqrt(1.0f - u.x * u.x);
		float phi = 2 * FW_PI * u.y;

		return Vec3f(cos(phi) * r, sin(phi) * r, u.x);
	}


	__forceinline static Vec3f cosineSampleHemisphere(int sampleIdx)
	{
		Vec2f u = m_sequence->getSample(sampleIdx);

		float r = sqrt(u.x);
		float theta = 2.0f * FW_PI * u.y;

		float x = r * cos(theta);
		float y = r * sin(theta);

		return Vec3f(x, y, sqrt(max(0.0f, 1.0f - u.x)));
	}

private:	

	static	int						m_nSamples;
	static  Sequence*				m_sequence;
	static	Sequence::SequenceType	m_sequenceType;
};


};	// namespace FW