#version 330 core

// Inputs
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

// Outputs
out vec2 UV;
out vec4 ShadowCoord;

// Values
uniform mat4 MVP;
uniform mat4 DepthBiasMVP;

void main()
{
	gl_Position =  MVP * vec4(vertexPosition_modelspace,1);
	ShadowCoord = DepthBiasMVP * vec4(vertexPosition_modelspace,1);	
	UV = vertexUV;
}
