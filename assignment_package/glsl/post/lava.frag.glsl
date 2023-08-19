#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

in vec2 fs_UV;

uniform sampler2D u_Texture;
uniform int u_Time;

out vec4 out_Col;

const float PI = 3.14159265;


// -- noise functions!! --

float rand(vec2 c){
    return fract(sin(dot(c.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float noise(vec2 p, float freq ){
    float unit = 1/freq;
    vec2 ij = floor(p/unit);
    vec2 xy = mod(p,unit)/unit;
    //xy = 3.*xy*xy-2.*xy*xy*xy;
    xy = .5*(1.-cos(PI*xy));
    float a = rand((ij+vec2(0.,0.)));
    float b = rand((ij+vec2(1.,0.)));
    float c = rand((ij+vec2(0.,1.)));
    float d = rand((ij+vec2(1.,1.)));
    float x1 = mix(a, b, xy.x);
    float x2 = mix(c, d, xy.x);
    return mix(x1, x2, xy.y);
}

float pNoise(vec2 p, int res){
    float persistance = .5;
    float n = 0.;
    float normK = 0.;
    float f = 4.;
    float amp = 1.;
    int iCount = 0;
    for (int i = 0; i<50; i++){
        n+=amp*noise(p, f);
        f*=2.;
        normK+=amp;
        amp*=persistance;
        if (iCount == res) break;
        iCount++;
    }
    float nf = n/normK;
    return nf*nf*nf*nf;
}

// -------------------

void main()
{
    float distFromCenterWeight = distance(vec2(0.5, 0.5), fs_UV);
    out_Col = mix(vec4(1.2, 0.5, 0.35, 1), vec4(2, 0.3, 0.2, 1), distFromCenterWeight)
            * vec4(texture(u_Texture, fs_UV + vec2(pNoise(fs_UV + vec2(0, u_Time * 0.00035), 2) * 0.8 * distFromCenterWeight, 0)).rgb, 1);
}
