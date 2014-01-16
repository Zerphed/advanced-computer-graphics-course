#pragma once

#include "base/Random.hpp"
#include <vector>

namespace FW
{

class RayTracer;
class Hit;
class RTTriangle;
class Image;
class ShadingMode;

class Sequence
{
public:

	enum SequenceType
	{
        SequenceType_Random = 0,
		SequenceType_Regular,
        SequenceType_Stratified,
		SequenceType_Halton
	};
    
    Sequence(int nSamples);
    virtual ~Sequence();
    
    virtual Vec2f getSample(int n) = 0;
    
    // Get an instance of the proper subclass
    static Sequence* constructSequence( SequenceType t, int nSamples );
    
protected:
    
    FW::Random m_random;
    int        m_nSamples;

};


class RandomSequence : public Sequence
{
public:

    RandomSequence(int nSamples);
    virtual ~RandomSequence();

    __forceinline Vec2f getSample(int n)
	{
		(void)n;
		return m_random.getVec2f();
	}
};


class RegularSequence : public Sequence
{
public:
    RegularSequence(int nSamples);
    virtual ~RegularSequence();

    __forceinline Vec2f getSample(int n)
	{
		return m_offset+(n*m_step);
	}
    
private:
    int		m_dim;
	float	m_step;
	float	m_offset;
};


class StratifiedSequence : public Sequence
{
public:

    StratifiedSequence(int nSamples);
    virtual ~StratifiedSequence();

    __forceinline Vec2f getSample(int n)
	{
		float subPixelOffset = m_random.getF32(-m_offset, m_offset);
		return (m_offset+(n*m_step))+subPixelOffset;
	}

private:

    int		m_dim;
	float	m_step;
	float	m_offset;
};


class HaltonSequence : public Sequence
{
public:

    HaltonSequence(int nSamples);
    virtual ~HaltonSequence();

    __forceinline Vec2f getSample(int n)
	{
		return Vec2f(getHaltonNumber(n, 2), getHaltonNumber(n, 3));
	}

private:

	static bool					initialized;	// A flag to mark whether the class is initialized or not
    static const int			maxHaltonIndex;	// Number of Halton values to precalculate in initialization
	static std::vector<float>	haltonBase2;	// Precalculated vector of maxHaltonIndex num Halton values of base 2
	static std::vector<float>	haltonBase3;	// Precalculated vector of maxHaltonIndex num Halton values of base 3

	__forceinline static float getHaltonNumber(int idx, int base)
	{
		if (base == 2 && idx < haltonBase2.size())
			return haltonBase2[idx];
		if (base == 3 && idx < haltonBase3.size())
			return haltonBase3[idx];

		float result = 0.0f;
		float f = 1.0f / base;
		int i = idx;
		while (i > 0.0f) 
		{
			result = result + f * (i%base);
			i = (int)floor((float)i / base);
			f = f / base;
		}
		return result;
		
	}

	__forceinline static void initialize(void) {
		FILE* fh2 = fopen("halton_base2.pcalc", "rb");
		FILE* fh3 = fopen("halton_base3.pcalc", "rb");

		if (!fh2 || !fh3) {
			fh2 = fopen("halton_base2.pcalc", "wb");
			fh3 = fopen("halton_base3.pcalc", "wb");

			for (int i = 0; i < maxHaltonIndex; ++i) {
				haltonBase2.push_back(getHaltonNumber(i, 2));
				haltonBase3.push_back(getHaltonNumber(i, 3));
			}

			size_t fh2_write = fwrite(&haltonBase2[0], sizeof(float), haltonBase2.size(), fh2);
			size_t fh3_write = fwrite(&haltonBase3[0], sizeof(float), haltonBase3.size(), fh3);
			if (!fh2_write != haltonBase2.size() || !fh3_write != haltonBase3.size())
				printf("ERROR: Could not write all the precalculated Halton values\n");
		}
		else {
			fseek(fh2, 0, SEEK_END);
			unsigned long fh2_len = (unsigned long)ftell(fh2);
			unsigned long fh2_vals = fh2_len/sizeof(float);
			fclose(fh2);

			fseek(fh3, 0, SEEK_END);
			unsigned long fh3_len = (unsigned long)ftell(fh3);
			unsigned long fh3_vals = fh3_len/sizeof(float);
			fclose(fh3);

			haltonBase2 = std::vector<float>(fh2_vals);
			haltonBase3 = std::vector<float>(fh3_vals);

			fh2 = fopen("halton_base2.pcalc", "rb");
			fh3 = fopen("halton_base3.pcalc", "rb");

			size_t fh2_read = fread(&haltonBase2[0], sizeof(float), fh2_vals, fh2);
			size_t fh3_read = fread(&haltonBase3[0], sizeof(float), fh3_vals, fh3);
			if (fh2_read != maxHaltonIndex || fh3_read != maxHaltonIndex)
				printf("ERROR: Could not read all the precalculated Halton values\n");
		}

		fclose(fh2);
		fclose(fh3);

		// Set the class initialization flag as true
		HaltonSequence::initialized = true;
	}
};


};	// namespace FW