#version 150

in vec3 pass_Normal;
in vec3 pass_Color;
in vec3 pass_LightColor;
in vec3 pass_PointLightColor;
in vec3 pass_PointLightDir;
in vec3 pass_AmbientLight;

out vec4 out_Color;

void main() {
  float lightStrength = -clamp(dot(pass_PointLightDir, pass_Normal), -1, 0);
  out_Color = vec4(pass_Color * (pass_AmbientLight + pass_PointLightColor * lightStrength), 1.0);
  out_Color /= out_Color + vec4(1);
}
