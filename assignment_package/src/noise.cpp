#include "noise.h"

#include <array>


float Noise::rand(glm::vec2 co) { return glm::fract(sin(glm::dot(co ,glm::vec2(12.9898,78.233))) * 43758.5453); }
float Noise::rand(glm::vec2 co, float l) { return rand(glm::vec2(rand(co), l)); }
float Noise::rand(glm::vec2 co, float l, float t) { return rand(glm::vec2(rand(co, l), t)); }

float Noise::rand(glm::vec3 co) { return glm::fract(sin(glm::dot(co, glm::vec3(35.123125, 53.134536, 94.213515))) * 340913.2435); }

float Noise::noise(glm::vec2 co){
    return glm::fract(sin(glm::dot(co ,glm::vec2(23.12456,69.233))) * 69210.4235);
}

float Noise::interp2D(float x, float y) {
    int intX = int(floor(x));
    float fractX = glm::fract(x);
    int intY = int(floor(y));
    float fractY = glm::fract(y);

    float v1 = noise(glm::vec2(intX, intY));
    float v2 = noise(glm::vec2(intX + 1, intY));
    float v3 = noise(glm::vec2(intX, intY + 1));
    float v4 = noise(glm::vec2(intX + 1, intY + 1));

    float i1 = glm::mix(v1, v2, fractX);
    float i2 = glm::mix(v3, v4, fractX);
    return glm::mix(i1, i2, fractY);
}

float Noise::worley(glm::vec2 uv, float cells) {
    uv *= cells;
    glm::vec2 uvInt = glm::vec2(floor(uv.x), floor(uv.y));
    glm::vec2 uvFract = glm::fract(uv);
    float minDist = 3.0;
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y));
            glm::vec2 point = glm::vec2(noise(uvInt + neighbor));
            glm::vec2 diff = neighbor + point - uvFract;
            float dist = glm::length(diff);
            minDist = glm::min(minDist, dist);
        }
    }
    return 1 - minDist - 0.02;
}

float Noise::worley2(glm::vec2 uv) {
    uv *= 6;
    glm::vec2 uvInt = glm::vec2(floor(uv.x), floor(uv.y));
    glm::vec2 uvFract = glm::fract(uv);

    float minDist = 1.0;
    float secondMinDist = 1.0;
    glm::vec2 closestPoint;
    for(int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y));
            glm::vec2 point = glm::vec2(noise(uvInt + neighbor));
            glm::vec2 diff = neighbor + point - uvFract;
            float dist = glm::length(diff);
            if (dist < minDist) {
                secondMinDist = minDist;
                minDist = dist;
                closestPoint = point;
            } else if (dist < secondMinDist) {
                secondMinDist = dist;
            }
        }
    }
    float height = -1 * minDist + 1 * secondMinDist;
    return height;
}

// goes from -1 to 1
// p must be normalized!
float Noise::perlin(glm::vec2 p, float dim) {
        glm::vec2 pos = glm::floor(p * dim);
        glm::vec2 posx = pos + glm::vec2(1.0, 0.0);
        glm::vec2 posy = pos + glm::vec2(0.0, 1.0);
        glm::vec2 posxy = pos + glm::vec2(1.0);
/*
        // For exclusively black/white noise
        float c = step(rand(pos, dim), 0.5);
        float cx = step(rand(posx, dim), 0.5);
        float cy = step(rand(posy, dim), 0.5);
        float cxy = step(rand(posxy, dim), 0.5);
*/
        float c = rand(pos, dim);
        float cx = rand(posx, dim);
        float cy = rand(posy, dim);
        float cxy = rand(posxy, dim);

        glm::vec2 d = glm::fract(p * dim);
        glm::vec2 dpi = glm::vec2(d.x * 3.1415, d.y * 3.1415);
        d = glm::cos(dpi);
        d *= -0.5;
        d += 0.5;

        float ccx = glm::mix(c, cx, d.x);
        float cycxy = glm::mix(cy, cxy, d.x);
        float center = glm::mix(ccx, cycxy, d.y);

        return center * 2.0 - 1.0;
}

// goes from -1 to 1
float Noise::perlin(glm::vec3 p, float dim) {
    glm::vec3 localPoint = glm::fract(p * dim);
    glm::vec3 cellPos = p * dim - localPoint;

    std::array<float, 8> dots;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                glm::vec3 localCorner = glm::vec3(x, y, z);
                glm::vec3 globalCorner = localCorner + cellPos;

                glm::vec3 cornerVector = glm::normalize(glm::vec3(rand(globalCorner),
                                                                  rand(globalCorner + glm::vec3(2353)),
                                                                  rand(globalCorner + glm::vec3(-523))));

                dots[x + y * 2 + z * 4] = glm::dot(cornerVector, localPoint - localCorner);
            }
        }
    }

    float x00 = glm::mix(dots[0], dots[1], localPoint.x);
    float xy0 = glm::mix(dots[2], dots[3], localPoint.x);
    float x0z = glm::mix(dots[4], dots[5], localPoint.x);
    float xyz = glm::mix(dots[6], dots[7], localPoint.x);

    float back = glm::mix(x00, xy0, localPoint.y);
    float front = glm::mix(x0z, xyz, localPoint.y);

    return glm::mix(back, front, localPoint.z);

}

float Noise::fbm(float x, float y) {
    float total = 0;
    float persistence = 0.5f;
    int octaves = 8;
    float freq = 2.f;
    float amp = 0.5f;
    for(int i = 1; i <= octaves; i++) {
        total += interp2D(x * freq,
                               y * freq) * amp;

        freq *= 2.f;
        amp *= persistence;
    }
    return pow(glm::smoothstep(0.25, 0.75, pow(total, 1.3)), 1.03);
}
