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

vec2 screenRes = vec2(1280, 720);
int segments = 3;
float triHeight = screenRes.y / segments;
vec2 triSize = vec2(2 * triHeight / sqrt(3.0), triHeight);
vec2 triCenter = triSize * vec2(0.5, 1.0 / 3.0);

vec2 kaleidoscopeUV(vec2 uv) {
    //scale 01 uvs to screen size
    vec2 pos = uv * screenRes;
    //center triangles horizontally;
    pos.x += 0.5 * screenRes.x + 0.25 * triSize.x;
    //transform pos to be between 01 for triangle extents
    pos /= triSize;
    //repeat pattern after 2 rows of triangles
    pos.y = mod(pos.y, 2);

    //mirror 2nd row on first row
    if (pos.y >= 1) {
        pos.y = 2 - pos.y;
    }
    //rectify triangle grid for easier coordinates
    float shearX = 0.5 * pos.y;
    //repeat triangles after 6 in a row
    pos.x = mod(pos.x - shearX, 3);

    //make triangles 4, 5, 6 flipped versions of triangles 1, 2, 3
    if (pos.x + pos.y > 2) {
        //translate to 1, 2, 3
        pos.x -= 2;
        //mirror y coordinate
        pos.y = 1 - pos.y;
        //mirror rectification shear as well
        pos.x += 2 * shearX;
        shearX = 0.5 - shearX;
    }
    //define transforms to rotate and flip triangles 2 & 3 correctly
    //rotation to rotate triangles to triangle 1
    float rotation = 0;
    //shift to translate to 1
    vec2 shift = vec2(0.0);
    //mirror to create mirrored image of 1
    bool doMirrorX = false;
    
    //set transforms for 3rd triangle
    if (pos.x > 1) {
        rotation = PI * 2.0 / 3.0;
        shift.x = - 1;
    //... for for 2nd triangle
    }else if (pos.x + pos.y > 1) {
        rotation = -PI / 3.0;
        shift = vec2(-0.5, -1.0 / 3.0);
        doMirrorX = true;
    }
    //undo rectification shear to be able to rotate properly
    pos.x += shearX;
    //perform transformtion
    pos += shift;
    pos *= triSize;

    //bring rotation center to middle of triangle
    pos -= triCenter;

    vec2 rotatedPos = vec2(
        pos.x * cos(rotation) - pos.y * sin(rotation),
        pos.x * sin(rotation) + pos.y * cos(rotation)
    );
    if (doMirrorX) {
        rotatedPos.x = 1 - rotatedPos.x;
    }
    rotatedPos += triCenter;
    rotatedPos /= screenRes;
    rotatedPos *= segments;
    return rotatedPos;
}

void main() {
    vec2 uv = TexCoords;
    uv = kaleidoscopeUV(uv);
    float depth = linearizeDepth(texture(DepthTex, TexCoords).r) / far;
    //    FragColor = vec4(depth, depth, depth, 1);
    //    FragColor = vec4(1.0) - texture(ColorTex, TexCoords);
    FragColor = texture(ColorTex, uv) + radialBlurColor(uv, 200, 1.0, 0.99);
}