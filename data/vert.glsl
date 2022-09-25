#version 460

layout(location = 0) uniform mat4 mvp;
layout(location = 1) in vec3 in_position;
layout(location = 2) in vec3 in_normal;

out vec3 normal;
out vec3 position;

void main(void)
{
	gl_Position = mvp * vec4(in_position, 1);
	position = in_position;
	normal = in_normal;
}