#version 330 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D ColorTex;
uniform sampler2D DepthTex;

float near = 0.1;
float far = 100.0;

float linearizeDepth(float depth) {
    return (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near));
}

void main() {
    FragColor = vec4(1.0) - texture(ColorTex, TexCoords);
}