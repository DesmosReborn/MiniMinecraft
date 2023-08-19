#version 150

uniform mat4 u_ViewProj;    // We're actually passing the inverse of the viewproj
                            // from our CPU, but it's named u_ViewProj so we don't
                            // have to bother rewriting our ShaderProgram class

uniform ivec2 u_Dimensions; // Screen dimensions

uniform vec3 u_Eye; // Camera pos

uniform float u_Time;
uniform float u_TimeOfDay;

uniform float u_Weather;


out vec4 outColor;
const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// Weather palette
const vec3 weather[5] = vec3[](vec3(150, 150, 150) / 255.0,
                               vec3(120, 120, 120) / 255.0,
                               vec3(100, 100, 100) / 255.0,
                               vec3(90, 90, 90)/ 255.0,
                               vec3(80, 80, 80) / 255.0);

// Day palette
const vec3 day[5] = vec3[](vec3(133, 211, 255) / 255.0,
                           vec3(120, 192, 255) / 255.0,
                           vec3(110, 187, 255) / 255.0,
                           vec3(100, 161, 255)/ 255.0,
                           vec3(90, 150, 211) / 255.0);

// Sunset palette
const vec3 sunset[5] = vec3[](vec3(150, 200, 200) / 255.0,
                               vec3(210, 190, 119) / 255.0,
                               vec3(230, 180, 103) / 255.0,
                               vec3(255, 150, 81) / 255.0,
                               vec3(255, 100, 51) / 255.0);
// Dusk palette
const vec3 dusk[5] = vec3[](vec3(20, 25, 60) / 255.0,
                            vec3(15, 20, 55) / 255.0,
                            vec3(10, 15, 50) / 255.0,
                            vec3(10, 10, 40) / 255.0,
                            vec3(5, 5, 30) / 255.0);

const vec3 sunColor = vec3(255, 255, 190) / 255.0;
const vec3 cloudColor = sunset[3];

vec2 sphereToUV(vec3 p) { //we have a "sphere" representing the sky surrounding the player and p represents a point on said sphere
    float phi = atan(p.z, p.x); //get phi of p in relation to the sphere
    if(phi < 0) {
        phi += TWO_PI; //remap phi to go from 0 to 2PI rather than -PI to PI
    }
    float theta = acos(p.y); //get theta of p
    return vec2(1 - phi / TWO_PI, 1 - theta / PI); //return sphere coordinates converted to UV coords
}

vec3 uvToDay(vec2 uv) { //create color gradient for day
    if(uv.y < 0.35) {
        return day[0];
    }
    else if(uv.y < 0.5) {
        return mix(day[0], day[1], (uv.y - 0.35) / 0.15);
    }
    else if(uv.y < 0.6) {
        return mix(day[1], day[2], (uv.y - 0.5) / 0.1);
    }
    else if(uv.y < 0.7) {
        return mix(day[2], day[3], (uv.y - 0.6) / 0.1);
    }
    else if(uv.y < 0.8) {
        return mix(day[3], day[4], (uv.y - 0.7) / 0.1);
    }
    return day[4];
}

vec3 uvToWeather(vec2 uv) { //create color gradient for day
    if(uv.y < 0.35) {
        return weather[0];
    }
    else if(uv.y < 0.5) {
        return mix(weather[0], weather[1], (uv.y - 0.35) / 0.15);
    }
    else if(uv.y < 0.6) {
        return mix(weather[1], weather[2], (uv.y - 0.5) / 0.1);
    }
    else if(uv.y < 0.7) {
        return mix(weather[2], weather[3], (uv.y - 0.6) / 0.1);
    }
    else if(uv.y < 0.8) {
        return mix(weather[3], weather[4], (uv.y - 0.7) / 0.1);
    }
    return weather[4];
}


vec3 uvToDusk(vec2 uv) {
    if(uv.y < 0.35) {
        return dusk[0];
    }
    else if(uv.y < 0.5) {
        return mix(dusk[0], dusk[1], (uv.y - 0.35) / 0.15);
    }
    else if(uv.y < 0.6) {
        return mix(dusk[1], dusk[2], (uv.y - 0.5) / 0.1);
    }
    else if(uv.y < 0.7) {
        return mix(dusk[2], dusk[3], (uv.y - 0.6) / 0.1);
    }
    else if(uv.y < 0.8) {
        return mix(dusk[3], dusk[4], (uv.y - 0.7) / 0.1);
    }
    return dusk[4];
}

vec3 uvToSunset(vec2 uv, vec2 pos, float raySunDot, vec3 dayColor, vec3 duskColor) { //create color gradient for sunset
    /*if(uv.y < 0.35) { //create yellow strip for bottom half of sunset
        return sunset[0];
    }
    else if(uv.y < 0.5) { //create orange-y yellow
        return mix(sunset[0], sunset[1], (uv.y - 0.35) / 0.15); //mix yellow with orange, then remap from 0.5 - 0.55 to 0 - 1
    }
    else if(uv.y < 0.6) { //works just like above "else if" except between different colors
        return mix(sunset[1], sunset[2], (uv.y - 0.5) / 0.1);
    }
    else if(uv.y < 0.7) {
        return mix(sunset[2], sunset[3], (uv.y - 0.6) / 0.1);
    }
    else if(uv.y < 0.8) {
        return mix(sunset[3], sunset[4], (uv.y - 0.7) / 0.1);
    }
    return sunset[4];*/
    vec3 outcolor = vec3(0) ;

    if (abs(uv.y - pos.y) < 0.05) {
        outcolor = 0.5 * sunset[4];
    } else if (abs(uv.y - pos.y) < 0.1) {
        outcolor = 0.5 * mix(sunset[4], sunset[3], (abs(uv.y - pos.y) - 0.05) / 0.05);
    } else if (abs(uv.y - pos.y) < 0.2) {
        outcolor = 0.5 * mix(sunset[3], sunset[2], (abs(uv.y - pos.y) - 0.1) / 0.1);
    } else if (abs(uv.y - pos.y) < 0.3) {
        outcolor = 0.5 * mix(sunset[2], sunset[1], (abs(uv.y - pos.y) - 0.2)  / 0.1);
    } else if (abs(uv.y - pos.y) < 0.5) {
        outcolor = 0.5 * mix(sunset[1], sunset[0], (abs(uv.y - pos.y) - 0.3) / 0.2);
    } else if (abs(uv.y - pos.y) < 0.7) {
        outcolor = 0.5 * mix(sunset[0], dayColor, (abs(uv.y - pos.y) - 0.5) / 0.2);
    } else {
        outcolor = 0.5 * dayColor;
    }/*{
        float t = 1 ;
        if (timeOfDay < 45000) {
            //do nothing
        } else if (timeOfDay > 60000) {
            t = 0;
        } else {
            t = (timeOfDay - 45000) / 15000;
        }
        vec3 subcolor = mix(dayColor, duskColor, t) ;
        outcolor = 0.5 * mix(subcolor, sunset[0], (raySunDot + 1) / 2) ;
    }*/

    if (raySunDot > 0.95) {
        outcolor += 0.5 * sunset[4];
    } else if (raySunDot > 0.9) {
        outcolor += 0.5 * mix(sunset[4], sunset[3], (0.95 - raySunDot) / 0.05);
    } else if (raySunDot > 0.85) {
        outcolor += 0.5 * mix(sunset[3], sunset[2], (0.9 - raySunDot) / 0.05);
    } else if (raySunDot > 0.8) {
        outcolor += 0.5 * mix(sunset[2], sunset[1], (0.85 - raySunDot) / 0.05);
    } else if (raySunDot > 0.65) {
        outcolor += 0.5 * mix(sunset[1], sunset[0], (0.8 - raySunDot) / 0.15);
    } else if (raySunDot > 0.5) {
        outcolor += 0.5 * mix(sunset[0], dayColor, (0.65 - raySunDot) / 0.15);
    } else {
        outcolor += 0.5 * dayColor;
    }/*else {
        float t = 1 ;
        if (timeOfDay < 45000) {
            //do nothing
        } else if (timeOfDay > 60000) {
            t = 0;
        } else {
            t = (timeOfDay - 45000) / 15000;
        }
        vec3 subcolor = mix(dayColor, duskColor, t) ;
        outcolor += 0.5 * mix(subcolor, sunset[0], (raySunDot + 1) / 1.65) ;
    }*/

    outcolor.x = min(255, outcolor.x);
    outcolor.y = min(255, outcolor.y);
    outcolor.z = min(255, outcolor.z);

    return outcolor;
}

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}

float WorleyNoise3D(vec3 p)
{
    // Tile the space
    vec3 pointInt = floor(p);
    vec3 pointFract = fract(p);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int z = -1; z <= 1; z++)
    {
        for(int y = -1; y <= 1; y++)
        {
            for(int x = -1; x <= 1; x++)
            {
                vec3 neighbor = vec3(float(x), float(y), float(z));

                // Random point inside current neighboring cell
                vec3 point = random3(pointInt + neighbor);

                // Animate the point
                point = 0.5 + 0.5 * sin(u_Time * 0.01 + 6.2831 * point); // 0 to 1 range

                // Compute the distance b/t the point and the fragment
                // Store the min dist thus far
                vec3 diff = neighbor + point - pointFract;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}

float WorleyNoise(vec2 uv)
{
    // Tile the space
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int y = -1; y <= 1; y++)
    {
        for(int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));

            // Random point inside current neighboring cell
            vec2 point = random2(uvInt + neighbor);

            // Animate the point
            point = 0.5 + 0.5 * sin(u_Time * 0.005 + 6.2831 * point); // 0 to 1 range

            // Compute the distance b/t the point and the fragment
            // Store the min dist thus far
            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

float worleyFBM(vec3 uv) {
    float sum = 0;
    float freq = 2;
    float amp = 0.3;
    for(int i = 0; i < 8; i++) {
        sum += WorleyNoise3D(uv * freq) * amp;
        freq *= 2;
        amp *= 0.5;
    }
    return sum;
}

//#define RAY_AS_COLOR
//#define SPHERE_UV_AS_COLOR
#define WORLEY_OFFSET

void main()
{
    vec2 ndc = (gl_FragCoord.xy / vec2(u_Dimensions)) * 2.0 - 1.0; // -1 to 1 NDC

//    outColor = vec3(ndc * 0.5 + 0.5, 1);

    vec4 p = vec4(ndc.xy, 1, 1); // Pixel at the far clip plane
    p *= 10000.0; // Times far clip plane value
    p = /*Inverse of*/ u_ViewProj * p; // Convert from unhomogenized screen to world

    vec3 rayDir = normalize(p.xyz - u_Eye);

#ifdef RAY_AS_COLOR
    outColor = 0.5 * (rayDir + vec3(1,1,1));
    return;
#endif

    vec2 uv = sphereToUV(rayDir);
#ifdef SPHERE_UV_AS_COLOR
    outColor = vec3(uv, 0);
    return;
#endif


    vec2 offset = vec2(0.0);
#ifdef WORLEY_OFFSET
    // Get a noise value in the range [-1, 1]
    // by using Worley noise as the noise basis of FBM
    offset = vec2(worleyFBM(rayDir));
    //offset *= 2.0;
    offset -= vec2(1.0);
#endif

    // Compute a gradient from the bottom of the sky-sphere to the top
    vec3 dayColor = uvToDay(uv + offset * 0.1);

    // Add a glowing sun in the sky
    vec3 sunDir = normalize(vec3(u_Eye.x + 1000 * -sin(u_TimeOfDay * PI / 12.f), u_Eye.y + 1000 * -cos(u_TimeOfDay * PI / 12.f), 1)); //set position of sun
    vec2 sunUV = sphereToUV(sunDir);
    sunUV.y -= 0.05;

    vec3 duskColor = uvToDusk(uv + offset * 0.03); //same but for dusk
    vec3 sunsetColor = uvToSunset(uv + offset * 0.03, sunUV, dot(rayDir, sunDir), dayColor, duskColor); //use noise function to blur color gradient of sunset (make the color transition less linear)
    vec3 weatherColor = uvToWeather(uv + offset * 0.03);

    float sunSize = 30; //set size of sun
    float angle = acos(dot(rayDir, sunDir)) * 360.0 / PI; //find angle between vector between player and sun and vector representing camera's look
    // If the angle between our ray dir and vector to center of sun
    // is less than the threshold, then we're looking at the sun

    vec3 sunMixColor = dayColor;

    // Otherwise our ray is looking into just the sky
    float raySunDot = dot(rayDir, sunDir); //get cos of the angle between look and sun
#define SUNSET_THRESHOLD 0.75
#define DAY_THRESHOLD -1
    if (u_TimeOfDay >= 4.f && u_TimeOfDay <= 8.f) {
        float t = (u_TimeOfDay - 4.f) / 4.f;
        dayColor = mix(duskColor, dayColor, t);
        sunMixColor = dayColor;
    } else if (u_TimeOfDay <= 16.f && u_TimeOfDay >= 8.f) {
        dayColor = dayColor;
        sunMixColor = dayColor;
    } else if (u_TimeOfDay <= 18.f && u_TimeOfDay >= 16.f) {
        float t = (u_TimeOfDay - 16.f) / 2.f;
        dayColor = mix(dayColor, sunsetColor, t);
        sunMixColor = dayColor;

    } else if (u_TimeOfDay <= 19.f && u_TimeOfDay >= 18.f) {
        dayColor = sunsetColor;
        sunMixColor = sunsetColor;

    } else if (u_TimeOfDay <= 20.f && u_TimeOfDay >= 19.f) {
        float t = (u_TimeOfDay - 19.f) / 1.f;
        dayColor = mix(sunsetColor, duskColor, t);
        sunMixColor = dayColor;
    } else {
        dayColor = duskColor;
        sunMixColor = duskColor;
    }
    /*if (raySunDot > SUNSET_THRESHOLD) {
        if (timeOfDay < 30000) {
            dayColor = dayColor;
            sunMixColor = dayColor;
        } else if (timeOfDay < 50000) {
            float t = (timeOfDay - 30000) / 20000;
            dayColor = mix(dayColor, sunsetColor, t);
        } else {
            dayColor = sunsetColor;
            sunMixColor = sunsetColor;
        }
    }
    else if(raySunDot > DUSK_THRESHOLD) {
        float t = (raySunDot - SUNSET_THRESHOLD) / (DUSK_THRESHOLD - SUNSET_THRESHOLD);
        dayColor = mix(sunsetColor, duskColor, t); //set color mix between sunset and dusk
    }
    // Any dot product <= -0.1 are pure dusk color
    else {
        dayColor = duskColor;
    }*/

    if(angle < sunSize) {
        // Full center of sun
        if(angle < 7.5) {
            dayColor = sunColor;
        }
        // Corona of sun, mix with sky color
        else {
            dayColor = mix(sunColor, sunMixColor, (angle - 7.5) / 22.5);
        }
    }

    outColor = vec4(mix(dayColor, weatherColor, u_Weather), 1) ;

}
