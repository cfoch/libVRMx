#version 460

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform bool u_ignoreNormals;

in vec3 in_position;
in vec3 in_normal;

out vec3 normal;
out vec3 worldPosition;

void main(void)
{
	worldPosition = vec3(u_model * vec4(in_position, 1.0));
	if (!u_ignoreNormals) {
		normal = normalize(transpose(inverse(mat3(u_model))) * in_normal);
	} else {
		normal = vec3(0, 0, 0);
	}

	gl_Position = u_projection * u_view * vec4(worldPosition, 1.0);
}