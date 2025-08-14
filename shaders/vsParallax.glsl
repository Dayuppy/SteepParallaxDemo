#version 330 core

layout (location = 0) in vec4 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec3 Normal;
//layout (location = 3) in vec4 Tangent; // No tangent in original Cg shader

out vec2 FragUV;
out vec3 tanEyeVec;
out vec3 tanLightVec;

uniform mat4 ModelViewProj;
uniform mat4 ModelViewI;
uniform vec3 lightPosition;

void main() {
    FragUV = UV;
    gl_Position = ModelViewProj * Position;

    // Eye position in object space
    vec4 eyePosition = ModelViewI * vec4(0,0,0,1);
    eyePosition /= eyePosition.w;

    // Light position in object space
    vec4 objectLightPosition = ModelViewI * vec4(lightPosition, 1.0f);
    objectLightPosition /= objectLightPosition.w;

    // For simple quad, tangent is X, normal is Z, binormal is Y
    vec3 tangent = vec3(1,0,0);
    vec3 normal = normalize(Normal);
    vec3 binormal = cross(normal, tangent);

    // Eye vector and light vector in object space
    vec3 eyeVec = normalize(eyePosition.xyz - Position.xyz);
    vec3 lightVec = normalize(objectLightPosition.xyz - Position.xyz);

    // Transform to tangent space
    mat3 TBN = mat3(tangent, binormal, normal);
    tanEyeVec = TBN * eyeVec;
    tanLightVec = TBN * lightVec;
}