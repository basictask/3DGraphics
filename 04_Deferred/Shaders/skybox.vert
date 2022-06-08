#version 400

// variables coming from the VBO
layout(location = 0) in vec3 vs_in_pos;

// variables going forward through the pipeline
out vec3 vs_out_pos;

// external parameters of the shader
uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(vs_in_pos, 0).xyzw;	// [x,y,w,w] => after homogeneous division [x/w, y/w, 1]

	vs_out_pos = vs_in_pos;
}