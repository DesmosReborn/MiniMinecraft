#pragma once

#include "vector"
#include "scene/chunk.h"

class Structure
{
public:
    Structure(glm::ivec2 pos, float seed);
    ~Structure();

    const glm::ivec2 pos;
    const float seed;

    virtual std::vector<std::pair<BlockType, glm::ivec3>> getStructureBlocks() = 0;
};
