#version 330 core

in vec2 TexCoords;
in vec2 pass_SunPos;

out vec4 FragColor;

uniform sampler2D ColorTex;
uniform sampler2D DepthTex;
uniform sampler2D LightTex;

float near = 0.1;
float far = 100.0;

float linearizeDepth(float depth) {
    return (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near));
}

vec4 radialBlurColor(int samples, float intensity, float decay) {
    vec2 screenPos = TexCoords;
    vec2 lightDir = (pass_SunPos - screenPos);
    lightDir *= 1.0 / samples;
    vec4 color = vec4(0); //texture(LightTex, screenPos);

    intensity /= samples;
    float illuminationDecay = 1.0;

    for (int i = 0; i < samples; ++i) {
        screenPos += lightDir;
        color += texture(LightTex, screenPos) * intensity * illuminationDecay;
        illuminationDecay *= decay;
    }
    return color;
}

void main() {
    float depth = linearizeDepth(texture(DepthTex, TexCoords).r) / far;
//    FragColor = vec4(depth, depth, depth, 1);
//    FragColor = vec4(1.0) - texture(ColorTex, TexCoords);
    FragColor = texture(ColorTex, TexCoords) + radialBlurColor(200, 1.0, 0.99);
}