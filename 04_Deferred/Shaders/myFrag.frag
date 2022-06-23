#version 130

// per-fragment attributes coming from the pipeline
in vec3 vs_out_pos;
in vec3 vs_out_normal;
in vec2 vs_out_tex0;
in vec4 vs_out_lightspace_pos;

// output value - the color of the fragment
out vec4 fs_out_col;

// scene properties
uniform vec3 eye_pos = vec3(0, 15, 15);

// light properties
uniform vec3 toLight = normalize(vec3( 0, 20, 20 ));

uniform vec4 La = vec4(0.1f, 0.1f, 0.1f, 1);
uniform vec4 Ld = vec4(0.75f, 0.75f, 0.75f, 1);
uniform vec4 Ls = vec4(1, 1, 1, 1);

// material properties
uniform vec4 Ka = vec4(1, 1, 1, 0);
uniform vec4 Kd = vec4(0.75f, 0.25f, 0.125f, 1);
uniform vec4 Ks = vec4(0, 1, 0, 0);
uniform float specular_power = 32;
uniform sampler2D texImage;
uniform sampler2D textureShadow;


void main()
{
	// Ambient
	vec4 ambient = La * Ka;

	// Diffuse
	vec3 normal = normalize( vs_out_normal );
	float di = clamp( dot( toLight, normal), 0.0f, 1.0f );
	vec4 diffuse = vec4(Ld.rgb*Kd.rgb*di, Kd.a);

	// Specular
	vec4 specular = vec4(0);

	// Shadow calculation
	if ( di > 0 )
	{
		vec3 e = normalize( eye_pos - vs_out_pos );
		vec3 r = reflect( -toLight, normal );
		float si = pow( clamp( dot(e, r), 0.0f, 1.0f ), specular_power );
		specular = Ls*Ks*si;
	}
	vec4 col = (ambient + diffuse + specular ) * texture(texImage, vs_out_tex0.st).rgba;

	vec3 lightcoords = (0.5*vs_out_lightspace_pos.xyz+0.5) / vs_out_lightspace_pos.w;
	vec2 lightuv = 0.5 * lightcoords.xy + 0.5;
	float bias = 0.001;
	
	if ( lightuv == clamp(lightuv,0,1))
	{
		float nearestToLight = texture(textureShadow, lightuv).x;

		if ( nearestToLight + bias >= lightcoords.z )
			fs_out_col = col;
		else
			fs_out_col = ambient*0.5;
	}
	else
		fs_out_col = vec4(1,0,0,1);
}