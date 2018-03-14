#version 130

uniform mat4 osg_ViewMatrixInverse;
uniform vec4 osg_MultiTexCoord0;

out vec3 positionEyeSpace;
out vec3 normalEyeSpace;
// out vec2 texCoord;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // world space
    mat4 modelWorld = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    vec3 positionWorldSpace = vec3(modelWorld * gl_Vertex);
    vec3 normalWorldSpace = mat3(modelWorld) * gl_Normal;

    // eye space
    positionEyeSpace = vec3(gl_ModelViewMatrix * gl_Vertex);
    normalEyeSpace = gl_NormalMatrix * gl_Normal;

    // calculate the reflection direction for an incident vector
    vec3 I = normalize(positionWorldSpace);
    vec3 N = normalize(normalWorldSpace);
    vec3 reflectedDirection = normalize(reflect(I, N));

    // texCoord = gl_MultiTexCoord0.st;
    // texCoord = osg_MultiTexCoord0.st;


    // tc = vec2( (gl_VertexID & 2) >> 1, 1 - (gl_VertexID & 1) );

    // gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_TexCoord[0] = osg_MultiTexCoord0;
}
