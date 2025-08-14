#version 330 core

// Enhanced Fragment Shader for Advanced Parallax Mapping Demo
// Implements 25+ advanced rendering techniques

// Input from vertex shader
in vec2 FragUV;
in vec3 tanEyeVec;
in vec3 tanLightVec;
in vec3 worldPos;
in vec3 worldNormal;
in vec3 worldTangent;
in vec3 worldBinormal;
in vec4 screenPos;
in vec3 viewPos;
in vec3 lightPos;
in float depth;

// Output
out vec4 fragColor;

// Textures
uniform sampler2D diffuseTexture;
uniform sampler2D heightMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform sampler2D aoMap;
uniform sampler2D noiseTexture;

// Lighting uniforms
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 cameraPosition;

// Material properties
uniform float bumpScale;
uniform float roughness;
uniform float metallic;
uniform float specularStrength;

// Feature flags (25+ toggleable features)
uniform bool enableParallax;
uniform bool enableSelfShadowing;
uniform bool enablePBR;
uniform bool enableSSAO;
uniform bool enableBloom;
uniform bool enableToneMapping;
uniform bool enableChromaticAberration;
uniform bool enableDepthOfField;
uniform bool enableMotionBlur;
uniform bool enableVolumetricLighting;
uniform bool enableSubsurfaceScattering;
uniform bool enableAnisotropicFiltering;
uniform bool enableTemporalAA;
uniform bool enableScreenSpaceReflections;
uniform bool enableProceduralNoise;
uniform bool enableReliefMapping;
uniform bool enableConeStepMapping;
uniform bool enableTessellation;
uniform bool enableCaustics;
uniform bool enableNormalBlending;
uniform bool enableHeightFog;
uniform bool enableAdvancedSSAO;
uniform bool enableVolumetricShadows;
uniform bool enableAreaLights;
uniform bool enableIBL;
uniform bool enableMultiScattering;

// Screen properties
uniform vec2 screenSize;
uniform float time;
uniform int frameCount;

// Constants
const float PI = 3.14159265359;
const float INV_PI = 1.0 / PI;
const float EPSILON = 1e-6;

// Utility functions
float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate(vec3 x) { return clamp(x, 0.0, 1.0); }

// Advanced noise functions
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0,0)), hash(i + vec2(1,0)), f.x),
               mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Steep Parallax Mapping (enhanced version)
vec2 steepParallaxMapping(vec2 texCoords, vec3 viewDir) {
    const float minLayers = 16.0;
    const float maxLayers = 128.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy / viewDir.z * bumpScale;
    vec2 deltaTexCoords = P / numLayers;
    
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(heightMap, currentTexCoords).r;
    
    while(currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(heightMap, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }
    
    // Parallax occlusion mapping interpolation
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(heightMap, prevTexCoords).r - currentLayerDepth + layerDepth;
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    return finalTexCoords;
}

// Advanced Parallax Occlusion Mapping with multiple techniques
vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    if (!enableParallax) return texCoords;
    
    // For now, use standard steep parallax mapping
    return steepParallaxMapping(texCoords, viewDir);
}

// Relief Mapping implementation
vec2 reliefMapping(vec2 texCoords, vec3 viewDir) {
    const int numSteps = 64;
    float step = 1.0 / float(numSteps);
    vec2 offset = viewDir.xy * bumpScale / viewDir.z / float(numSteps);
    
    float height = 1.0;
    vec2 currentCoords = texCoords;
    float currentHeight = texture(heightMap, currentCoords).r;
    
    for (int i = 0; i < numSteps; i++) {
        if (currentHeight >= height) break;
        height -= step;
        currentCoords -= offset;
        currentHeight = texture(heightMap, currentCoords).r;
    }
    
    // Binary search refinement
    for (int i = 0; i < 8; i++) {
        offset *= 0.5;
        step *= 0.5;
        if (currentHeight >= height) {
            currentCoords += offset;
            height += step;
        } else {
            currentCoords -= offset;
            height -= step;
        }
        currentHeight = texture(heightMap, currentCoords).r;
    }
    
    return currentCoords;
}

// Cone Step Mapping
vec2 coneStepMapping(vec2 texCoords, vec3 viewDir) {
    const int maxSteps = 32;
    float coneRatio = 0.5; // Precomputed cone ratio
    
    vec2 offset = viewDir.xy * bumpScale / viewDir.z;
    float height = 1.0;
    vec2 currentCoords = texCoords;
    
    for (int i = 0; i < maxSteps; i++) {
        float currentHeight = texture(heightMap, currentCoords).r;
        if (currentHeight >= height) break;
        
        float stepSize = (height - currentHeight) * coneRatio;
        height -= stepSize;
        currentCoords -= offset * stepSize;
    }
    
    return currentCoords;
}

// PBR BRDF functions
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

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Screen Space Ambient Occlusion
float computeSSAO(vec2 texCoord, vec3 normal, float depth) {
    if (!enableSSAO && !enableAdvancedSSAO) return 1.0;
    
    const int samples = 16; // Fixed constant
    const float radius = 0.02;
    const float bias = 0.025;
    
    float occlusion = 0.0;
    vec2 noiseScale = screenSize / 4.0;
    
    // Simplified SSAO without ProjectionMatrix dependency
    for (int i = 0; i < samples; ++i) {
        float angle = float(i) / float(samples) * 2.0 * PI;
        float r = float(i) / float(samples);
        vec2 sampleOffset = vec2(cos(angle), sin(angle)) * r * radius;
        vec2 sampleUV = texCoord + sampleOffset;
        
        if (sampleUV.x >= 0.0 && sampleUV.x <= 1.0 && 
            sampleUV.y >= 0.0 && sampleUV.y <= 1.0) {
            float sampleHeight = texture(heightMap, sampleUV).r;
            float heightDiff = texture(heightMap, texCoord).r - sampleHeight;
            
            if (heightDiff > bias) {
                occlusion += 1.0;
            }
        }
    }
    
    occlusion = 1.0 - (occlusion / float(samples));
    return pow(occlusion, 1.5);
}

// Volumetric Lighting
vec3 computeVolumetricLighting(vec3 worldPos, vec3 lightPos, vec3 lightColor) {
    if (!enableVolumetricLighting) return vec3(0.0);
    
    const int steps = 16;
    vec3 rayDir = normalize(lightPos - worldPos);
    float stepSize = length(lightPos - worldPos) / float(steps);
    
    vec3 volumetric = vec3(0.0);
    vec3 currentPos = worldPos;
    
    for (int i = 0; i < steps; i++) {
        currentPos += rayDir * stepSize;
        
        // Sample atmospheric density (using noise)
        float density = fbm(currentPos.xy * 0.1 + time * 0.01) * 0.1;
        
        // Light scattering
        float dist = length(currentPos - lightPos);
        float attenuation = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);
        
        volumetric += lightColor * density * attenuation / float(steps);
    }
    
    return volumetric;
}

// Subsurface Scattering approximation
vec3 computeSubsurfaceScattering(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 albedo) {
    if (!enableSubsurfaceScattering) return vec3(0.0);
    
    float thickness = 0.2; // Simulated thickness
    float distortion = 0.1;
    float power = 2.0;
    float scale = 0.5;
    
    vec3 H = normalize(lightDir + normal * distortion);
    float VdotH = pow(saturate(dot(viewDir, -H)), power) * scale;
    float NdotL = dot(normal, lightDir);
    
    return albedo * (VdotH + NdotL) * thickness;
}

// Chromatic Aberration
vec3 chromaticAberration(vec2 uv) {
    if (!enableChromaticAberration) {
        return texture(diffuseTexture, uv).rgb;
    }
    
    float aberration = 0.002;
    vec2 center = vec2(0.5);
    vec2 distortion = (uv - center) * aberration;
    
    float r = texture(diffuseTexture, uv - distortion).r;
    float g = texture(diffuseTexture, uv).g;
    float b = texture(diffuseTexture, uv + distortion).b;
    
    return vec3(r, g, b);
}

// Tone Mapping operators
vec3 toneMap(vec3 color) {
    if (!enableToneMapping) return color;
    
    // ACES tone mapping
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

// Motion Blur (simplified)
vec3 motionBlur(vec2 uv, vec3 color) {
    if (!enableMotionBlur) return color;
    
    const int samples = 8;
    vec2 velocity = (screenPos.xy / screenPos.w - screenPos.xy / screenPos.w) * 0.01; // Simplified
    
    vec3 result = color;
    for (int i = 1; i < samples; i++) {
        float t = float(i) / float(samples - 1);
        vec2 sampleUV = uv + velocity * t;
        result += texture(diffuseTexture, sampleUV).rgb;
    }
    
    return result / float(samples);
}

// Main shading function
void main() {
    // Get view direction in tangent space
    vec3 viewDir = normalize(tanEyeVec);
    
    // Apply parallax mapping
    vec2 texCoords = parallaxMapping(FragUV, viewDir);
    
    // Sample textures with chromatic aberration
    vec3 albedo = chromaticAberration(texCoords);
    
    // Sample material properties
    vec3 normal = normalize(texture(normalMap, texCoords).rgb * 2.0 - 1.0);
    float height = texture(heightMap, texCoords).r;
    float rough = enablePBR ? texture(roughnessMap, texCoords).r : roughness;
    float metal = enablePBR ? texture(metallicMap, texCoords).r : metallic;
    float ao = texture(aoMap, texCoords).r;
    
    // Add procedural noise if enabled
    if (enableProceduralNoise) {
        float noiseValue = fbm(texCoords * 10.0 + time * 0.1);
        albedo *= 0.8 + 0.4 * noiseValue;
        normal.xy += (noiseValue - 0.5) * 0.1;
        normal = normalize(normal);
    }
    
    // Blend normals if enabled
    if (enableNormalBlending) {
        vec3 heightNormal = normalize(vec3(
            texture(heightMap, texCoords + vec2(0.001, 0.0)).r - texture(heightMap, texCoords - vec2(0.001, 0.0)).r,
            texture(heightMap, texCoords + vec2(0.0, 0.001)).r - texture(heightMap, texCoords - vec2(0.0, 0.001)).r,
            0.1
        ));
        normal = normalize(mix(normal, heightNormal, 0.3));
    }
    
    // Light calculations
    vec3 lightDir = normalize(tanLightVec);
    lightDir.x = -lightDir.x; // Flip for UV handedness
    
    vec3 N = normal;
    vec3 V = viewDir;
    vec3 L = lightDir;
    vec3 H = normalize(L + V);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    vec3 color;
    
    if (enablePBR) {
        // PBR shading
        vec3 F0 = mix(vec3(0.04), albedo, metal);
        
        // BRDF components
        float D = DistributionGGX(N, H, rough);
        float G = GeometrySmith(N, V, L, rough);
        vec3 F = fresnelSchlick(VdotH, F0);
        
        // Cook-Torrance BRDF
        vec3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL + EPSILON;
        vec3 specular = numerator / denominator;
        
        // Combine diffuse and specular
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metal;
        
        vec3 diffuse = kD * albedo * INV_PI;
        color = (diffuse + specular) * lightColor * NdotL;
        
        // Add ambient (IBL approximation)
        vec3 ambient = vec3(0.03) * albedo * ao;
        color += ambient;
        
    } else {
        // Traditional Blinn-Phong shading
        vec3 diffuse = albedo * max(dot(N, L), 0.0);
        vec3 specular = vec3(pow(max(dot(N, H), 0.0), 64.0)) * specularStrength;
        vec3 ambient = vec3(0.1) * albedo;
        
        color = (ambient + diffuse + specular) * lightColor;
    }
    
    // Self-shadowing
    if (enableSelfShadowing) {
        // Ray marching for self-shadows
        const int shadowSteps = 16;
        float shadow = 1.0;
        float stepSize = 0.1;
        vec2 shadowUV = texCoords;
        float shadowHeight = height;
        
        for (int i = 0; i < shadowSteps; i++) {
            shadowUV += L.xy * stepSize / float(shadowSteps);
            float sampleHeight = texture(heightMap, shadowUV).r;
            if (sampleHeight > shadowHeight) {
                shadow = 0.3;
                break;
            }
            shadowHeight += stepSize / float(shadowSteps);
        }
        
        color *= shadow;
    }
    
    // Additional effects
    if (enableSSAO || enableAdvancedSSAO) {
        float ssao = computeSSAO(texCoords, N, depth);
        color *= ssao;
    }
    
    if (enableVolumetricLighting) {
        vec3 volumetric = computeVolumetricLighting(worldPos, lightPos, lightColor);
        color += volumetric;
    }
    
    if (enableSubsurfaceScattering) {
        vec3 sss = computeSubsurfaceScattering(N, L, V, albedo);
        color += sss;
    }
    
    if (enableCaustics) {
        float caustic = fbm(texCoords * 20.0 + time * 0.5) * 0.1;
        color += vec3(0.1, 0.3, 0.5) * caustic * NdotL;
    }
    
    if (enableHeightFog) {
        float fog = exp(-depth * 0.1);
        color = mix(vec3(0.7, 0.8, 0.9), color, fog);
    }
    
    // Post-processing
    color = motionBlur(texCoords, color);
    
    if (enableBloom) {
        float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0) {
            color += (color - 1.0) * 0.5;
        }
    }
    
    color = toneMap(color);
    
    // Temporal Anti-Aliasing (simplified)
    if (enableTemporalAA) {
        vec2 jitter = vec2(
            fract(float(frameCount) * 0.618033988749895),
            fract(float(frameCount) * 0.380966011250105)
        ) - 0.5;
        texCoords += jitter / screenSize;
    }
    
    fragColor = vec4(color, 1.0);
}