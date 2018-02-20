#version 130

uniform mat4 osg_ViewMatrixInverse;
uniform vec3 cameraPos;

void main() {
    gl_Position = ftransform();
    gl_TexCoord[0] = gl_MultiTexCoord0;
    mat4 modelWorld4x4 = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    mat3 modelWorld3x3 = mat3(modelWorld4x4);
    vec4 worldPos = modelWorld4x4 *  gl_Vertex;

    // world space
    vec3 positionWorldSpace = vec3(modelWorld4x4 * gl_Vertex);
    vec3 normalWorldSpace = normalize(modelWorld3x3 * gl_Normal);

    // view space
    vec3 normalEyeSpace = vec3(gl_NormalMatrix * gl_Normal);
    vec3 positionEyeSpace = vec3(gl_ModelViewMatrix * gl_Vertex);

    // calculate the reflection direction for an incident vector
    vec3 N = normalize(modelWorld3x3 * gl_Normal);
    vec3 E = normalize(worldPos.xyz - cameraPos.xyz);
    gl_TexCoord[1].xyz = normalize(reflect(E, N));
}
