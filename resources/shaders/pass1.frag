#version 130

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;
in mat3 TBN;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;
uniform float reflectance;
uniform float attenuationCoeff;

uniform sampler2D normalTex;
uniform bool useNormalTex;

// primary reflections: rasterization
vec4 primaryReflections() {
    vec3 nNormalEyeSpace;

    // Normal for textured scenes (by normal mapping)
    if (useNormalTex) {
        vec3 normalRGB = texture2D(normalTex, gl_TexCoord[0].xy).rgb;
        vec3 normalMap = (normalRGB * 2.0 - 1.0) * TBN;
        nNormalEyeSpace = normalize(normalMap);
    }
    else {
        nNormalEyeSpace = normalize(normalEyeSpace);
    }

    // Material's reflectivity property
    if (reflectance > 0) {
        nNormalEyeSpace = min(nNormalEyeSpace * reflectance, 1.0);
    }

    // Depth calculation
    vec3 nPositionEyeSpace = normalize(-positionEyeSpace);
    float linearDepth = sqrt(positionEyeSpace.x * positionEyeSpace.x +
                             positionEyeSpace.y * positionEyeSpace.y +
                             positionEyeSpace.z * positionEyeSpace.z);
    linearDepth = linearDepth / farPlane;
    gl_FragDepth = linearDepth;

    // Attenuation effect of sound in the water
    nNormalEyeSpace = nNormalEyeSpace * exp(-2 * attenuationCoeff * linearDepth);

    // output the normal and depth data as matrix
    vec4 output = vec4(0, 0, 0, 1);
    if (linearDepth <= 1) {
        if (drawNormal) output.z = abs(dot(nPositionEyeSpace, nNormalEyeSpace));
        if (drawDepth)  output.y = linearDepth;
    }
    return output;
}

void main() {
    // output the primary reflections as texture
    gl_FragData[0] = primaryReflections();
}
