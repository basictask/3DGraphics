#version 400

// incoming vertex attributes from the VBO via the VAO
// now with explicit location!
layout(location=0) in vec3 vs_in_pos;
layout(location=1) in vec3 vs_in_normal;
layout(location=2) in vec2 vs_in_tex0;

// values that are forwarded on the pipeline
out vec3 vs_out_pos;
out vec3 vs_out_normal;
out vec2 vs_out_tex0;

// transformation this shader need to perform
uniform mat4 world;
uniform mat4 worldIT;
uniform mat4 MVP;

void main()
{
	gl_Position   = MVP    * vec4( vs_in_pos, 1 );
	vs_out_pos    = (world * vec4( vs_in_pos, 1 )).xyz;
	vs_out_normal = (worldIT * vec4(vs_in_normal, 0)).xyz;
	vs_out_tex0   = vs_in_tex0;
}