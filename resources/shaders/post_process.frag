#version 330 core
#define PI 3.1415926535897932384626433832795

in vec2 TexCoords;
in vec2 pass_SunPos;

out vec4 FragColor;

uniform sampler2D ColorTex;
uniform sampler2D DepthTex;
uniform sampler2D LightTex;
uniform sampler2D NoiseTex;
uniform float Time;

const float NEAR = 0.1;
const float FAR = 10.0;

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
const int TRIANGLE_ROWS = 3;
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
    pos.x += 0.5 + triHalfWidth / TRIANGLE_ROWS;
    //map triangle width & height to 01 range
    pos *= vec2(ASPECT, 1.0) / vec2(2.0 / sqrt(3.0), 1.0) * TRIANGLE_ROWS;
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

    //distort uv from center outwards
    vec2 distortedUV = center + normalize(delta) * tan(radius * power) * halfDiagonal / tan(halfDiagonal * power);
    distortedUV.x /= ASPECT;
    return distortedUV;
}

const int PIXEL_SIZE = 6;
const vec2 PIXEL_SCREEN_RES = SCREEN_RES / PIXEL_SIZE;
const int COLOR_DEPTH = 4; // Higher num - higher colors quality

const mat4 ditherTable = mat4(
-4.0, 0.0, -3.0, 1.0,
2.0, -2.0, 3.0, -1.0,
-3.0, 1.0, -4.0, 0.0,
3.0, -1.0, 2.0, -2.0
);

//https://www.shadertoy.com/view/tsKGDm
vec4 dithered(vec2 uv, float ditherStrength) {
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

//https://www.shadertoy.com/view/MsKfRw
// created by florian berger (flockaroo) - 2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// crosshatch effect
#define SHADERTOY_RES vec2(640, 360)
//factor to control hatch moving between -1 and 1
#define FLICKER 0.1

#define TWO_PI 6.28318530718
#define hatchScale (SHADERTOY_RES.x/600.)
//time passed to the shader
//#define Time 1.0

vec2 randOffsets;
float randAmplitude;
float randScale;


//This function applies a smooth distortion to the UV coordinates based on the resolution of the shader.
//It uses sine waves to create a smooth variation in the UV coordinates.
vec2 uvSmooth(vec2 uv, vec2 resolution) {
    return uv + .6 * sin(uv * resolution * TWO_PI) / TWO_PI / resolution;
}

vec4 getRand4(vec2 pos) {
    vec2 texRes = vec2(textureSize(NoiseTex, 0));
    vec2 uv = pos / texRes.xy;
    uv = uvSmooth(uv, texRes);
    return textureLod(NoiseTex, uv, 0.);
}

//This function calculates the color of a pixel based on its position.
//It uses a combination of random vectors, scaling factors, and texture sampling to determine the final color.
vec4 getColor(vec2 pos) {
    vec4 randVec = (getRand4((pos + randOffsets) * .05 * randScale / hatchScale + Time * 131. * FLICKER) - .5) * 10. * randAmplitude;
    vec2 uv = (pos + randVec.xy * hatchScale) / SHADERTOY_RES;
    vec4 c = texture(ColorTex, uv) + radialBlurColor(uv, 200, 1.0, 0.99);
    return c;
}

float luminance(vec3 color) {
    return dot(color, vec3(0.2125, 0.7152, 0.0722));
}

float getGrayColor(vec2 pos) {
    return luminance(getColor(pos).xyz);
}

//This function calculates the gradient of the grayscale values at a given position.
//It computes the differences in grayscale values in the x and y directions using a small epsilon value.
vec2 getGradient(vec2 pos, float eps) {
    vec2 delta = vec2(eps, 0);
    return vec2(
        getGrayColor(pos + delta.xy) - getGrayColor(pos - delta.xy),
        getGrayColor(pos + delta.yx) - getGrayColor(pos - delta.yx)
    ) / eps / 2.;
}

vec4 crosshatch(vec2 fragCoord) {
    fragCoord *= SHADERTOY_RES;
    vec4 fragColor = vec4(1, 1, 1, 1.0);

    // subtraction of 2 rand values, so its [-1..1] and noise-wise not as white anymore
    vec4 randVec = getRand4(fragCoord * 1.2 / sqrt(hatchScale)) - getRand4(fragCoord * 1.2 / sqrt(hatchScale) + vec2(1, -1) * 1.5);

    // cross hatch
    randAmplitude = 0.;
    int hatchNum = 4;

    //returns 2D orthogonal vector
    #define ortho(v) (v.yx * vec2(-1, 1))
    //also controls the angle between multiple hatches? 0 - no rotation, 1 - 45°?
    #define shiftYFactor 1.6
    #define cosPhaseShiftY(angle) cos(angle - vec2(0, shiftYFactor))

    float hatch = 0.;
    float hatch2 = 0.;
    float appliedHatchesNum = 0.;

    for (int i = 0; i < hatchNum; i++) {
        vec2 timeOffset = getRand4(fragCoord * .02 + Time * 1120.).xy - .5;
        float brightness = getGrayColor(fragCoord + 1.5 * hatchScale * timeOffset * clamp(FLICKER, -1., 1.)) * 1.7;

        // chose the hatch angle to be proportional to i*i
        // so the first 2 hatches are close to the same angle,
        // and all the higher i-ths hatches are fairly random in angle
        float hatchAngle = -.5 - .08 * float(i * i);
        //rotates hatching according to hatchangle
        mat2 hatchRotate = mat2(cosPhaseShiftY(hatchAngle), ortho(cosPhaseShiftY(hatchAngle)));
        //rotates uv coordinates to hatchangle
        vec2 uvHatch = hatchRotate * fragCoord;
        //* vec2(.05, 1) scales down x coordinate which stretches random patterns across the x axis
        uvHatch = uvHatch / sqrt(hatchScale) * vec2(.05, 1) * 1.3;
        //creates a random brightness (x coord) where close x coords get similar values
        //sinus probably introduces a randomness along y axis
        vec4 randomHatch = pow(getRand4(uvHatch + 1003.123 * Time * FLICKER + vec2(sin(uvHatch.y), 0)), vec4(1.));

        //decrease brightness of pixel per hatching
        hatch += 1. - smoothstep(.5, 1.5, (randomHatch.x) + brightness) - .3 * abs(randVec.z);
        //i think one of these two actually makes bright hatchings
        hatch2 = max(hatch2, 1. - smoothstep(.5, 1.5, (randomHatch.x) + brightness) - .3 * abs(randVec.z));
        appliedHatchesNum += 1.;

        //no more hatches if something br is too big?
        if (float(i) > (1. - brightness) * float(hatchNum) && i >= 2) {
            break;
        }
    }
    //decreases pixel brightness with a 50/50 mix of hatches
    fragColor.xyz *= 1. - clamp(mix(hatch / appliedHatchesNum, hatch2, 0.5), 0., 1.);
    //brighten up hatches a bit
    float brightenFactor = 0.7;
    fragColor.xyz = 1. - ((1. - fragColor.xyz) * brightenFactor);

    // white paper
    vec3 paperNoise = .95 + .06 * randVec.xxx + .06 * randVec.xyz;
    fragColor.xyz *= luminance(paperNoise);

    fragColor.w = 1.;
    return fragColor;
}
const mat3 GAUSSIAN_KERNEL = mat3(
            1. / 16., 2. / 16., 1. / 16.,
            2. / 16., 4. / 16., 2. / 16.,
            1. / 16., 2. / 16., 1. / 16.
            );

vec4 gaussianBlurColor(vec2 uv) {
    vec2 pixelPos = floor(uv * SCREEN_RES);
    vec4 outColor = vec4(0);

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            vec2 neighborPixel = pixelPos + vec2(dx, dy);
            vec2 neigborUV = neighborPixel / SCREEN_RES;
            outColor += texture(ColorTex, neigborUV) * GAUSSIAN_KERNEL[dy + 1][dx + 1];
            outColor += radialBlurColor(neigborUV, 200, 1.0, 0.99)  * GAUSSIAN_KERNEL[dy + 1][dx + 1];
        }
    }
    return outColor;
}

uniform bool IsMirrorXEnabled;
uniform bool IsMirrorYEnabled;
uniform bool IsFisheyeEnabled;
uniform bool IsKaleidoscopeEnabled;

uniform bool IsHatchingEnabled;
uniform bool IsDitheringEnabled;
uniform bool IsBlurEnabled;
uniform bool IsGrayscaleEnabled;

void main() {
    vec2 uv = TexCoords;

    if (IsMirrorXEnabled) {
        uv.x = 1.0 - uv.x;
    }
    if (IsMirrorYEnabled) {
        uv.y = 1.0 - uv.y;
    }
    if (IsFisheyeEnabled) {
        uv = fisheye(uv, 0.8);
    }
    if (IsKaleidoscopeEnabled) {
        uv = kaleidoscopeUV(uv);
    }

    //if (IsDepthEnabled) {
    //    float depth = linearizeDepth(texture(DepthTex, TexCoords).r) / FAR;
    //    FragColor = vec4(depth, depth, depth, 1);
    //    FragColor = texture(LightTex, TexCoords);
    //}
    vec4 outColor = vec4(0.0);

    if (IsHatchingEnabled) {
        outColor = crosshatch(uv);
    } else if (IsDitheringEnabled) {
        outColor = dithered(uv, 0.005);
    } else if (IsBlurEnabled) {
        outColor = gaussianBlurColor(uv);
    } else {
        //default rendering
        outColor = texture(ColorTex, uv) + radialBlurColor(uv, 200, 1.0, 0.99);
    }

    if (IsGrayscaleEnabled) {
        float luminance = luminance(outColor.xyz);
        outColor.xyz = vec3(luminance);
    }
    FragColor = outColor;
}