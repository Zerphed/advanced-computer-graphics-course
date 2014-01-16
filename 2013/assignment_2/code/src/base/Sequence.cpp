#define _CRT_SECURE_NO_WARNINGS

#include "Sequence.hpp"

namespace FW
{

// -------------------------------------------------------------------

Sequence::Sequence(int nSamples) : m_nSamples(nSamples)
{

}

Sequence::~Sequence() 
{

}
    
    
Sequence* Sequence::constructSequence(SequenceType t, int nSamples) 
{
	switch (t)
	{
		case SequenceType_Random:
			return new RandomSequence(nSamples);
			break;
		case SequenceType_Regular:
			return new RegularSequence(nSamples);
			break;
		case SequenceType_Stratified:
			return new StratifiedSequence(nSamples);
			break;
		case SequenceType_Halton:
			return new HaltonSequence(nSamples);
			break;
	}
}

// -------------------------------------------------------------------


RandomSequence::RandomSequence(int nSamples) : Sequence(nSamples)
{

}

RandomSequence::~RandomSequence()
{

}


// -------------------------------------------------------------------


RegularSequence::RegularSequence(int nSamples) : Sequence(nSamples)
{
	// The dimension will be scaled downwards from the nSamples
	m_dim = (int)FW::sqrt(float(nSamples));

	m_step = (1.0f/nSamples);
	m_offset = m_step*0.5f;
}

RegularSequence::~RegularSequence() 
{

}

// -------------------------------------------------------------------


StratifiedSequence::StratifiedSequence(int nSamples) : Sequence(nSamples)
{
	// The dimension will be scaled downwards from the nSamples
	m_dim = (int)FW::sqrt(float(nSamples));

	m_step = (1.0f/nSamples);
	m_offset = m_step*0.5f;
}

StratifiedSequence::~StratifiedSequence()
{

}


// -------------------------------------------------------------------

// Initialize the class statics
bool				HaltonSequence::initialized = false;	
const int			HaltonSequence::maxHaltonIndex = 100000;
std::vector<float>	HaltonSequence::haltonBase2 = std::vector<float>();
std::vector<float>	HaltonSequence::haltonBase3 = std::vector<float>();

HaltonSequence::HaltonSequence(int nSamples) : Sequence(nSamples)
{
	// If the class isn't initialized, initialize
	// i.e. precalculate the Halton values for the entire class
	if (!initialized)
		initialize();
}

HaltonSequence::~HaltonSequence() 
{

}


};	// namespace FW