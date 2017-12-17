#version 130

uniform mat4 osg_ViewMatrixInverse;
uniform vec3 cameraPos;

out vec3 positionEyeSpace;
out vec3 normalEyeSpace;
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
    vec3 directionEyeSpace = normalize(positionWorldSpace - cameraPos);

    // Normal maps are built in tangent space, interpolating the vertex normal and a RGB texture.
    // TBN is the conversion matrix between Tangent Space -> World Space.
    vec3 n = normalize(normalEyeSpace);
    vec3 t = cross(n, vec3(-1,0,0));
    vec3 b = cross(n, t) + cross(t, n);
    TBN = mat3(t, b, n);

    // Texture for normal mapping (irregularities surfaces)
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    // Texture for dynamic cube mapping (indirect reflection)
    gl_TexCoord[1].xyz = reflect(   normalize(directionEyeSpace),
                                    normalize(normalWorldSpace));
}
