#version 330 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inTexCoords;

uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform vec3 SunPos;

out vec2 TexCoords;
out vec2 pass_SunPos;

void main() {
    gl_Position = vec4(inPos.x, inPos.y, 0.0, 1.0);
    TexCoords = inTexCoords;

    vec4 sunGlPos = (ProjectionMatrix * ViewMatrix) * vec4(SunPos, 1.0);
    vec4 sunNdc = sunGlPos / sunGlPos.w;
    vec2 sunViewPortPos = sunNdc.xy * 0.5 + 0.5;
    pass_SunPos = sunViewPortPos;
}
