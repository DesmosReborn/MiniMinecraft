#include "lookout.h"

#define LOOKOUT_BLOCK COBBLESTONE

std::vector<std::pair<BlockType, glm::ivec3>> Lookout::getStructureBlocks()
{
    int height = glm::mix(5, 28, seed);

    std::vector<std::pair<BlockType, glm::ivec3>> blocks;

    for (int y = -5; y <= height; ++y) {
        for (int x = 0; x <= 1; ++x) {
            blocks.push_back(std::make_pair(LOOKOUT_BLOCK, glm::ivec3(x, y, -1)));
            blocks.push_back(std::make_pair(LOOKOUT_BLOCK, glm::ivec3(x, y, 2)));
            blocks.push_back(std::make_pair(LOOKOUT_BLOCK, glm::ivec3(-1, y, x)));
            blocks.push_back(std::make_pair(LOOKOUT_BLOCK, glm::ivec3(2, y, x)));
        }
    }

    for (int x = 0; x <= 1; ++x) {
        for (int z = 0; z <= 1; ++z) {
            blocks.push_back(std::make_pair(LOOKOUT_BLOCK, glm::ivec3(x, height - 1, z)));
        }
    }

    return blocks;
}
