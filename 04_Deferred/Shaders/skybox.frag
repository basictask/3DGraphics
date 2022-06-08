#version 330 core

// variables coming from the pipeline
in vec3 vs_out_pos;

out vec4 fs_out_col;

uniform samplerCube skyboxTexture;

void main()
{
	fs_out_col = texture(skyboxTexture, vs_out_pos);
}