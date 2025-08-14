#version 330 core

// PBR-focused Fragment Shader
// Implements physically based rendering with advanced material properties

in vec2 FragUV;
in vec3 tanEyeVec;
in vec3 tanLightVec;
in vec3 worldPos;
in vec3 worldNormal;

out vec4 fragColor;

// PBR Textures
uniform sampler2D diffuseTexture;    // Albedo
uniform sampler2D normalMap;         // Normal map
uniform sampler2D roughnessMap;      // Roughness
uniform sampler2D metallicMap;       // Metallic
uniform sampler2D aoMap;             // Ambient occlusion
uniform sampler2D heightMap;         // Height/displacement

// Lighting
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 cameraPosition;

// Material properties
uniform float roughness;
uniform float metallic;
uniform float bumpScale;
uniform bool enableParallax;

// PBR Constants
const float PI = 3.14159265359;

// Utility functions
float saturate(float x) { return clamp(x, 0.0, 1.0); }

// Advanced parallax mapping with binary search refinement
vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    if (!enableParallax) return texCoords;
    
    // Initial steep parallax mapping
    const float minLayers = 8.0;
    const float maxLayers = 64.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy / viewDir.z * bumpScale;
    vec2 deltaTexCoords = P / numLayers;
    
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(heightMap, currentTexCoords).r;
    
    // Linear search
    while(currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(heightMap, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }
    
    // Binary search refinement for smoother results
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float nextDepth = currentDepthMapValue - currentLayerDepth;
    float prevDepth = texture(heightMap, prevTexCoords).r - currentLayerDepth + layerDepth;
    
    float weight = nextDepth / (nextDepth - prevDepth);
    return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// Geometry function (Smith's method)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel equation (Schlick's approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Multiple importance sampling for environment lighting
vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    
    // Spherical to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // Tangent-space to world-space
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

void main() {
    // Normalize tangent space vectors
    vec3 viewDir = normalize(tanEyeVec);
    vec3 lightDir = normalize(tanLightVec);
    lightDir.x = -lightDir.x; // Correct handedness
    
    // Apply parallax mapping
    vec2 texCoords = parallaxMapping(FragUV, viewDir);
    
    // Sample material properties
    vec3 albedo = texture(diffuseTexture, texCoords).rgb;
    vec3 normal = normalize(texture(normalMap, texCoords).rgb * 2.0 - 1.0);
    float rough = texture(roughnessMap, texCoords).r * roughness;
    float metal = texture(metallicMap, texCoords).r * metallic;
    float ao = texture(aoMap, texCoords).r;
    
    // Ensure minimum roughness to avoid numerical issues
    rough = max(rough, 0.04);
    
    // Calculate vectors
    vec3 N = normalize(normal);
    vec3 V = normalize(viewDir);
    vec3 L = normalize(lightDir);
    vec3 H = normalize(L + V);
    
    // Calculate dot products
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    // Calculate F0 (surface reflection at zero incidence)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metal);
    
    // Cook-Torrance BRDF
    float D = DistributionGGX(N, H, rough);
    float G = GeometrySmith(N, V, L, rough);
    vec3 F = fresnelSchlick(VdotH, F0);
    
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    
    // Calculate diffuse component
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metal; // Metallic surfaces have no diffuse
    
    vec3 diffuse = kD * albedo / PI;
    
    // Combine lighting
    vec3 radiance = lightColor * 3.0; // Boost light intensity
    vec3 Lo = (diffuse + specular) * radiance * NdotL;
    
    // Simple ambient lighting (could be IBL)
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    
    // HDR tonemapping (ACES)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    fragColor = vec4(color, 1.0);
}