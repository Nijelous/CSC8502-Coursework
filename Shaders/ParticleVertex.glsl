#version 330 core
layout(location = 0) in vec3 vertices;
layout(location = 1) in vec3 translations;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform float scale;

void main(void){
	vec3 vertexPos = vertices + translations;

	mat4 modelMatrix = mat4(1.0);
	modelMatrix[3][0] = vertexPos.x;
	modelMatrix[3][1] = vertexPos.y;
	modelMatrix[3][2] = vertexPos.z;
	modelMatrix[0][0] = scale;
	modelMatrix[1][1] = scale;
	modelMatrix[2][2] = scale;

	mat4 mv = viewMatrix * modelMatrix;
	mv[0][0] = modelMatrix[0][0];
	mv[0][1] = 0;
	mv[0][2] = 0;

	mv[1][0] = 0;
	mv[1][1] = modelMatrix[1][1];
	mv[1][2] = 0;
	
	mv[2][0] = 0;
	mv[2][1] = 0;
	mv[2][2] = modelMatrix[2][2];

	mat4 mvp = projMatrix * mv;

	gl_Position = mvp * vec4(vertices, 1.0);
}