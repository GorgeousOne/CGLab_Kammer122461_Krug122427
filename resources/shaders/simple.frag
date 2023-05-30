#version 150

in vec3 pass_Normal;
in vec3 pass_Color;
in vec3 pass_LightColor;
in vec3 pass_PointLightColor;
in vec3 pass_PointLightDir;
in float pass_PointLightDist;
in vec3 pass_ViewDir;
in vec3 pass_AmbientLight;
flat in int pass_IsCelEnabled;

out vec4 out_Color;

void main() {
    if (pass_IsCelEnabled == 0) {
        float lambertian = max(dot(pass_PointLightDir, pass_Normal), 0.0);

        vec3 halfDir = normalize(pass_PointLightDir + pass_ViewDir);
        float specAngle = max(dot(halfDir, pass_Normal), 0.0);
        float specular = pow(specAngle, 50);

        vec3 color =
        pass_Color * pass_AmbientLight
        + pass_Color * lambertian * pass_PointLightColor / pass_PointLightDist
        + vec3(1.0) * specular * pass_PointLightColor / pass_PointLightDist;

        out_Color = vec4(color / (vec3(1.0) + color), 1.0);
    } else {
        out_Color = vec4(pass_Color, 1);
    }
}
