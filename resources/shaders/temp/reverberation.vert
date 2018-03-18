#version 130

uniform mat4 osg_ViewMatrixInverse;
uniform vec3 cameraPosition;

out vec3 positionEyeSpace;
out vec3 normalEyeSpace;

out vec3 reflectedDirection;
out vec3 incidentPosition;

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
    vec3 I = normalize(positionWorldSpace - cameraPosition);
    vec3 N = normalize(normalWorldSpace);
    reflectedDirection = normalize(reflect(I, N));
    incidentPosition = I;

    // set texture coordinates for normal mapping
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
