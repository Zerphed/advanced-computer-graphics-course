#include "InstantRadiosity.hpp"
#include "ShadowMap.hpp"
#include "Hit.hpp"
#include "RTTriangle.hpp"

namespace FW 
{

void InstantRadiosity::castIndirect(RayTracer *rt, MeshWithColors *scene, const LightSource& ls, int num)
{
	// If the caller requests a different number of lights than before, reallocate everything.
	// (This is OpenGL resource management stuff, don't touch unless you specifically need to)
	if (m_indirectLights.size() != num)
	{
		printf("Deleting %i indirect light sources.\n", num);
		for (auto iter = m_indirectLights.begin(); iter != m_indirectLights.end(); iter++)
			iter->freeShadowMap();
		m_indirectLights.resize(num);
		for (auto iter = m_indirectLights.begin(); iter != m_indirectLights.end(); iter++)
			iter->setEnabled(false);
	}

	// Request #num exiting rays from the light.
	std::vector<Vec3f> origs, dirs, E_times_pdf;
	ls.sampleEmittedRays(num, origs, dirs, E_times_pdf);

	// At this point m_indirectLights holds #num lights that are off.
	// Loop through the rays and fill in the corresponding lights in m_indirectLights
	// based on what happens to the ray.
	for (int i = 0; i < num; i++)
	{
		// Intersect against the scene
		Hit h = rt->rayCast(origs[i], dirs[i]);
		const RTTriangle* tri = h.triangle;

		if ( tri != 0 )
		{
			// YOUR CODE HERE
			// Ray hit the scene, now position the light m_indirectLights[i] correctly,
			// color it based on the texture or diffuse color, etc. (see the LightSource declaration for the list 
			// of things that a light source needs to have)
			// A lot of this code is like in the Assignment 2's corresponding routine.

			const RTToMesh* map = (const RTToMesh*)tri->m_userPointer;
			const Vec3i& indices = scene->indices( map->submesh )[ map->tri_idx ];

			// Fetch the barycentrics
			Vec3f barycentrics = tri->getBarycentrics(h.intersection);
			
			// Check for backfaces => don't accumulate if we hit a surface from below!
			const Vec3f tnormal = tri->getNormal(h.intersection, barycentrics);

			// Divide incident irradiance by PI so that we can turn it into outgoing
			// radiosity by multiplying by the reflectance factor below.
			//Ei *= (1.0f / FW_PI);
		
			Vec3f Ei;
			// check for texture
			const MeshBase::Material& mat = scene->material(map->submesh);
			if ( mat.textures[MeshBase::TextureType_Diffuse].exists() )
			{
				// Yes, texture; fetch diffuse albedo from there.
				// First interpolate UV coordinates from the vertices using barycentrics
				// Then turn those into pixel coordinates within the texture
				// Finally fetch the color using Image::getVec4f() (point sampling is fine).
				// Use the result as the diffuse reflectance instead of mat.diffuse.

				const Texture& tex = mat.textures[MeshBase::TextureType_Diffuse];
				const Image& teximg = *tex.getImage();

				Vec2f texCoord = barycentrics[0] * scene->vertex(indices[0]).t + 
								 barycentrics[1] * scene->vertex(indices[1]).t + 
								 barycentrics[2] * scene->vertex(indices[2]).t;

				Vec2i texCoordi = Vec2i( (texCoord.x - floor(texCoord.x))*teximg.getSize().x, 
										 (texCoord.y - floor(texCoord.y))*teximg.getSize().y );

				Ei = teximg.getVec4f(texCoordi).getXYZ();
			}
			else
			{
				Ei = mat.diffuse.getXYZ();
			}

			Vec3f emission = E_times_pdf[i] * Ei;

			// Set the indirect light parameters
			m_indirectLights[i].setPosition(h.intersection);
			Mat3f orientation = LightSource::formBasis(-tnormal);
			m_indirectLights[i].setOrientation(orientation);
			m_indirectLights[i].setEmission(emission);
			m_indirectLights[i].setFOV(m_indirectFOV);

			// Replace this with true once your light is ready to be used in rendering
			m_indirectLights[i].setEnabled(true);
		}
		else
		{
			// If we missed the scene, disable the light so it's skipped in all rendering operations.
			m_indirectLights[i].setEnabled(false);
		}
	}
}

void InstantRadiosity::renderShadowMaps(MeshWithColors *scene)
{
	// YOUR CODE HERE
	// Loop through all lights, and call the shadow map renderer for those that are enabled.
	// (see App::renderFrame for an example usage of the SM rendering call)
	for (auto iter = m_indirectLights.begin(); iter != m_indirectLights.end(); ++iter) {
		if (iter->isEnabled())
			iter->renderShadowMap(m_gl, scene, &m_smContext);
	}
}





//////////// Stuff you probably will not need to touch:

void InstantRadiosity::setup(GLContext* gl, Vec2i resolution)
{
	m_gl = gl;

	// Clear any existing reserved textures
	for (auto iter = m_indirectLights.begin(); iter != m_indirectLights.end(); ++iter)
		iter->freeShadowMap();

	// Set up the shadow map buffers
	m_smContext.setup(resolution);

}

void InstantRadiosity::draw( const Mat4f& worldToCamera, const Mat4f& projection  )
{
	// Just visualize all the light source positions
	for (auto iter = m_indirectLights.begin(); iter != m_indirectLights.end(); ++iter)
		if (iter->isEnabled())
			iter->draw(worldToCamera, projection, true, false);
}


}