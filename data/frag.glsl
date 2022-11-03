#version 460

in vec3 normal;
in vec3 worldPosition;

out vec4 FragColor;
uniform vec4 u_baseColorFactor;
uniform vec4 u_metallicFactor;
uniform vec4 u_roughnessFactor;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main(void)
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);



    // vec3 viewPos = vec3(0.0, 0.0, 0.0);
    // vec3 lightPos = vec3(0.0, 0.0, 3.0);
    // vec3 lightDir = normalize(lightPos - worldPosition);
    // vec3 normalized = normalize(normal);

    // // Ambient lighning
    // vec4 ambient = vec4(0.8 * u_baseColorFactor.rgb, 1.0);
    // // Diffuse lightning
    // float diff = max(dot(lightDir, normalized), 0.0);
    // vec4 diffuse = vec4(diff * u_baseColorFactor.rgb, 1.0);
    // // Specular lightning
    // vec3 viewDir = normalize(viewPos - u_baseColorFactor.rgb);
    // vec3 reflectDir = reflect (-lightDir, normalized);
    // // Blinn-Phong
    // vec3 halfwayDir = normalize(lightDir + viewDir);
    // float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    // vec4 specular = vec4(vec3(0.3) * spec, 1.0);

    // // FragColor = vec4 (ambient.rgb + diffuse, 1);
    // FragColor = ambient + diffuse + specular;
}