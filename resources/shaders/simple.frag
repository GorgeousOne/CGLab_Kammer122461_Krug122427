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
    //amount of light hitting the surface based on the angle between the normal and the light direction
    float lambertian = max(dot(pass_PointLightDir, pass_Normal), 0.0);

    //the vector inbetween light direction and the view direction
    vec3 halfDir = normalize(pass_PointLightDir + pass_ViewDir);
    //the angle between the half vector and the normal
    float specAngle = max(dot(halfDir, pass_Normal), 0.0);
    //specular intensity on the surface
    float specular = pow(specAngle, 50.0);

    if (pass_IsCelEnabled == 1) {
        specular = round(specular);
        lambertian = ceil(2 * lambertian) / 2;

        //angle between the view direction and the normal
        float viewAngle = dot(pass_ViewDir, pass_Normal);

        if (viewAngle < 0.3) {
            out_Color = vec4(pass_Color, 1);
            return;
        }
    }
    vec3 color =
        pass_Color * pass_AmbientLight
        + pass_Color * lambertian * pass_PointLightColor / pass_PointLightDist
        + vec3(1.0) * specular * pass_PointLightColor / pass_PointLightDist;

    out_Color = vec4(color / (vec3(1.0) + color), 1.0);
}
