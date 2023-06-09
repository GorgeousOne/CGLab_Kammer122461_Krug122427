#version 330 core
// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;

//Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform vec3 Color;
uniform vec3 PointLightColor;
uniform vec3 PointLightPos;
uniform vec3 AmbientLight;
uniform vec3 CameraPos;
uniform bool IsCelEnabled;
uniform bool IsNormalMapEnabled;

out vec3 pass_VertexPos;
out vec3 pass_Normal;
out vec3 pass_Color;
out vec3 pass_PointLightColor;
out vec3 pass_PointLightDir;
out float pass_PointLightDist;
out vec3 pass_ViewDir;
out vec3 pass_AmbientLight;
out vec2 pass_TexCoord;

void main(void)
{
	vec4 worldPos = ModelMatrix * vec4(in_Position, 1.0);
    gl_Position = (ProjectionMatrix * ViewMatrix) * worldPos;
    pass_VertexPos = worldPos.xyz;
    pass_Normal = normalize((NormalMatrix * vec4(in_Normal, 0.0)).xyz);
    pass_Color = Color;
    
    // calculate distances
    vec3 lightDist = PointLightPos - worldPos.xyz;
    pass_PointLightDist = length(lightDist);
    //calculate normalized light direction
    pass_PointLightDir = lightDist / pass_PointLightDist;
    //square light distance for light falloff
    pass_PointLightDist *= pass_PointLightDist;

    pass_PointLightColor = PointLightColor;
    pass_AmbientLight = AmbientLight;
    pass_ViewDir = normalize(CameraPos - worldPos.xyz);
	pass_TexCoord = in_TexCoord;
}
