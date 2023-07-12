#version 150

//Assignment 2.1 - create and display stars geometry

in vec3 pass_Color;
out vec4 out_Color;

void main() {
  out_Color = vec4(pass_Color, 1.0);
}
