#version 330 core
#define PI 3.1415926535897932384626433832795

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

vec4 radialBlurColor(vec2 uv, int samples, float intensity, float decay) {
    vec2 screenPos = uv;
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

vec3 barycentric(vec2 p, vec2 a, vec2 b, vec2 c) {
    vec2 v0 = b - a;
    vec2 v1 = c - a;
    vec2 v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    return vec3(u, v, w);
}

vec2 screenRes = vec2(1280, 720);
float aspect = screenRes.x / screenRes.y;

//horizontal triangle rows on the screen
int segments = 3;
//how much of the height of the scene one triangle covers
float triHeight = 1;
float triHalfWidth = triHeight / sqrt(3.0) / aspect;

//the 3 possible uvs for all triangles in the kaleidoscope
vec2[3] uvs = vec2[3](vec2(0.5 - triHalfWidth, 0.0), vec2(0.5 + triHalfWidth, 0.0), vec2(0.5, triHeight));
//the 4 vertices of 2 triangles if they were sheared to form a square
vec2[4] vertices = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));

vec2 kaleidoscopeUV(vec2 uv) {
    vec2 pos = uv;
    //center triangles horizontally;
    pos.x += 0.5 + triHalfWidth / segments;
    //map triangle width & height to 01 range
    pos *= vec2(aspect, 1.0) / vec2(2.0 / sqrt(3.0), 1.0) * segments;
    //repeat pattern after 2 rows of triangles
    pos.y = mod(pos.y, 2.0);

    //mirror 2nd row on first row
    int isMirroredY = int(pos.y); //either 0 or 1
    int isInvertedY = int(-2 * (isMirroredY - 0.5)); //maps 0, 1 to 1, -1
    pos.y = isMirroredY * 2 + isInvertedY * pos.y;

    //rectify triangle position ny shearing for easier calculations
    float shearX = 0.5 * pos.y;
    //repeat triangles after 6th in a row
    pos.x = mod(pos.x - shearX, 3.0);

    //calculate offset of triangle uvs in it's row of 6 triangles
    int uvOffset = int(pos.x / 1);

    //bring position in range of first 2 triangles (2 sheared triangles that for a sqaure)
    pos.x = mod(pos.x, 1.0);

    //calculate vertex offset if position lies in 2nd upside down triangle
    int vertOffset = int(pos.x + pos.y);
    //ofsset uvs 1 more if position is in upside down triangle
    uvOffset += vertOffset;

    //interpolate weights of the 3 vertices surrounding the position
    vec3 uvw = barycentric(pos, vertices[vertOffset], vertices[vertOffset + 1], vertices[vertOffset + 2]);
    //use weights at uvs
    vec2 uvInterpolated =
        uvw.x * uvs[uvOffset] +
        uvw.y * uvs[int(mod(uvOffset + 1, 3))] +
        uvw.z * uvs[int(mod(uvOffset + 2, 3))];

    return uvInterpolated;
}

void main() {
    vec2 uv = TexCoords;
    uv = kaleidoscopeUV(uv);
    float depth = linearizeDepth(texture(DepthTex, TexCoords).r) / far;
    //    FragColor = vec4(depth, depth, depth, 1);
    //    FragColor = vec4(1.0) - texture(ColorTex, TexCoords);
    FragColor = texture(ColorTex, uv) + radialBlurColor(uv, 200, 1.0, 0.99);
}