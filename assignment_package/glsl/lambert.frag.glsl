#version 150
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

uniform sampler2D u_Texture;
uniform int u_Time;
uniform float u_TimeOfDay;
uniform float u_Weather;

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_LightVec;
in vec4 fs_UV;

out vec4 out_Col; // This is the final output color that you will see on your
                  // screen for the pixel that is currently being processed.

const float PI = 3.14159265359;


void main()
{
    float sunHeight = -cos(u_TimeOfDay * PI / 12.f);
    float mappedSunHeight = sunHeight * 0.5f + 0.5f;

    // Material base color (before shading)
    vec2 uv = fs_UV.xy ;
    // if (fs_UV.w > 0.5) {
    //     float offsetx = sin((mod(u_Time, 10) * 0.01)) ;
    //     float offsety = abs(cos((mod(u_Time, 10) * 0.01))) ;
    //     uv.x += (offsetx * 1/16.f) ;
    //     uv.y -= (offsety * 1/16.f) ;
    // }

    vec4 diffuseColor = texture(u_Texture, uv);

    // Sun lambert intensity
    float lambertIntensity = dot(normalize(fs_Nor), -normalize(fs_LightVec)) * 0.5f + 0.5f;
    float altLambertIntensity = dot(normalize(fs_Nor), normalize(vec4(0.3f, 1.f, 0.5f, 0))) * 0.5f + 0.5f;

    float dayMix = smoothstep(-0.3f, 0.2f, sunHeight);
    vec3 ambientTerm = mix(vec3(0.12f, 0.12f, 0.23f), vec3(0.35f, 0.35f, 0.55f), dayMix);
    vec3 sunTerm = lambertIntensity * vec3(0.75f, 0.7f, 0.45f);
    vec3 moonTerm = altLambertIntensity * vec3(0.25f, 0.25f, 0.21f);
    vec3 sunnyFinalLighting = mix(moonTerm, sunTerm, dayMix) + ambientTerm;

    vec3 weatherDayLighting = vec3(0.2f, 0.2f, 0.3f) * altLambertIntensity + vec3(0.6f, 0.6f, 0.6f);
    float weatherDayMultiplier = mix(0.4f, 0.7f, mappedSunHeight);
    vec3 weatherFinalLighting = weatherDayLighting * weatherDayMultiplier;

    // Compute final shaded color
    out_Col = vec4(diffuseColor.rgb * mix(sunnyFinalLighting, weatherFinalLighting, u_Weather), diffuseColor.a);
}
