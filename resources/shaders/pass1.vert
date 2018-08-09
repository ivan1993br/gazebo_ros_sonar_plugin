#version 130

uniform mat4 osg_ViewMatrixInverse;

out vec3 posWorldSpace;
out vec3 normalWorldSpace;
out vec3 cameraPos;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // position/normal in world space
    mat4 modelWorld = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    posWorldSpace = vec3(modelWorld * gl_Vertex);
    normalWorldSpace = mat3(modelWorld) * gl_Normal;

    // incident vector
    cameraPos = osg_ViewMatrixInverse[3].xyz;

    // Normal maps are built in tangent space, interpolating the vertex normal and a RGB texture.
    // TBN is the conversion matrix between Tangent Space -> World Space.
    // TODO: update TBN matrix with world coordinates

    // Texture for normal mapping (irregularities surfaces)
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}