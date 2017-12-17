#version 130

uniform mat4 osg_ViewMatrixInverse;
uniform vec3 cameraPos;

void main() {
    gl_Position = ftransform();
    gl_TexCoord[0] = gl_MultiTexCoord0;
    mat4 ModelWorld4x4 = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    mat3 ModelWorld3x3 = mat3( ModelWorld4x4 );
    vec4 WorldPos = ModelWorld4x4 *  gl_Vertex;
    vec3 N = normalize( ModelWorld3x3 * gl_Normal );
    vec3 E = normalize( WorldPos.xyz - cameraPos.xyz );
    gl_TexCoord[1].xyz = reflect( E, N );
}
