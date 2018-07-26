#version 130

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;
in mat3 TBN;
in vec3 directionEyeSpace;
in vec3 reflectedDir;

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

    // Attenuation effect of sound in the water
    nNormalEyeSpace = nNormalEyeSpace * exp(-2 * attenuationCoeff * linearDepth);

    // Normalize depth using range value (farPlane)
    linearDepth = linearDepth / farPlane;
    gl_FragDepth = linearDepth;

    // presents the normal and depth data as matrix
    vec4 output = vec4(0, 0, 0, 1);
    if (linearDepth <= 1) {
        if (drawNormal) output.z = abs(dot(nPositionEyeSpace, nNormalEyeSpace));
        if (drawDepth)  output.y = linearDepth;
    }

    return output;
}

void main() {
    // output: primary reflections by rasterization
    gl_FragData[0] = primaryReflections();

    // output: incident vector (origin) of reflected surfaces
    gl_FragData[1] = vec4(directionEyeSpace, 1);

    // output: direction vector of reflected surfaces
    gl_FragData[2] = vec4(reflectedDir, 1);
}
