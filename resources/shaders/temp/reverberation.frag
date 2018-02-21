#version 130

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;

out vec4 out_data;

void main() {
    vec3 nNormalEyeSpace = normalize(normalEyeSpace);

    vec3 nPositionEyeSpace = normalize(-positionEyeSpace);

    float linearDepth = sqrt(positionEyeSpace.x * positionEyeSpace.x +
                             positionEyeSpace.y * positionEyeSpace.y +
                             positionEyeSpace.z * positionEyeSpace.z);

    linearDepth = linearDepth / farPlane;

    // output the normal and depth data as matrix
    out_data = vec4(0, 0, 0, 1);
    if (linearDepth <= 1) {
        if (drawNormal) out_data.z = abs(dot(nPositionEyeSpace, nNormalEyeSpace));
        if (drawDepth)  out_data.y = linearDepth;
    }

    gl_FragDepth = linearDepth;
}
