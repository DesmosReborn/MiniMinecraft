#include "icespike.h"

std::vector<std::pair<BlockType, glm::ivec3>> IceSpike::getStructureBlocks()
{
    int height = glm::mix(4, 25, seed);

    std::vector<std::pair<BlockType, glm::ivec3>> blocks;

    // inner spike
    for (int y = -height; y <= height; ++y) {
        blocks.push_back(std::make_pair(ICE, glm::ivec3(0, y, 0)));
    }

    // outer spike
    for (int y = -height / 2; y <= height / 2; ++y) {
        blocks.push_back(std::make_pair(ICE, glm::ivec3(-1, y, -1)));
        blocks.push_back(std::make_pair(ICE, glm::ivec3(1, y, -1)));
        blocks.push_back(std::make_pair(ICE, glm::ivec3(1, y, 1)));
        blocks.push_back(std::make_pair(ICE, glm::ivec3(-1, y, 1)));
    }

    for (int y = -height / 4 * 3; y <= height / 4 * 3; ++y) {
        blocks.push_back(std::make_pair(ICE, glm::ivec3(1, y, 0)));
        blocks.push_back(std::make_pair(ICE, glm::ivec3(-1, y, 0)));
        blocks.push_back(std::make_pair(ICE, glm::ivec3(0, y, 1)));
        blocks.push_back(std::make_pair(ICE, glm::ivec3(0, y, -1)));
    }

    return blocks;
}
