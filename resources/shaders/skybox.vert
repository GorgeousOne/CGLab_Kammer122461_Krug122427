#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform vec3 CameraPos;

void main() {
    //texture coordinates somehow need to be mirrored except along z axis
    TexCoords = -aPos;
    TexCoords.z *= -1;
    gl_Position = ProjectionMatrix * ViewMatrix * vec4(aPos + CameraPos, 1.0);
}