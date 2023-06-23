#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube Tex;

void main()
{
    FragColor = texture(Tex, TexCoords);
}