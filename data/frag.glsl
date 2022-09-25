#version 460

in vec3 normal;
in vec3 position;

out vec4 FragColor;
uniform vec4 u_baseColorFactor;
uniform vec4 u_metallicFactor;
uniform vec4 u_roughnessFactor;

void main(void)
{
    vec3 viewPos = vec3(0.0, 0.0, 0.0);
    vec3 lightPos = vec3(0.0, 0.0, 3.0);
    vec3 lightDir = normalize(lightPos - position);
    vec3 normalized = normalize(normal);

    // Ambient lighning
    vec4 ambient = vec4(0.8 * u_baseColorFactor.rgb, 1.0);
    // Diffuse lightning
    float diff = max(dot(lightDir, normalized), 0.0);
    vec4 diffuse = vec4(diff * u_baseColorFactor.rgb, 1.0);
    // Specular lightning
    vec3 viewDir = normalize(viewPos - u_baseColorFactor.rgb);
    vec3 reflectDir = reflect (-lightDir, normalized);
    // Blinn-Phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec4 specular = vec4(vec3(0.3) * spec, 1.0);

    // FragColor = vec4 (ambient.rgb + diffuse, 1);
    FragColor = ambient + diffuse + specular;
}