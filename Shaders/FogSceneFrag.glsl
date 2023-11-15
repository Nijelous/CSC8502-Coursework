#version 330 core

uniform sampler2D sceneTex;
uniform sampler2D depth;
uniform vec3 fogColour;
uniform float density;
uniform float gradient;
uniform float near;
uniform float far;

in Vertex{
	vec2 texCoord;
} IN;

out vec4 fragColor;

void main(void){
	fragColor = texture2D(sceneTex, IN.texCoord.xy);
	float distance = texture(depth, IN.texCoord).r;
	distance = (2.0 * near * far) / (far + near - (distance * 2.0 - 1.0) * (far - near));
	float visibility = clamp(exp(-pow((distance * density), gradient)), 0.0, 1.0);
	fragColor = mix(vec4(fogColour, 1.0), fragColor, visibility);
}