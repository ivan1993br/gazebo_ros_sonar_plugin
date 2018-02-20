#version 130

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;
in mat3 TBN;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;
uniform sampler2D normalTexture;
uniform float reflectance;
uniform float attenuationCoeff;

out vec4 out_data;

void main() {
    vec3 nNormalEyeSpace;

    // Normal for textured scenes (by normal mapping)
    if (textureSize(normalTexture, 0).x > 1) {
        vec3 normalRGB = texture2D(normalTexture, gl_TexCoord[0].xy).rgb;
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

    vec3 nPositionEyeSpace = normalize(-positionEyeSpace);

    float linearDepth = sqrt(positionEyeSpace.x * positionEyeSpace.x +
                             positionEyeSpace.y * positionEyeSpace.y +
                             positionEyeSpace.z * positionEyeSpace.z);

    // Attenuation effect of sound in the water
    nNormalEyeSpace = nNormalEyeSpace * exp(-2 * attenuationCoeff * linearDepth);

    linearDepth = linearDepth / farPlane;

    // output the normal and depth data as matrix
    out_data = vec4(0, 0, 0, 1);
    if (linearDepth <= 1) {
        if (drawNormal) out_data.z = abs(dot(nPositionEyeSpace, nNormalEyeSpace));
        if (drawDepth)  out_data.y = linearDepth;
    }

    gl_FragDepth = linearDepth;
}
