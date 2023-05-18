#version 150

in vec3 pass_Normal;
in vec3 pass_Color;
in vec3 pass_LightColor;
in vec3 pass_PointLightColor;
in vec3 pass_PointLightDir;
in vec3 pass_AmbientLight;

out vec4 out_Color;

void main() {
  out_Color = vec4(pass_Color * pass_AmbientLight, 1.0);
  float lightStrength = clamp(dot(pass_PointLightDir, pass_Normal), 0, 1);
  out_Color += vec4(lightStrength * pass_Color * pass_PointLightColor, 1.0);
}
