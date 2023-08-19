#include "pyramid.h"

std::vector<std::pair<BlockType, glm::ivec3>> Pyramid::getStructureBlocks()
{
    // seed determines how big it is, from radius 15 to 35
    int radius = glm::mix(15, 35, seed);

    std::vector<std::pair<BlockType, glm::ivec3>> blocks;

    for (int y = 0; y <= radius; ++y) {
        for (int x = - radius + y; x < radius - y; ++x) {
            for (int z = - radius + y; z < radius - y; ++z) {
                blocks.push_back(std::make_pair(SANDSTONE, glm::ivec3(x, y - radius / 5, z)));
            }
        }
    }

    return blocks;
}
