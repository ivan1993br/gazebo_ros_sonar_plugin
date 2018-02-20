#version 130

uniform samplerCube cubemapTexture;
uniform bool colorBuffer;

void main (void) {
    vec3 reflected = vec3(textureCube(cubemapTexture, gl_TexCoord[1].xyz));

    if (colorBuffer) {
        gl_FragColor = vec4(reflected, 1.0);
    } else {
        // reflected = normalize(reflected);
        gl_FragColor = vec4(1 - reflected, 1.0);
        // gl_FragColor = vec4(1.0);
    }
}
