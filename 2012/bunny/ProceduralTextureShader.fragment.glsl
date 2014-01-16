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

uniform int time;

uniform mat4 viewMatrix;

uniform LightSource lights[MAX_LIGHTS];
uniform int numOfLights;
uniform Material material;
uniform samplerCube textureImage;

in vec4 fragmentPosition;		// IN: Interpolated fragment position
in vec3 fragmentNormal;			// IN: Fragment normal and diffuse coefficient
in vec3 fragmentTextureCoord;	// IN: texture coordinates

out vec4 fragmentColor;			// OUT: Fragment color to the frame buffer

const float pi = 3.141592653589793;

float julia(vec3 pos) {
	vec2 aa; 
	float theta = atan(pos.x, pos.y); 
	float phi = atan(pos.z, pos.y); 
	vec2 a = vec2(theta/pi, phi/pi); 
	vec2 c = vec2(-0.8f + sin(1.0f*time / 8000.0f)/300.0f, 0.156f + cos(1.0f*time / 8000.0f)/300.0f);
	
	for (int i = 0; i < 200; i++) { 
		aa = vec2(a.x*a.x-a.y*a.y+c.x, 2*a.x*a.y+c.y); 
		a = aa; 
	}

	a = fract(aa); 
	return a.x;
}

vec3 mapColor(float value) {
	vec3 color; 
	
	value = clamp(value, 0.0f, 1.0f);

	if (value < 0.2f) 
		color = vec3 (value * 2, 0.5f, 0.5f); 
	else if (value < 0.4f) 
		color = vec3 (1.0f - (value - 0.2f) * 5, (value - 0.2f) * 5, 0.0f); 
	else if (value < 0.6f) 
		color = vec3 (0.2f, 1.0f - (value - 0.4f) * 5, (value - 0.4f) * 5); 
	else if (value < 0.8f) 
		color = vec3 ((value - 0.6f) * 5, 0.0f, 1.0f); 
	else 
		color = vec3 (1.0f - (value - 0.8f) * 5 + 0.5f, 0.5f, 1.0f - (value - 0.8f) * 5 + 0.5f); 
		
	return clamp(color, 0.0f, 1.0f);
}

void main (void) {
	// Getting the fragment color from the procedural texture mapping
	float ret = julia(fragmentTextureCoord);
	vec3 mapped = mapColor(ret);
	vec4 textureColor = vec4(mapped, texture(textureImage, fragmentTextureCoord).w);

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
		fragmentColor += intensity * textureColor * fragmentDiffuse * NdotL;

		// Specular light part of Phong lighting: I_s * k_s * (V * R)
		// (2 * dot(-lightDirection, fragmentNormal) * fragmentNormal) + lightDirection;
		vec3 r = reflect(-lightDirection, fragmentNormal);
		vec3 v = normalize(fragmentPosition.xyz);

		VdotR = dot(v, r);
		VdotR = max(pow(VdotR, material.specularity), 0.0);

		fragmentColor += intensity * fragmentSpecular * VdotR;
	}
}