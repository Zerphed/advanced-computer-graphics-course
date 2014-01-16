#version 330

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform int time;
uniform int throbbing;

layout(location=0) in vec4 vertexPosition;
layout(location=1) in vec4 vertexColor;
layout(location=2) in vec3 vertexNormal;

out vec3 fragmentTextureCoord;
out vec3 fragmentNormal;
out vec4 fragmentPosition;

void main(void)
{   
	float offset;
	if (throbbing == 1)
		offset = (sin((1.0f*time)/1000.0f + sin(vertexPosition.x) * 5+sin(vertexPosition.y*10)*3 + sin(5*vertexPosition.z) ) / 3.0f) + 1.0f;
	else
		offset = 1.0f;


	vec4 scaledVertexPosition = vec4(vertexPosition.x * offset, vertexPosition.y * offset, vertexPosition.z * offset, vertexPosition.w);

	fragmentPosition = viewMatrix * modelMatrix * scaledVertexPosition;

	// This won't do if model is ununiformly scaled, in that case: http://www.songho.ca/opengl/gl_normaltransform.html
	fragmentNormal = normalize((viewMatrix * modelMatrix * vec4(vertexNormal, 0.0f)).xyz); // OUT

	// Cube mapping of the texture to the model
	fragmentTextureCoord = scaledVertexPosition.xyz; // OUT

	// Reflection model
	// fragmentTextureCoord = reflect((viewMatrix * modelMatrix * scaledVertexPosition).xyz, vertexNormal);

	gl_Position = (projectionMatrix * viewMatrix * modelMatrix) * scaledVertexPosition;
}