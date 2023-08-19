#pragma once

#include "glm_includes.h"


namespace Noise
{
    // pure noise

    float rand(glm::vec2 co),
        rand(glm::vec2 co, float l),
        rand(glm::vec2 co, float l, float t);

    float rand(glm::vec3 co);

    float noise(glm::vec2 co);

    // smooth noise functions

    float interp2D(float x, float y);

    float worley(glm::vec2 uv, float cells),
                 worley2(glm::vec2 uv);

    float perlin(glm::vec2 p, float dim),
        perlin(glm::vec3 p, float dim);

    float fbm(float x, float y);

};
