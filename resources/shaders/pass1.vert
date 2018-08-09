#version 130

uniform mat4 osg_ViewMatrixInverse;

out vec3 positionEyeSpace;
out vec3 normalEyeSpace;
out vec3 positionWorldSpace;
out vec3 normalWorldSpace;

out mat3 TBN;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // position/normal in eye space
    positionEyeSpace = vec3(gl_ModelViewMatrix * gl_Vertex);
    normalEyeSpace = gl_NormalMatrix * gl_Normal;

    // position/normal in world space
    mat4 modelWorld = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    positionWorldSpace = vec3(modelWorld * gl_Vertex);
    normalWorldSpace = mat3(modelWorld) * gl_Normal;

    // Normal maps are built in tangent space, interpolating the vertex normal and a RGB texture.
    // TBN is the conversion matrix between Tangent Space -> World Space.
    // TODO: check correct spaces
    vec3 N = normalize(normalEyeSpace);
    vec3 T = cross(N, vec3(-1,0,0));
    vec3 B = cross(N, T) + cross(T, N);
    TBN = mat3(T, B, N);

    // Texture for normal mapping (irregularities surfaces)
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}