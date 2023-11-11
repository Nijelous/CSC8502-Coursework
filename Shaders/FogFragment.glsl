#version 330 core

uniform sampler2D sceneTex;
uniform float visibility;
uniform vec3 fogColour;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColor;

void main(void){
	fragColor = texture2D(sceneTex, IN.texCoord.xy);
	fragColor = mix(vec4(fogColour, 1.0), fragColor, visibility);
}