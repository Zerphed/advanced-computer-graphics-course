#version 330

#define MAX_LIGHTS 12

struct LightSource {
	vec4 position;
	vec3 direction;
	vec4 color;
	float spotCutoff;
	float spotCoeff;
	int type;
};

struct Material {
	vec4 diffuse;
	vec4 specular;
	float specularity;
};

uniform mat4 viewMatrix;

uniform LightSource lights[MAX_LIGHTS];
uniform int numOfLights;
uniform Material material;

uniform samplerCube textureImage;
uniform samplerCube normalMapImage;

in vec4 fragmentPosition;		// IN: Interpolated fragment position
in vec3 fragmentNormal;			// IN: Fragment normal and diffuse coefficient
in vec3 fragmentTextureCoord;	// IN: texture coordinates

out vec4 fragmentColor;			// OUT: Fragment color to the frame buffer

void main (void) {
	// Look up the texture from the normal mapping
	vec4 normalPerturb = 2.0f * texture(normalMapImage, fragmentTextureCoord) - 1.0f;
	vec3 calcNormal = normalize(normalPerturb.xyz * 0.4f + fragmentNormal);

	// Getting the fragment color from the texture cube mapping
	//vec4 textureColor = texture(textureImage, fragmentTextureCoord);

	// Reflection mapping, for the lulz :)
	vec4 textureColor = texture(textureImage, calcNormal);

	// Ambient light part of Phong lighting: I_a * k_a (constant)
	fragmentColor += textureColor * material.diffuse * vec4(0.3f, 0.3f, 0.3f, 1.0f);

	float NdotL, VdotR;

	for (int i = 0; i < numOfLights; i++) {
		
		vec3 lightDirection;
		float intensity = 1.0f;
		float angle;

		switch (lights[i].type) {
			// Directional light (inifinitely far away)
			case 0: 
			{
				lightDirection = normalize((viewMatrix * vec4(lights[i].direction, 0.0f)).xyz);
				break;
			}
			// Point light
			case 1:
			{
				lightDirection = -(viewMatrix * lights[i].position - fragmentPosition).xyz;
				intensity /= dot(lightDirection, lightDirection);
				lightDirection = normalize(lightDirection);
				break;
			}
			// Spotlight
			case 2:
			{
				lightDirection = -(viewMatrix * lights[i].position - fragmentPosition).xyz;
				intensity /= dot(lightDirection, lightDirection);
				lightDirection = normalize(lightDirection);

				// Check whether the spotlight hits the object at all
				NdotL = dot(calcNormal, lightDirection);

				if (NdotL > 0.0f) {
					angle = degrees(acos(dot(normalize((viewMatrix * vec4(lights[i].direction, 0.0f)).xyz), lightDirection.xyz)));
					
					if (angle >= lights[i].spotCutoff) {
						float t = clamp((angle - lights[i].spotCutoff) * lights[i].spotCoeff, 0.0f, 1.0f);
						intensity *= (2*(t*t*t)) - 3*(t*t) + 1.0f;
					}
				}
				else
					intensity = 0.0f;
				break;
			}
		}

		// Calculating some initial lighting values
		vec4 fragmentDiffuse = material.diffuse * lights[i].color;
		vec4 fragmentSpecular = material.specular * lights[i].color;

		// Diffuse light part of Phong lighting: I_d * k_d * (N * L)
		NdotL = max(dot(calcNormal, lightDirection), 0.0);
		fragmentColor += intensity * textureColor * fragmentDiffuse * NdotL;

		// Specular light part of Phong lighting: I_s * k_s * (V * R)
		// (2 * dot(-lightDirection, fragmentNormal) * fragmentNormal) + lightDirection;
		vec3 r = reflect(-lightDirection, calcNormal);
		vec3 v = normalize(fragmentPosition.xyz);

		VdotR = dot(v, r);
		VdotR = max(pow(VdotR, material.specularity), 0.0);

		fragmentColor += intensity * fragmentSpecular * VdotR;
	}
}