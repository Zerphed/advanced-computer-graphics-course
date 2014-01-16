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

in vec4 fragmentPosition;		// IN: Interpolated fragment position
in vec3 fragmentNormal;			// IN: Fragment normal and diffuse coefficient
in vec3 fragmentTextureCoord;	// IN: texture coordinates

out vec4 fragmentColor;			// OUT: Fragment color to the frame buffer

void main (void) {
	// Getting the fragment color from the texture cube mapping
	vec4 textureColor = texture(textureImage, fragmentTextureCoord);

	float NdotL, VdotR;

	// Ambient light part of Phong lighting: I_a * k_a (constant)
	fragmentColor += textureColor * material.diffuse * vec4(0.3f, 0.3f, 0.3f, 1.0f);

	for (int i = 0; i < numOfLights; i++) {
		
		vec3 lightDirection;
		float intensity = 1.0f;
		float angle;

		switch (lights[i].type) {
			// Directional light (inifinitely far away)
			case 0:
				lightDirection = normalize((viewMatrix * -vec4(lights[i].direction, 0.0f)).xyz);
				break;
			// Point light
			case 1:
				lightDirection = -(viewMatrix * lights[i].position - fragmentPosition).xyz;
				intensity /= dot(lightDirection, lightDirection);
				lightDirection = normalize(lightDirection);
				break;
			// Spotlight
			case 2:
				lightDirection = -(viewMatrix * lights[i].position - fragmentPosition).xyz;
				intensity /= dot(lightDirection, lightDirection);
				lightDirection = normalize(lightDirection);

				// Check whether the spot hits the object at all
				NdotL = dot(fragmentNormal, lightDirection);
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

		// Calculating some initial lighting values
		vec4 fragmentDiffuse = material.diffuse * lights[i].color;
		vec4 fragmentSpecular = material.specular * lights[i].color;

		// Diffuse light part of Phong lighting: I_d * k_d * (N * L)
		NdotL = max(dot(fragmentNormal, lightDirection), 0.0);

		// Toon shading for diffuse light
		if (NdotL > 0.95)
			NdotL = 1.0f;
		else if (NdotL > 0.5)
			NdotL = 0.6f;
		else if (NdotL > 0.25)
			NdotL = 0.4f;
		else
			NdotL = 0.0f;

		fragmentColor += intensity * textureColor * fragmentDiffuse * NdotL;

		// Specular light part of Phong lighting: I_s * k_s * (V * R)
		// (2 * dot(-lightDirection, fragmentNormal) * fragmentNormal) + lightDirection;
		vec3 r = reflect(-lightDirection, fragmentNormal);
		vec3 v = normalize(fragmentPosition.xyz);

		VdotR = dot(v, r);
		VdotR = max(pow(VdotR, material.specularity), 0.0);

		// Toon shading for specular light
		if (VdotR > 0.95)
			VdotR = 1.0f;
		else if (VdotR > 0.5)
			VdotR = 0.6f;
		else if (VdotR > 0.25)
			VdotR = 0.4f;
		else
			VdotR = 0.0f;

		fragmentColor += intensity * fragmentSpecular * VdotR;
	}
}