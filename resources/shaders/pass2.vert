#version 130

uniform mat4 osg_ViewMatrixInverse;
uniform vec3 cameraPos;

out vec3 directionEyeSpace;
out vec3 reflectedDir;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // world space
    mat4 modelWorld = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    vec3 positionWorldSpace = vec3(modelWorld * gl_Vertex);
    vec3 normalWorldSpace = mat3(modelWorld) * gl_Normal;

    // eye space
    directionEyeSpace = normalize(positionWorldSpace - cameraPos);

    // calculate the reflection direction for an incident vector
    vec3 WN = normalize(normalWorldSpace);
    vec3 I = directionEyeSpace;
    reflectedDir = normalize(reflect(I, WN));

    // Texture for normal mapping (irregularities surfaces)
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
