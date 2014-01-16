#include "AreaLight.hpp"
#include "Sequence.hpp"
#include "Sampler.hpp"


namespace FW
{

void AreaLight::draw( const Mat4f& worldToCamera, const Mat4f& projection  )
{
	glUseProgram(0);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float*)&projection);
	glMatrixMode(GL_MODELVIEW);
	Mat4f M = worldToCamera *m_xform * m_scale;
	glLoadMatrixf((float*)&M);
	glBegin(GL_TRIANGLES);
	glColor3fv( &m_E.x );
	glVertex3f(1,1,0); glVertex3f(1,-1,0); glVertex3f( -1,-1,0 );
	glVertex3f(1,1,0); glVertex3f( -1,-1,0 ); glVertex3f(-1,1,0); 
	glEnd();
}

void AreaLight::sample( float& pdf, Vec3f& p, int randomIdx, Sequence* s) const
{
	//static int primes[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71};

	// YOUR CODE HERE
	// You should draw a random point on the light source and evaluate the PDF.
	// Store the results in "pdf" and "p".
	// 
	// Note: The "size" member is _one half_ the diagonal of the light source.
	// That is, when you map the square [-1,1]^2 through the scaling matrix
	// 
	// S = ( size.x    0   )
	//     (   0    size.y )
	// 
	// the result is the desired light source quad (see draw() function above).
	// This means the total area of the light source is 4*size.x*size.y.
	// This has implications for the computation of the PDF.

	// For extra credit, implement QMC sampling using some suitable sequence.
	// Use the "base" input for controlling the progression of the sequence from
	// the outside. If you only implement purely random sampling, "base" is not required.
	//Vec4f rpoint = Vec4f(Vec2f( HaltonSequence::getHaltonNumber(randomIdx, primes[(4*b+2)%20]), HaltonSequence::getHaltonNumber(randomIdx, primes[(4*b+3)%20]) ), 0.0f, 1.0f);

	Vec4f rpoint = Vec4f(Sampler::uniformSample(s, randomIdx), 0.0f, 1.0f);
	p = (m_xform*m_scale*rpoint).getXYZ();
	pdf = m_pdf;
}

} // namespace FW