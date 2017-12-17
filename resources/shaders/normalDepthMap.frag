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
uniform samplerCube cubeMap;

// out vec4 out_data;

void main() {
    vec3 nNormalEyeSpace;

    // Normal for textured scenes (by normal mapping)
    if (textureSize(normalTexture, 0).x > 1) {
        vec3 normalRGB = texture2D(normalTexture, gl_TexCoord[0].xy).rgb;
        vec3 normalMap = (normalRGB * 2.0 - 1.0) * TBN;
        nNormalEyeSpace = normalize(normalMap);
    } else {
        nNormalEyeSpace = normalize(normalEyeSpace);
    }

    // Material's reflectivity property
    if (reflectance > 0) {
        nNormalEyeSpace = min(nNormalEyeSpace * reflectance, 1.0);
    }

    // Dynamic cube mapping
    vec3 cubeColor = textureCube(cubeMap, gl_TexCoord[1].xyz).rgb;

    vec3 nPositionEyeSpace = normalize(-positionEyeSpace);

    float linearDepth = sqrt(positionEyeSpace.x * positionEyeSpace.x +
                             positionEyeSpace.y * positionEyeSpace.y +
                             positionEyeSpace.z * positionEyeSpace.z);

    // Attenuation effect of sound in the water
    nNormalEyeSpace = nNormalEyeSpace * exp(-2 * attenuationCoeff * linearDepth);

    linearDepth = linearDepth / farPlane;

    // out_data = vec4(0, 0, 0, 0);
    // if (!(linearDepth > 1)) {
    //     if (drawNormal){
    //         float value = dot(nPositionEyeSpace, nNormalEyeSpace);
    //         out_data.zw = vec2( abs(value), 1.0);
    //     }
    //     if (drawDepth) {
    //         out_data.yw = vec2(linearDepth, 1.0);
    //     }
    // }

    // gl_FragDepth = linearDepth;
    gl_FragColor = vec4(cubeColor, 1.0);
}
