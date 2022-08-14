#version 460

layout(location = 0) in vec3 in_vertex;
layout(location = 1) uniform mat4 mvp;

void main(void)
{
	gl_Position = mvp * vec4(in_vertex, 1);
}