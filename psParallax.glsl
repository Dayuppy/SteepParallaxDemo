#version 330 core

in vec2 FragUV;
in vec3 tanEyeVec;
in vec3 tanLightVec;

out vec4 fragColor;

uniform sampler2D diffuseTexture; // Renamed from texture
uniform sampler2D heightMap;
uniform sampler2D normalMap;
uniform float bumpScale;
uniform float parralax;

const float diffuseCoeff = 0.7;
const float specularCoeff = 0.6;

void main()
{
    vec2 texUV;

    // Compute the approximated texture coordinate displacement
    float height = texture(heightMap, FragUV).r;
    height = height * 2*bumpScale - bumpScale;
    vec3 tanEyeVecN = normalize(tanEyeVec);
    if(parralax > 0){
        texUV = FragUV + (tanEyeVecN.yx * height);
    } else {
        texUV = FragUV;
    }

    // Fetch the normal at this point
    vec3 normal = (texture(normalMap, texUV).rgb-0.5)*2;
    normal = normalize(normal);

    // extract the light vector in texture space
    vec3 tanLightVecN = normalize(tanLightVec);

    // Why is that required ??
    tanLightVecN.x = - tanLightVecN.x;

    // compute diffuse lighting
    float diffuse = max(dot(tanLightVecN, normal), 0.0) * diffuseCoeff;

    // compute specular lighting
    vec3 tanHalf = normalize(tanLightVecN+tanEyeVecN);
    float specular = max(dot(tanHalf, normal), 0.0);
    specular = pow(specular, 64.0) * specularCoeff;

    vec3 ambient = vec3(0.4, 0.4, 0.6)*1.4;

    vec3 texColor = texture(diffuseTexture, texUV).rgb; // Renamed texture

    // output final fragColor
    fragColor = vec4(texColor*(ambient + vec3(1.5, 1.5, 1.0)*0.7*diffuse)+ vec3(1.5, 1.5, 1.0)*0.7*specular, 1.0f);
}