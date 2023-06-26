#version 330 core
#define PI 3.1415926535897932384626433832795

in vec2 TexCoords;
in vec2 pass_SunPos;

out vec4 FragColor;

uniform sampler2D ColorTex;
uniform sampler2D DepthTex;
uniform sampler2D LightTex;

const float NEAR = 0.1;
const float FAR = 100.0;

float linearizeDepth(float depth) {
    return (2.0 * NEAR * FAR) / (FAR + NEAR - (depth * 2.0 - 1.0) * (FAR - NEAR));
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

const vec2 SCREEN_RES = vec2(1280, 720);
const float ASPECT = SCREEN_RES.x / SCREEN_RES.y;

//horizontal triangle rows on the screen
const int SEGMENTS = 3;
//how much of the height of the scene one triangle covers
float triHeight = 1;
float triHalfWidth = triHeight / sqrt(3.0) / ASPECT;

//the 3 possible uvs for all triangles in the kaleidoscope
vec2[3] uvs = vec2[3](vec2(0.5 - triHalfWidth, 0.0), vec2(0.5 + triHalfWidth, 0.0), vec2(0.5, triHeight));
//the 4 vertices of 2 triangles if they were sheared to form a square
vec2[4] vertices = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));

vec2 kaleidoscopeUV(vec2 uv) {
    vec2 pos = uv;
    //center triangles horizontally;
    pos.x += 0.5 + triHalfWidth / SEGMENTS;
    //map triangle width & height to 01 range
    pos *= vec2(ASPECT, 1.0) / vec2(2.0 / sqrt(3.0), 1.0) * SEGMENTS;
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

//from https://www.shadertoy.com/view/4s2GRR
vec2 fisheye(vec2 uv, float strength) {
    vec2 pos = uv;
    pos.x *= ASPECT;

    vec2 center = vec2(0.5 * ASPECT, 0.5);
    vec2 delta = pos - center;
    //radius from screen center
    float radius = length(delta);

    float halfDiagonal = length(center);
    float power = (PI / (2.0 * halfDiagonal)) * strength;

    vec2 distortedUV = center + normalize(delta) * tan(radius * power) * halfDiagonal / tan( halfDiagonal * power);
    distortedUV.x /= ASPECT;
    return distortedUV;
}

const int PIXEL_SIZE = 6;
const vec2 PIXEL_SCREEN_RES = SCREEN_RES / PIXEL_SIZE;
const int COLOR_DEPTH = 4;   // Higher num - higher colors quality

const mat4 ditherTable = mat4(
    -4.0, 0.0, -3.0, 1.0,
    2.0, -2.0, 3.0, -1.0,
    -3.0, 1.0, -4.0, 0.0,
    3.0, -1.0, 2.0, -2.0
);

//https://www.shadertoy.com/view/tsKGDm
vec4 dithered(vec2 uv, float ditherStrength) {
    //pixelate uv coordinates
    vec2 pixelPos = floor(uv * PIXEL_SCREEN_RES);
    vec2 pixelUV = pixelPos / PIXEL_SCREEN_RES;
    //get texture color (hardcodedly)
    vec3 color = (texture(ColorTex, pixelUV) + radialBlurColor(pixelUV, 200, 1.0, 0.99)).xyz;

    //dither color
    color += ditherTable[int(pixelPos.x) % 4][int(pixelPos.y) % 4] * ditherStrength;
    //reduce colors depth
    color = floor(color * COLOR_DEPTH) / COLOR_DEPTH;
    return vec4(color, 1.0);
}

void main() {
    vec2 uv = TexCoords;
//    uv = kaleidoscopeUV(uv);
//    uv = fisheye(uv, 0.8);

//    float depth = linearizeDepth(texture(DepthTex, TexCoords).r) / far;
//    FragColor = vec4(depth, depth, depth, 1);
//    FragColor = vec4(1.0) - texture(ColorTex, TexCoords);

//    FragColor = texture(ColorTex, uv) + radialBlurColor(uv, 200, 1.0, 0.99);
    FragColor = dithered(uv, 0.005);
}