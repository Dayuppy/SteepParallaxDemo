#version 330 core

// -----------------------------------------------------------------------------
// This fragment shader produces high‐quality surface detail by:
//   1. Parallax Occlusion Mapping (POM) with smooth interpolation to remove
//      visible “stepping” or banding in the parallax effect.
//   2. Reconstruction of a tangent‐space normal by blending:
//        • A normal map for fine micro‐detail.
//        • A finite‐difference derivative of the height map for broader
//          macro‐shape and silhouette accuracy.
//   3. Blinn‐Phong lighting model with a height‐based specular boost so that
//      peaks receive crisper, stronger highlights.
//   4. Screen‐space Ambient Occlusion (SSAO) computed from the height map,
//      giving crevices and recesses darker shading.
//   5. Self‐shadowing by ray‐marching the height map along the light direction,
//      softened with a small PCF kernel for penumbral blur.
// Together, these techniques give the illusion of real geometry at very low
// tessellation cost, while minimizing artifacts like banding or aliasing.
// -----------------------------------------------------------------------------

in vec2 FragUV;        // The original UV coordinates passed from the vertex shader,
//                     // in tangent‐space, used to lookup all textures.
in vec3 tanEyeVec;     // View vector in tangent‐space: points from surface to camera.
in vec3 tanLightVec;   // Light vector in tangent‐space: points from surface to light.

out vec4 fragColor;    // The final computed color for this fragment (RGBA).

// -----------------------------------------------------------------------------

// Samplers for the three input textures, all sampled with the same final UV:
//  • diffuseTexture: the base color (albedo) of the material.
//  • heightMap: grayscale height map defining surface relief.
//  • normalMap: artist‐painted normal map encoding fine surface bumps.
uniform sampler2D diffuseTexture;
uniform sampler2D heightMap;
uniform sampler2D normalMap;

// Control flags and scales:
//  • selfShadowTest: if >0, enable self‐shadowing; if 0, skip that pass.
//  • bumpScale: global multiplier for how “tall” the parallax relief appears.
uniform float selfShadowTest;
uniform float bumpScale;

// Lighting coefficients:
//  • diffuseCoeff: fraction of light contributing to Lambertian diffuse.
//  • baseSpecularCoeff: base multiplier for the Blinn‐Phong specular term.
const float diffuseCoeff      = 0.7;
const float baseSpecularCoeff = 0.6;

// Minimum limits to prevent full black in AO or shadows:
//  • aoMin: darkest ambient occlusion (0=no darkening, 1=full dark).
//  • shadowMin: darkest self‐shadow (0=pure black, 1=no shadow).
const float aoMin     = 0.08;
const float shadowMin = 0.1;

// SSAO sampling parameters:
//  • AO_SAMPLES: number of directions to sample around the fragment.
//  • AO_RADIUS: maximum UV offset in each direction for occlusion sampling.
const int   AO_SAMPLES = 8;
const float AO_RADIUS  = 0.012;

// -----------------------------------------------------------------------------
// Parallax Occlusion Mapping function with continuous intersection interpolation.
// Instead of stopping exactly on a discrete step, we linearly interpolate
// between the last two samples to approximate the true surface intersection.
// This removes the “stepped” look when the camera or object moves.
// -----------------------------------------------------------------------------
vec2 parallaxTrace(
    sampler2D heightMap, // height data source
    vec2 uv,             // starting UV coordinates
    vec3 viewDir,        // normalized view direction in tangent‐space
    float bumpScale      // controls apparent depth
) {
    // 1) Determine how many linear steps to march based on viewing angle.
    //    We use more steps when the surface is nearly edge‐on (small viewDir.z)
    //    because parallax error is more obvious there.
    //    mix(72,36, |viewDir.z| ) smoothly blends between 72 steps (edge‐on)
    //    and 36 (face‐on).
    float numSteps = mix(72.0, 36.0, abs(viewDir.z));

    // 2) steepScale allows us to exaggerate or dampen the relief.
    //    A value >1 makes hills taller; =1 is a 1:1 mapping.
    float steepScale = 1.0;

    // 3) Compute the incremental UV step per iteration:
    //    – We step opposite the view direction (hence the minus).
    //    – viewDir.yx: swap XY so that view.x shifts V in Y, view.y shifts U in X.
    //    – bumpScale * steepScale scales the total parallax offset.
    //    – Dividing by abs(viewDir.z) ensures a constant silhouette thickness
    //      regardless of angle.
    //    – Divide by numSteps to break the total offset into per‐step increments.
    vec2 deltaUV = -viewDir.yx * bumpScale * steepScale
                   / (abs(viewDir.z) * numSteps);

    // 4) Each step reduces a normalized “heightRemaining” by this amount.
    //    We start at 1.0 (the top of the height map) and march down.
    float deltaH = 1.0 / numSteps;

    // 5) Initialize current UV, heightRemaining, and fetch the first sample.
    vec2  curUV      = uv;
    float heightRem  = 1.0;
    float curSample  = texture(heightMap, curUV).r;

    // 6) Store the previous sample and UV so we can interpolate later.
    vec2  prevUV;
    float prevSample = heightRem; // initial “previous” height = top

    // 7) Linear search: march forward until the sampled height
    //    is greater than or equal to the remaining ray height.
    //    That indicates we have stepped “through” the surface.
    while (heightRem > 0.0 && curSample < heightRem) {
        heightRem   -= deltaH;    // step down in height
        prevUV       = curUV;     // store previous position
        prevSample   = curSample; // store previous sample value
        curUV       += deltaUV;   // advance UV
        curSample    = texture(heightMap, curUV).r;  // sample height
    }

    // 8) At this point, curSample >= heightRem, so we’ve gone one step too far.
    //    To get a smooth transition, we linearly interpolate between prevUV and
    //    curUV based on how far each sample overshoots the intersection.
    float afterDepth  = curSample  - heightRem;         // overshoot on current
    float beforeDepth = prevSample - (heightRem + deltaH); // overshoot on previous

    //  t = fraction along the segment [prevUV→curUV] that best approximates the
    //      true intersection.
    float t = clamp(beforeDepth / (beforeDepth + afterDepth), 0.0, 1.0);
    vec2 finalUV = mix(prevUV, curUV, t);

    return finalUV;
}

// -----------------------------------------------------------------------------
// Compute a tangent‐space normal from the height map by finite differences.
// This gives us a broad‐scale normal for macro relief that blends with the
// fine micro‐detail from the normal map.
// -----------------------------------------------------------------------------
vec3 computeHeightNormalTS(
    sampler2D heightMap, // height data
    vec2 uv,             // UV at which to compute derivative
    vec2 texelSize,      // inverse texture dimensions: (1/width,1/height)
    float bumpScale      // how strongly slopes are scaled
) {
    // Sample center, right, and up heights
    float hc = texture(heightMap, uv).r;
    float hr = texture(heightMap, uv + vec2(texelSize.x, 0.0)).r;
    float hu = texture(heightMap, uv + vec2(0.0, texelSize.y)).r;

    // Compute partial derivatives ∂h/∂x and ∂h/∂y
    float dx = (hr - hc) * bumpScale;
    float dy = (hu - hc) * bumpScale;

    // Construct a normal whose XY components are the negative slope
    // and Z = 1.0 ensures it points mostly upwards.
    return normalize(vec3(-dx, -dy, 1.0));
}

// -----------------------------------------------------------------------------
float saturate(float x) { return clamp(x, 0.0, 1.0); }       // clamp to [0,1]
float lerp(float a, float b, float t) { return a + t*(b - a); } // linear interp

// -----------------------------------------------------------------------------
// Main shading entry point
// -----------------------------------------------------------------------------
void main() {
    // 1) Define our light color (slightly yellow) and ambient fill color.
    vec3 lightColor  = vec3(1.0, 1.0, 0.65);
    vec3 ambientBase = vec3(0.4, 0.4, 0.6) * 1.4; // boost ambient slightly

    // 2) Normalize the incoming view vector in tangent space.
    vec3 tanEyeN = normalize(tanEyeVec);

    // 3) Compute parallax‐corrected UV coordinates.
    //    Removing step artifacts here is critical to a stable, smooth result.
    vec2 finalUV = parallaxTrace(
        heightMap, FragUV, tanEyeN, bumpScale
    );

    // 4) Sample the albedo at the displaced UV.
    vec3 albedo = texture(diffuseTexture, finalUV).rgb;

    // 5) Build two tangent‐space normals:
    //    a) nH from the height map derivative for macro shape.
    //    b) nM from the normal map for micro detail.
    vec2 texelSize = 1.0 / vec2(textureSize(heightMap, 0));
    vec3 nH = computeHeightNormalTS(
        heightMap, finalUV, texelSize, bumpScale
    );
    vec3 nM = normalize(
        texture(normalMap, finalUV).rgb * 2.0 - 1.0
    );

    // 6) Blend them 50/50 so neither macro nor micro detail is lost.
    vec3 N = normalize(mix(nM, nH, 0.5));

    // 7) Prepare the light vector (flip X to match UV handedness).
    vec3 tanLightN = normalize(tanLightVec);
    tanLightN.x = -tanLightN.x;

    // 8) Compute standard Lambertian diffuse term.
    float NdotL = max(dot(N, tanLightN), 0.0);

    // 9) Compute Blinn‐Phong half‐vector and specular term.
    vec3 halfVec = normalize(tanLightN + tanEyeN);
    float NdotH  = max(dot(N, halfVec), 0.0);

    // 10) Height‐based specular boost:
    //     • Higher height (peaks) get sharper, stronger highlights.
    //     • We raise height to 2.5 so the boost is concentrated near peaks.
    float hVal     = texture(heightMap, finalUV).r;
    float boost    = lerp(0.9, 2.5, pow(hVal, 2.5));
    float expo     = lerp(32.0, 96.0, hVal);
    float specular = pow(NdotH, expo) * baseSpecularCoeff * boost;

    // -------------------------------------------------------------------------
    // 11) Ambient Occlusion: sample surrounding heights to darken crevices.
    // -------------------------------------------------------------------------
    float ao = 1.0;
    {
        float sumAO = 0.0;
        for (int i = 0; i < AO_SAMPLES; ++i) {
            // Spread samples in a circle
            float ang = 6.2831853 * float(i) / float(AO_SAMPLES);
            vec2 dir = vec2(cos(ang), sin(ang));
            vec2 uvS = finalUV + dir * AO_RADIUS;

            // Compute raw occlusion by height difference
            float neighborH = texture(heightMap, uvS).r;
            float rawAO     = saturate(hVal - neighborH + 0.03);

            // Modulate by normal similarity for smoother transitions
            vec3 nS = normalize(
                texture(normalMap, uvS).rgb * 2.0 - 1.0
            );
            rawAO *= (0.4 + 0.6 * max(dot(N, nS), 0.0));

            sumAO += rawAO;
        }
        ao = sumAO / float(AO_SAMPLES);

        // Flatten AO where surface is close to orthogonal to view
        float flatten = abs(N.z);
        ao = lerp(ao, 1.0, flatten);

        // Clamp between minimum occlusion and fully lit
        ao = lerp(aoMin, 1.0, ao);
    }

    // -------------------------------------------------------------------------
    // 12) Self‐Shadowing: ray march along light direction through height map.
    //     We blur with a 3×3 PCF kernel for penumbra softness.
    // -------------------------------------------------------------------------
    float shadow = 1.0;
    if (selfShadowTest > 0.0 && NdotL > 0.0) {
        int numShadowSteps = int(lerp(48.0, 12.0, abs(tanLightN.z)));
        float steepScale = 1.0; // MATCH parallax!
        float shadowDeltaH = 1.0 / float(numShadowSteps);
        vec2 shadowDeltaUV = tanLightN.yx * bumpScale * steepScale / (abs(tanLightN.z) * float(numShadowSteps));

        float shadowSum = 0.0;
        int pcfRings = 3;
        int pcfSamples = 4;
        for (int dx = -pcfRings; dx <= pcfRings; ++dx)
        for (int dy = -pcfRings; dy <= pcfRings; ++dy)
        {
            vec2 pcfOffset = vec2(dx, dy) * 0.0015;
            vec2 shadowUV = finalUV + pcfOffset;
            float shadowHeight = texture(heightMap, finalUV).r + shadowDeltaH * 0.1;
            bool inShadow = false;
            for (int i = 0; i < numShadowSteps && shadowHeight < 1.0; ++i) {
                float testHeight = texture(heightMap, shadowUV).r;
                if (testHeight > shadowHeight) {
                    inShadow = true;
                    break;
                }
                shadowHeight += shadowDeltaH;
                shadowUV += shadowDeltaUV;
            }
            shadowSum += inShadow ? shadowMin : 1.0;
            pcfSamples++;
        }
        shadow = shadowSum / float(pcfSamples);
    }

    // -------------------------------------------------------------------------
    // 13) Final composite: combine albedo with ambient + diffuse,
    //     then add specular, each modulated by AO and self‐shadow.
    // -------------------------------------------------------------------------
    vec3 color = albedo * (
                      ambientBase * ao
                    + lightColor * diffuseCoeff * NdotL * shadow
                  )
               + lightColor * specular * shadow;

    fragColor = vec4(color, 1.0);
}