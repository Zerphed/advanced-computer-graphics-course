#version 330

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform int time;
uniform int throbbing;

layout(location=0) in vec4 vertexPosition;
layout(location=1) in vec4 vertexColor;
layout(location=2) in vec3 vertexNormal;
layout(location=3) in vec2 vertexUV;
layout(location=4) in vec4 vertexTangent;
layout(location=5) in vec4 vertexBinormal;

out vec3 fragmentTextureCoord;
out vec3 fragmentNormal;
out vec4 fragmentPosition;
out mat4 tangentMatrix;

void main(void)
{   
	float offset;
	if (throbbing == 1)
		//offset = (sin((1.0f*time) / 1000.0f + vertexPosition.x * 5) / 3.0f) + 1.0f;
		offset = (sin((1.0f*time)/1000.0f + sin(vertexPosition.x) * 5+sin(vertexPosition.y*10)*3 + sin(5*vertexPosition.z) ) / 3.0f) + 1.0f;
	else
		offset = 1.0f;

	// The PLY data has some problems, so there is need to clamp the tangent data
	vec4 vertexTangentClamped;
	if (dot(vertexTangent, vertexTangent) < 0.000001f)
		vertexTangentClamped = vec4(1.0f, 0.0f, 0.0f, 0.0f);
	else
		vertexTangentClamped = vertexTangent;

	vec4 scaledVertexPosition = vec4(vertexPosition.x * offset, vertexPosition.y * offset, vertexPosition.z * offset, vertexPosition.w);

	fragmentPosition = viewMatrix * modelMatrix * scaledVertexPosition;

	vec4 n = normalize(modelMatrix * vec4(vertexNormal, 0.0f));
	vec4 t = normalize(modelMatrix * vec4(vertexTangentClamped.xyz, 0.0f));
	vec4 b = vec4(cross(n.xyz, t.xyz), 0.0f);

	mat4 tangentMatrix = mat4(n, b, t, vec4(0.0f));
	tangentMatrix = transpose(tangentMatrix);

	// This won't do if model is ununiformly scaled, in that case: http://www.songho.ca/opengl/gl_normaltransform.html
	fragmentNormal = normalize((viewMatrix * modelMatrix * vec4(vertexNormal, 0.0f)).xyz); // OUT

	// Cube mapping of the texture to the model
	fragmentTextureCoord = scaledVertexPosition.xyz; // OUT

	// Reflection model
	// fragmentTextureCoord = reflect((viewMatrix * modelMatrix * scaledVertexPosition).xyz, vertexNormal);

	gl_Position = (projectionMatrix * viewMatrix * modelMatrix) * scaledVertexPosition;
}