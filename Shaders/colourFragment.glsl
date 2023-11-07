#version 330 core

in Vertex{
	smooth vec4 colour;
} IN;

out vec4 fragColour;
void main(void) {
	fragColour = IN.colour;
}