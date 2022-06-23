#version 330 core

// Inputs
in vec2 UV;
in vec4 ShadowCoord;

// Ouputs
layout(location = 0) out vec3 color;

// Values
uniform sampler2D myTextureSampler;
uniform sampler2DShadow shadowMap;

void main()
{
	vec3 LightColor = vec3(0,0,0); // No color for the light
	
	vec3 MaterialDiffuseColor = texture(myTextureSampler, UV).rgb;

	float visibility = texture(shadowMap, vec3(ShadowCoord.xy, (ShadowCoord.z)/ShadowCoord.w));

	color = visibility * MaterialDiffuseColor * LightColor;
}