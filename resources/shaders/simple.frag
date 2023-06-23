#version 330 core

in vec3 pass_VertexPos;
in vec3 pass_Normal;
in vec3 pass_Color;
in vec3 pass_LightColor;
in vec3 pass_PointLightColor;
in vec3 pass_PointLightDir;
in float pass_PointLightDist;
in vec3 pass_ViewDir;
in vec3 pass_AmbientLight;
in vec2 pass_TexCoord;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 LightEmitColor;

uniform sampler2D Tex;
uniform sampler2D NormalMap;
uniform bool IsCelEnabled;
uniform bool IsNormalMapEnabled;

vec3 perturbNormal(vec3 vertex_pos, vec3 surf_norm, vec2 uv) {
    //derivatives in x and y direction of the geometry surface at the given vertex position
    vec3 q0 = dFdx(vertex_pos.xyz);
    vec3 q1 = dFdy(vertex_pos.xyz);

    vec2 st0 = dFdx(uv.st);
    vec2 st1 = dFdy(uv.st);

    //calculate vector basis for tangent space
    vec3 S = -normalize(q0 * st1.t - q1 * st0.t);
    vec3 T = normalize(-q0 * st1.s + q1 * st0.s);
    vec3 N = normalize(surf_norm);

    //convert normal map in 0 to 1 range to vector range in -1 to 1
    vec3 mapN = texture2D(NormalMap, uv).xyz * 2.0 - 1.0;

    //rotation matrix to convert from tangent space to world space
    mat3 tsn = mat3(S, T, N);
    //return rotated normal vector
    return normalize(tsn * mapN);
}

void main() {
    vec3 normal = pass_Normal;

    if (IsNormalMapEnabled) {
        normal = perturbNormal(pass_VertexPos, pass_Normal, pass_TexCoord);
    }
    //amount of light hitting the surface based on the angle between the normal and the light direction
    float lambertian = max(dot(pass_PointLightDir, normal), 0.0);

    //the vector inbetween light direction and the view direction
    vec3 halfDir = normalize(pass_PointLightDir + pass_ViewDir);
    //the angle between the half vector and the normal
    float specAngle = max(dot(halfDir, normal), 0.0);
    //specular intensity on the surface
    float specular = pow(specAngle, 50.0);

    //vec3 planetColor = pass_Color;
    vec3 planetColor = texture2D(Tex, pass_TexCoord).xyz;

    if (IsCelEnabled) {
        specular = round(specular);
        lambertian = ceil(2 * lambertian) / 2;

        //angle between the view direction and the normal
        float viewAngle = dot(pass_ViewDir, normal);

        if (viewAngle < 0.3) {
            FragColor = vec4(pass_Color, 1);
            LightEmitColor = vec4(0, 0, 0, 1);
            return;
        }
    }
    vec3 ambient = pass_AmbientLight;

    //make the sun ✨shine✨
    if (length(pass_Color) > 10) {
        ambient += 100;
    }
    vec3 color =
            planetColor * ambient
            + planetColor * lambertian * pass_PointLightColor / pass_PointLightDist
            + vec3(1.0) * specular * pass_PointLightColor / pass_PointLightDist;

    FragColor = vec4(color / (vec3(1.0) + color), 1.0);

    //make sun visible in light only texture
    float luminance = dot(pass_Color, vec3(0.2125, 0.7152, 0.0722));

    if (luminance > 100.0) {
        LightEmitColor = vec4(pass_Color, 1.0);
    } else {
        LightEmitColor = vec4(0, 0, 0, 1);
    }
}
