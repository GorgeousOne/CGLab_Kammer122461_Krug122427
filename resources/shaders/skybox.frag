#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube SkyTex;

void main()
{
    FragColor = texture(SkyTex, TexCoords);
}