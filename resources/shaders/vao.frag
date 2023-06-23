#version 330 core
in vec3 pass_Color;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 LightEmitColor;

void main(){
    FragColor = vec4(pass_Color, 1.0);
    FragColor = vec4(0.0);
}
