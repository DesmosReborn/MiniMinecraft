#include "tree.h"

std::vector<std::pair<BlockType, glm::ivec3>> Tree::getStructureBlocks()
{
    std::vector<std::pair<BlockType, glm::ivec3>> blocks;

    // make leaf stuffs
    for (int y = 3; y <= 4; ++y) {
        for (int x = -2; x <= 2; ++x) {
            for (int z = -2; z <= 2; ++z) {
                blocks.push_back(std::make_pair(LEAF, glm::ivec3(x, y, z)));
            }
        }
    }

    // upper leaf stuffs
    for (int y = 5; y <= 6; ++y) {
        for (int x = -1; x <= 1; ++x) {
            for (int z = -1; z <= 1; ++z) {
                if (glm::abs(x) + glm::abs(z) + y == 8) continue;
                blocks.push_back(std::make_pair(LEAF, glm::ivec3(x, y, z)));
            }
        }
    }

    // make trunk
    for (int y = 1; y <= 4; ++y) {
        blocks.push_back(std::make_pair(WOOD, glm::ivec3(0, y, 0)));
    }

    return blocks;
}
