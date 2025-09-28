#version 460
#pragma optimize (off)

const float ao = 1.0;
const float PI = 3.14159265359;

out vec4 FragColor;
in vec3 normal;
in vec3 worldPosition;
in vec2 texCoord;

uniform bool u_ignoreNormals;
uniform vec3 u_cameraPosition;
uniform vec3 u_lightColors[4];
uniform vec3 u_lightPositions[4];
uniform uint u_lightCount;

uniform sampler2D u_baseColorTexture;
uniform vec4 u_baseColorFactor;
uniform float u_metallicFactor;
uniform float u_roughnessFactor;

float DistributionGGX(vec3 N, vec3 H, float u_roughnessFactor)
{
    float a = u_roughnessFactor*u_roughnessFactor;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float u_roughnessFactor)
{
    float r = (u_roughnessFactor + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float u_roughnessFactor)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, u_roughnessFactor);
    float ggx1 = GeometrySchlickGGX(NdotL, u_roughnessFactor);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main(void)
{
    vec3 N;
    if (!u_ignoreNormals) {
        N = normalize(normal);
    } else {
        vec3 dpdx = dFdx(worldPosition);
        vec3 dpdy = dFdy(worldPosition);
        N = normalize(cross(dpdx, dpdy));
    }

    vec3 V = normalize(u_cameraPosition - worldPosition);

    vec4 baseColor = texture(u_baseColorTexture, texCoord);
    vec3 albedo = baseColor.rgb * u_baseColorFactor.rgb;
    // vec3 albedo = vec3(0.5f, 0.0f, 0.0f);
    float metallic = u_metallicFactor;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < u_lightCount; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(u_lightPositions[i] - worldPosition);
        vec3 H = normalize(V + L);
        float distance = length(u_lightPositions[i] - worldPosition);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = u_lightColors[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, u_roughnessFactor);   
        float G   = GeometrySmith(N, V, L, u_roughnessFactor);      
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}