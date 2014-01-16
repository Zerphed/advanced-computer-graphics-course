#define _CRT_SECURE_NO_WARNINGS

#include "Sampler.hpp"

namespace FW 
{
	// Initialize the static variables of the sampler class (FIX THESE PROPERLY)
	//int Sampler::m_nSamples = 100000;
	//Sequence::SequenceType Sampler::m_sequenceType = Sequence::SequenceType_Halton;

	int Sampler::m_nSamples = 16;
	Sequence::SequenceType Sampler::m_sequenceType = Sequence::SequenceType_Stratified;
}