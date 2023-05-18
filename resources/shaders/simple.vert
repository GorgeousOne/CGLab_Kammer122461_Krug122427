#version 150
#extension GL_ARB_explicit_attrib_location : require
// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;

//Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform vec3 Color;
uniform vec3 PointLightColor;
uniform vec3 PointLightPos;
uniform vec3 AmbientLight;

out vec3 pass_Normal;
out vec3 pass_Color;
out vec3 pass_PointLightColor;
out vec3 pass_PointLightDir;
out vec3 pass_AmbientLight;

void main(void)
{
	vec4 worldPos = ModelMatrix * vec4(in_Position, 1.0);
	gl_Position = (ProjectionMatrix * ViewMatrix) * worldPos;
	pass_Normal = (NormalMatrix * vec4(in_Normal, 0.0)).xyz;
	pass_Color = Color;

	pass_PointLightDir = normalize(worldPos.xyz - PointLightPos);
	pass_PointLightColor = PointLightColor;
	pass_AmbientLight = AmbientLight;
}
