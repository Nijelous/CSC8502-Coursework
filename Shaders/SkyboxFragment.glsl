#version 330 core

uniform samplerCube cubeTex;
uniform samplerCube cubeTex2;
uniform float skyMixing;
in Vertex{
	vec3 viewDir;
} IN;

out vec4 fragColour;

void main(void) {
	fragColour = mix(texture(cubeTex, normalize(IN.viewDir)), texture(cubeTex2, normalize(IN.viewDir)), skyMixing);
}