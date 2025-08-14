#version 330 core

// Enhanced Vertex Shader for Advanced Parallax Mapping Demo
// Supports PBR, multiple lights, and advanced effects

layout (location = 0) in vec4 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec3 Normal;
layout (location = 3) in vec4 Tangent;

// Output to fragment shader
out vec2 FragUV;
out vec3 tanEyeVec;
out vec3 tanLightVec;
out vec3 worldPos;
out vec3 worldNormal;
out vec3 worldTangent;
out vec3 worldBinormal;
out vec4 screenPos;
out vec3 viewPos;
out vec3 lightPos;
out float depth;

// Matrices
uniform mat4 ModelViewProj;
uniform mat4 ModelViewI;
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

// Lighting
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

// Time for animation effects
uniform float time;

// Feature flags
uniform float tessellationLevel;
uniform float displacementScale;
uniform bool enableProcNoise;
uniform bool enableMotionVectors;

// Procedural noise function
float noise(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 54.53))) * 43758.5453);
}

void main() {
    FragUV = UV;
    
    // Transform position to world space
    vec4 worldPosition = ModelMatrix * Position;
    worldPos = worldPosition.xyz;
    
    // Transform normals to world space
    mat3 normalMatrix = mat3(ModelMatrix);
    worldNormal = normalize(normalMatrix * Normal);
    worldTangent = normalize(normalMatrix * Tangent.xyz);
    worldBinormal = cross(worldNormal, worldTangent) * Tangent.w;
    
    // Calculate view position
    viewPos = (ViewMatrix * worldPosition).xyz;
    
    // Calculate screen position for post-processing effects
    vec4 clipPos = ModelViewProj * Position;
    screenPos = clipPos;
    depth = clipPos.z / clipPos.w;
    
    // Eye position in object space (for parallax calculations)
    vec4 eyePosition = ModelViewI * vec4(0,0,0,1);
    eyePosition /= eyePosition.w;
    
    // Light position in object space
    vec4 objectLightPosition = ModelViewI * vec4(lightPosition, 1.0);
    objectLightPosition /= objectLightPosition.w;
    lightPos = objectLightPosition.xyz;
    
    // Calculate tangent space vectors for parallax mapping
    vec3 tangent = worldTangent;
    vec3 normal = worldNormal;
    vec3 binormal = worldBinormal;
    
    // Eye and light vectors in object space
    vec3 eyeVec = normalize(eyePosition.xyz - Position.xyz);
    vec3 lightVec = normalize(objectLightPosition.xyz - Position.xyz);
    
    // Transform to tangent space
    mat3 TBN = mat3(tangent, binormal, normal);
    tanEyeVec = TBN * eyeVec;
    tanLightVec = TBN * lightVec;
    
    // Procedural vertex displacement for advanced effects
    vec4 finalPosition = Position;
    if (enableProcNoise) {
        float noiseValue = noise(Position.xyz * 0.1 + time * 0.1);
        finalPosition.xyz += Normal * noiseValue * displacementScale * 0.1;
    }
    
    // Apply tessellation displacement if enabled
    if (tessellationLevel > 1.0) {
        // Simulate tessellation displacement
        float tessNoise = noise(Position.xyz * tessellationLevel);
        finalPosition.xyz += Normal * tessNoise * displacementScale * 0.05;
    }
    
    gl_Position = ModelViewProj * finalPosition;
}