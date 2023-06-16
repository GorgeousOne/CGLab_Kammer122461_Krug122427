#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform vec3 CameraPos;

void main()
{
    TexCoords = aPos;
    gl_Position = ProjectionMatrix * ViewMatrix * vec4(aPos + CameraPos, 1.0);
}