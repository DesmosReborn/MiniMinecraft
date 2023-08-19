#pragma once

#include "structure.h"

class Lookout : public Structure
{
public:
    using Structure::Structure;

    std::vector<std::pair<BlockType, glm::ivec3>> getStructureBlocks() override;
};
