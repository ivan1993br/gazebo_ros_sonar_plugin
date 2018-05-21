#version 130

uniform mat4 osg_ViewMatrixInverse;
uniform vec3 cameraPos;

out vec3 positionEyeSpace;
out vec3 normalEyeSpace;
out vec3 directionEyeSpace;
out vec3 reflectedDir;
out mat3 TBN;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // world space
    mat4 modelWorld = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    vec3 positionWorldSpace = vec3(modelWorld * gl_Vertex);
    vec3 normalWorldSpace = mat3(modelWorld) * gl_Normal;

    // eye space
    positionEyeSpace = vec3(gl_ModelViewMatrix * gl_Vertex);
    normalEyeSpace = gl_NormalMatrix * gl_Normal;
    directionEyeSpace = normalize(positionWorldSpace - cameraPos);

    // Normal maps are built in tangent space, interpolating the vertex normal and a RGB texture.
    // TBN is the conversion matrix between Tangent Space -> World Space.
    vec3 N = normalize(normalEyeSpace);
    vec3 T = cross(N, vec3(-1,0,0));
    vec3 B = cross(N, T) + cross(T, N);
    TBN = mat3(T, B, N);

    // calculate the reflection direction for an incident vector
    vec3 WN = normalize(normalWorldSpace);
    vec3 I = directionEyeSpace;
    reflectedDir = normalize(reflect(I, WN));

    // Texture for normal mapping (irregularities surfaces)
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
