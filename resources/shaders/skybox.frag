#version 330 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 LightEmitColor;

in vec3 TexCoords;

uniform samplerCube Tex;

void main()
{
    FragColor = texture(Tex, TexCoords);
    LightEmitColor = vec4(0.0);
}