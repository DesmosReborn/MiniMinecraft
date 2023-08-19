#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

in vec2 fs_UV;

uniform sampler2D u_Texture;
uniform int u_Time;
uniform float u_Weather;

out vec4 out_Col;

const float PI = 3.14159265;


// -- noise functions!! --

float noise(vec2 co){
    return fract(sin(dot(co.xy ,vec2(23.12456,69.233))) * 69210.4235);
}

float WorleyNoise(vec2 uv, float cells) {
    uv *= cells;
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);
    float minDist = 1.0;
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = vec2(noise(uvInt + neighbor));
            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

void main()
{
    float wNoise = WorleyNoise(fs_UV - vec2((-u_Time*0.0002),(-u_Time*0.0002)), 3);
    out_Col = vec4((texture(u_Texture, fs_UV + mix(vec2(0), vec2(wNoise * 0.01), u_Weather)).rgb - mix(0, wNoise*0.1, u_Weather)), 1);
    float greyScale = (out_Col.r + out_Col.g + out_Col.b)/3;
    out_Col = mix(out_Col, vec4(greyScale, greyScale, greyScale, 1), u_Weather * 0.35);
}
