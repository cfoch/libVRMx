#version 460

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 projection;

layout(location = 3) in vec3 in_position;
layout(location = 4) in vec3 in_normal;

out vec3 normal;
out vec3 worldPosition;


void main(void)
{
	normal = mat3(model) * in_normal;
	worldPosition = vec3(model * vec4(in_position, 1.0));

	gl_Position = projection * view * vec4(worldPosition, 1.0);
}