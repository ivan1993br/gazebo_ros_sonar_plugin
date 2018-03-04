#version 130

uniform mat4 osg_ViewMatrixInverse;

out vec3 positionEyeSpace;
out vec3 normalEyeSpace;

// ray definition, with an origin point and a direction vector
struct Ray {
    vec3 origin;
    vec3 direction;
};

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
}
