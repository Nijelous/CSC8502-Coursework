#version 330 core
uniform sampler2D diffuseTex;

in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColour;
void main(void){
	vec4 t0 = texture2D(diffuseTex, IN.texCoord);
	fragColour = t0;
}