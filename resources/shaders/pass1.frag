#version 130

in vec3 posWorldSpace;
in vec3 normalWorldSpace;
in vec3 cameraPos;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;     // TODO: rename this uniform -> drawDistance
uniform float reflectance;
uniform float attenuationCoeff;

uniform sampler2D normalTex;
uniform bool useNormalTex;

// primary reflections: rasterization
vec4 primaryReflections() {
    vec3 nPosWorldSpace = normalize(cameraPos - posWorldSpace);
    vec3 nNormalWorldSpace = normalize(normalWorldSpace);

   // Material's reflectivity property
    if (reflectance > 0)
        nNormalWorldSpace = min(nNormalWorldSpace * reflectance, 1.0);

    // Distance calculation
    float viewDistance = length(cameraPos - posWorldSpace);

    // Attenuation effect of sound in the water
    nNormalWorldSpace = nNormalWorldSpace * exp(-2 * attenuationCoeff * viewDistance);

    // Normalize distance using range value (farPlane)
    float nViewDistance = viewDistance / farPlane;

    // presents the normal and depth data as matrix
    vec4 output = vec4(0, 0, 0, 1);
    if (nViewDistance <= 1) {
        if (drawDepth)  output.y = nViewDistance;
        if (drawNormal) output.z = abs(dot(nPosWorldSpace, nNormalWorldSpace));
    }

    return output;
}

void main() {
    // output: primary reflections by rasterization
    vec4 firstR = primaryReflections();

    gl_FragData[0] = firstR;
}