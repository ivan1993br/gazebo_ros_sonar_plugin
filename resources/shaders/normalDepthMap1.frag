#version 130

uniform samplerCube cubeMap;
uniform sampler2D colorMap;
const float reflect_factor = 0.5;

void main (void)															
{
	vec3 base_color = texture2D(colorMap, gl_TexCoord[0].xy).rgb;

	vec3 cube_color = textureCube(cubeMap, gl_TexCoord[1].xyz).rgb;

	gl_FragColor = vec4( mix(cube_color, base_color, reflect_factor).rgb, 1.0);
}
