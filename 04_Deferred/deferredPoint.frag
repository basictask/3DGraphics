#version 130

in vec2 vs_out_tex;

out vec4 fs_out_col;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D positionTexture;

uniform vec3 lightPos = vec3(0,-1,0);
uniform vec4 Ld = vec4(0.5,0.5,0.5,1);

void main()
{
	vec3 pos = texture(positionTexture, vs_out_tex).rgb;

	vec3 lightDir = normalize(lightPos - pos);
	float dist = distance(lightPos, pos);

	vec4 Kd = texture( diffuseTexture, vs_out_tex );
	vec3 n = normalize( texture( normalTexture, vs_out_tex ).rgb );

	fs_out_col = Ld*Kd*clamp(dot(n, lightDir), 0, 1) / (dist * dist);
}