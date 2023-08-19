#include "chunk.h"

#include "noise.h"

#include "structuredata/pyramid.h"
#include "structuredata/tree.h"
#include "structuredata/icespike.h"
#include "structuredata/lookout.h"


VertexData::VertexData(glm::vec4 p, glm::vec2 u) : pos(p), uv(u) {}

BlockFaceData::BlockFaceData(Direction dir, glm::vec3 n,
                             const VertexData &a, const VertexData &b,
                             const VertexData &c, const VertexData &d)
    : direction(dir), normal(n), verts{a, b, c, d} {}

template <typename T>
size_t EnumHash::operator()(T t) const {
    return static_cast<size_t>(t);
}

Chunk::Chunk(OpenGLContext* context, int x, int z) : Drawable(context), X(x), Z(z), m_blocks(),
    m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}, isBuffered(false)
{}

Chunk::~Chunk()
{}

void Chunk::generateTerrain() {
    // fill with empty first
    std::fill_n(m_blocks.begin(), HEIGHT * WIDTH * WIDTH, EMPTY);

    // iterate through all XZ in chunk
    for (int cx = 0; cx < Chunk::WIDTH; ++cx) {
        for (int cz = 0; cz < Chunk::WIDTH; ++cz) {
            generateTerrainColumn(cx, cz);
        }
    }

    // generate structures
    std::vector<uPtr<Structure>> structures = getStructures();

    glm::ivec3 chunkBoundsMin{X, 0, Z};
    glm::ivec3 chunkBoundsMax{X + WIDTH, HEIGHT, Z + WIDTH};

    for (auto& structure : structures) {
        int x = structure->pos.x, z = structure->pos.y;
        int rootHeight = getTerrainHeight(x, z);

        glm::ivec3 rootPos = glm::ivec3(x, rootHeight, z);
        auto blocks = structure->getStructureBlocks();
        for (auto& block : blocks) {
            glm::ivec3 pos = block.second + rootPos;
            if (pos.x < chunkBoundsMin.x || pos.y < chunkBoundsMin.y || pos.z < chunkBoundsMin.z
                    || pos.x >= chunkBoundsMax.x || pos.y >= chunkBoundsMax.y || pos.z >= chunkBoundsMax.z) {
                continue;
            }

            setBlockAt(pos.x - X, pos.y, pos.z - Z, block.first);
        }
    }
}

void Chunk::generateTerrainColumn(int cx, int cz)
{
    // absolute xz coordinates
    int x = cx + X;
    int z = cz + Z;

    // Add water from 128 to 138
    for (int y = 114; y < 138; y++) {
        setBlockAt(cx, y, cz, WATER);
    }

    // overall stuff
    auto blends = getHeightHumidityBlend(x, z);
    float heightBlend = blends.first;
    float humidityBlend = blends.second;

    float oceanWeight = getOceanWeight(x, z);

    int terrainY = getTerrainHeight(x, z, heightBlend, humidityBlend, oceanWeight);

    // generate stone blocks by default
    for (int y = 0; y <= terrainY; ++y) {
        setBlockAt(cx, y, cz, STONE);
    }

    // add top blocks depending on biome
    Biome biome = getBiome(heightBlend, humidityBlend, oceanWeight);

    switch (biome) {
        case ARCHIPELAGO: {
            if (terrainY >= 138) {
                setBlockAt(cx, terrainY, cz, LEAF);
            }
            break;
        }
        case GRASS_LANDS: {
            for (int y = terrainY - 3; y <= terrainY - 1; ++y) {
                setBlockAt(cx, y, cz, DIRT);
            }
            setBlockAt(cx, terrainY, cz, GRASS);
            break;
        }
        case DESERT: {
            for (int y = terrainY - 3; y <= terrainY; ++y) {
                setBlockAt(cx, y, cz, SAND);
            }
            break;
        }
        case MOUNTAINS: {
            for (int y = terrainY - 3; y <= terrainY; ++y) {
                setBlockAt(cx, y, cz, STONE);
            }
            float snowThreshold = glm::mix(180.f, 220.f, Noise::fbm(x * 0.02f, z * 0.02f));
            if (terrainY > snowThreshold) {
                setBlockAt(cx, terrainY + 1, cz, SNOW);
            }
            break;
        }
        case OCEAN: {
            break;
        }
    }

    // cave systems
    float spaghettiMask = Noise::perlin(glm::vec2(x, z) * 0.01f, 1.f) * 0.5f + 0.5f;
    float cheeseMask = Noise::perlin(glm::vec2(x, z) * 0.01f + glm::vec2(1023), 1.f) * 0.5f + 0.5f;
    for (int y = 0; y < 128; ++y) {
        // blend to top
        float mappedHeight = (y + 3.f) / (128.f - 3.f);
        float heightClip = (glm::smoothstep(0.75f, 1.0f, mappedHeight) + glm::smoothstep(0.75f, 1.f, 1.f - y));

        // edges of noise and such (like spaghetti)
        float spaghettiCaveCarve = Noise::perlin(glm::vec3(x, y, z), 0.03f);
        float spaghettiFill = glm::mix(0.085f, 0.13f, spaghettiMask);
        float spaghettiSdf = glm::mix(-spaghettiFill, 1.f - spaghettiFill, glm::abs(spaghettiCaveCarve));

        // holes like swiss cheese
        float cheeseCarve = Noise::perlin(glm::vec3(x, y, z) + glm::vec3(2309.363), 0.019f) * 0.5f + 0.5f;
        float cheeseFill = glm::mix(0.075f, 0.12f, cheeseMask);
        float cheeseSdf = glm::mix(-cheeseFill, 1.f - cheeseFill, cheeseCarve);

        float mergeRadius = 0.045f;
        glm::vec2 mergeVec(spaghettiSdf, cheeseSdf);
        mergeVec = glm::min(mergeVec, glm::vec2(0));
        float caveCarve = glm::length(mergeVec) - mergeRadius;

        if (caveCarve < -heightClip) {
            setBlockAt(cx, y, cz, y < 24 ? LAVA : EMPTY);
        }
    }

    setBlockAt(cx, 0, cz, BEDROCK);
}

std::pair<float, float> Chunk::getHeightHumidityBlend(int x, int z)
{
    float heightBlend = glm::smoothstep(0.4f, 0.6f, Noise::fbm(x * 0.004f + 63.841f, z * 0.004f + 83.4517f));
    float humidityBlend = glm::smoothstep(0.4f, 0.6f, Noise::fbm(x * 0.004f, z * 0.004f));
    return std::make_pair(heightBlend, humidityBlend);
}

float Chunk::getOceanWeight(int x, int z)
{
    return glm::smoothstep(0.45f, 0.65f, Noise::fbm(x * 0.001f, z * 0.001f));
}

int Chunk::getTerrainHeight(int x, int z, float heightBlend, float humidityBlend, float oceanWeight)
{
    // -- GRASSLAND --
    float grassLandY = glm::mix(138.f, 166.f, abs(Noise::worley(glm::vec2(x, z), 0.01f)) +
                                 Noise::fbm((x)* 0.05, (z)* 0.05) * 0.06);

    // -- MOUNTAINS --
    float my1noise = Noise::fbm((x)* 0.005, (z)* 0.005);
    float mountainY1 = glm::mix(0.65f, 0.9f, my1noise * my1noise);
    float my2noise = Noise::fbm((x + 9000)* 0.005, (z + 9000)* 0.005);
    float mountainY2 = glm::mix(0.65f, 0.9f, my2noise * my2noise);

    // Chooses the highest mountain
    float mountainY = glm::max(mountainY1, mountainY2);
    mountainY *= 255.f;

    // -- ARCHIPELAGO --
    float archipelagoYBig = glm::mix(114.f, 185.f, glm::smoothstep(0.2f, 1.f, Noise::perlin(glm::vec2(x, z), 0.02f) * 0.5f + 0.5f));
    float archipelagoYSmall = glm::mix(0.f, 13.f, Noise::perlin(glm::vec2(x, z), 0.1f) * 0.5f + 0.5f);
    float archipelagoY = archipelagoYBig + archipelagoYSmall;

    // -- DESERT --
    float desertY = glm::mix(139.f, 165.f, Noise::fbm(x * 0.003f + 1593.2f, z * 0.003f + 234.3f));

    float humidsBlend = glm::mix(grassLandY, archipelagoY, heightBlend);
    float drysBlend = glm::mix(desertY, mountainY, heightBlend);
    float fullBlend = glm::mix(drysBlend, humidsBlend, humidityBlend);

    float terrainHeightWithOcean = glm::mix(fullBlend, 114.f, oceanWeight);

    return glm::floor(terrainHeightWithOcean);
}

int Chunk::getTerrainHeight(int x, int z)
{
    auto blends = getHeightHumidityBlend(x, z);
    float oceanWeight = getOceanWeight(x, z);
    return getTerrainHeight(x, z, blends.first, blends.second, oceanWeight);
}

Biome Chunk::getBiome(float height, float humidity, float oceanWeight)
{
    if (oceanWeight > 0.1f) {
        return OCEAN;
    }
    if (height > 0.5f && humidity > 0.5f) {
        return ARCHIPELAGO;
    }
    if (height < 0.5f && humidity > 0.5f) {
        return GRASS_LANDS;
    }
    if (height < 0.5 && humidity < 0.5) {
        return DESERT;
    }
    // if (height > 0.5 && humidity < 0.5) {
    // would check, but we need a default return case LOL
    return MOUNTAINS;
}

Biome Chunk::getBiome(int x, int z)
{
    auto blends = getHeightHumidityBlend(x, z);
    float oceanWeight = getOceanWeight(x, z);
    return getBiome(blends.first, blends.second, oceanWeight);
}

std::vector<std::pair<glm::ivec2, float>> Chunk::getVoronoiPoints(int sx, int sz, int ex, int ez, float cellSize, float frequency)
{
    std::vector<std::pair<glm::ivec2, float>> points;

    int xStartCell = glm::floor(sx * 1.f / cellSize);
    int zStartCell = glm::floor(sz * 1.f / cellSize);

    int xEndCell = glm::floor(ex * 1.f / cellSize);
    int zEndCell = glm::floor(ez * 1.f / cellSize);

    for (int xCell = xStartCell; xCell <= xEndCell; ++xCell) {
        for (int zCell = zStartCell; zCell <= zEndCell; ++zCell) {
            float hasPointChance = Noise::rand(glm::vec2(xCell, zCell));

            // skip this one if rand value doesn't hit frequency probability
            if (hasPointChance > frequency) {
                continue;
            }

            float xFrac = Noise::rand(glm::vec2(xCell * 1235.231f, zCell * 631.613f));
            float zFrac = Noise::rand(glm::vec2(xCell * 838.513f, zCell * 351.345f));

            float randValue = Noise::rand(glm::vec2(xCell * 823.156f, zCell * 235.456f));

            int xPos = glm::floor((xCell * 1.f + xFrac) * cellSize);
            int zPos = glm::floor((zCell * 1.f + zFrac) * cellSize);

            // add our point, as long as it's in bounds.
            if (xPos >= sx && xPos < ex && zPos >= sz && zPos < ez) {
                points.push_back(std::make_pair(glm::ivec2(xPos, zPos), randValue));
            }
        }
    }

    return points;
}

std::vector<uPtr<Structure>> Chunk::getStructures()
{
    std::vector<uPtr<Structure>> structures;

    // pyramids
    int pyramidRadius = 40;
    auto pyramids = getVoronoiPoints(X - pyramidRadius, Z - pyramidRadius, X + WIDTH + pyramidRadius, Z + WIDTH + pyramidRadius, 160, 0.4f);
    for (auto& pData : pyramids) {
        Biome biome = getBiome(pData.first.x, pData.first.y);
        if (biome != DESERT) continue;

        structures.push_back(mkU<Pyramid>(pData.first, pData.second));
    }

    // trees
    int treeRadius = 4;
    auto trees = getVoronoiPoints(X - treeRadius, Z - treeRadius, X + WIDTH + treeRadius, Z + WIDTH + treeRadius, 10, 0.14f);
    for (auto& tData : trees) {
        Biome biome = getBiome(tData.first.x, tData.first.y);
        if (biome != GRASS_LANDS) continue;

        structures.push_back(mkU<Tree>(tData.first, tData.second));
    }

    // ice spikes
    int iceSpikeRadius = 2;
    auto iceSpikes = getVoronoiPoints(X - iceSpikeRadius, Z - iceSpikeRadius, X + WIDTH + iceSpikeRadius, Z + WIDTH + iceSpikeRadius, 13, 0.25f);
    for (auto& spikeData : iceSpikes) {
        Biome biome = getBiome(spikeData.first.x, spikeData.first.y);
        if (biome != MOUNTAINS) continue;

        structures.push_back(mkU<IceSpike>(spikeData.first, spikeData.second));
    }

    // lookouts
    int lookoutRadius = 4;
    auto lookouts = getVoronoiPoints(X - lookoutRadius, Z - lookoutRadius, X + WIDTH + lookoutRadius, Z + WIDTH + lookoutRadius, 16, 0.22f);
    for (auto& lookout : lookouts) {
        Biome biome = getBiome(lookout.first.x, lookout.first.y);
        if (biome != ARCHIPELAGO) continue;

        structures.push_back(mkU<Lookout>(lookout.first, lookout.second));
    }

    return structures;
}

const std::unordered_map<Direction, Direction, EnumHash> Chunk::oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

const std::unordered_map<Direction, std::array<int, 3>> Chunk::directionVector {
    {XPOS, {1, 0, 0}},
    {XNEG, {-1, 0, 0}},
    {YPOS, {0, 1, 0}},
    {YNEG, {0, -1, 0}},
    {ZPOS, {0, 0, 1}},
    {ZNEG, {0, 0, -1}},
};

const int Chunk::HEIGHT = 256,
          Chunk::WIDTH = 16;

bool Chunk::isSolid(BlockType block)
{
    return (blockFaceUVs.find(block) != blockFaceUVs.end() &&
            blockFaceUVs.at(block).at(XPOS).z == 1) ;
}

bool Chunk::isTransparent(BlockType block) {
    return (blockFaceUVs.find(block) != blockFaceUVs.end() &&
            blockFaceUVs.at(block).at(XPOS).z < 1 && blockFaceUVs.at(block).at(XPOS).z > 0) ;
}

void Chunk::createVBOdata()
{
    std::vector<GLuint> idx_o, idx_t;
    std::vector<glm::vec4> combined_o, combined_t;

    // fill vectors
    getInterleavedVBOdata(idx_o, combined_o, idx_t, combined_t);
    bufferInterleavedVBOdata(idx_o, combined_o, idx_t, combined_t);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + WIDTH * y + WIDTH * HEIGHT * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

std::vector<Chunk*> Chunk::getNeighbors()
{
    std::vector<Chunk*> out;
    for (auto& c : m_neighbors)
    {
        if (c.second) {
            out.push_back(c.second);
        }
    }
    return out;
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + WIDTH * y + WIDTH * HEIGHT * z) = t;
}

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

const std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec4, EnumHash>> Chunk::blockFaceUVs {
    {GRASS, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(3.f, 15.f, 1, 0)},
                                                               {XNEG, glm::vec4(3.f, 15.f, 1, 0)},
                                                               {ZPOS, glm::vec4(3.f, 15.f, 1, 0)},
                                                               {ZNEG, glm::vec4(3.f, 15.f, 1, 0)},
                                                               {YPOS, glm::vec4(8.f, 13.f, 1, 0)},
                                                               {YNEG, glm::vec4(2.f, 15.f, 1, 0)}}},
    {DIRT, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(2.f, 15.f, 1, 0)},
                                                              {XNEG, glm::vec4(2.f, 15.f, 1, 0)},
                                                              {ZPOS, glm::vec4(2.f, 15.f, 1, 0)},
                                                              {ZNEG, glm::vec4(2.f, 15.f, 1, 0)},
                                                              {YPOS, glm::vec4(2.f, 15.f, 1, 0)},
                                                              {YNEG, glm::vec4(2.f, 15.f, 1, 0)}}},
    {STONE, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(1.f, 15.f, 1, 0)},
                                                               {XNEG, glm::vec4(1.f, 15.f, 1, 0)},
                                                               {ZPOS, glm::vec4(1.f, 15.f, 1, 0)},
                                                               {ZNEG, glm::vec4(1.f, 15.f, 1, 0)},
                                                               {YPOS, glm::vec4(1.f, 15.f, 1, 0)},
                                                               {YNEG, glm::vec4(1.f, 15.f, 1, 0)}}},
    {COBBLESTONE, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(0.f, 14.f, 1, 0)},
                                                                     {XNEG, glm::vec4(0.f, 14.f, 1, 0)},
                                                                     {ZPOS, glm::vec4(0.f, 14.f, 1, 0)},
                                                                     {ZNEG, glm::vec4(0.f, 14.f, 1, 0)},
                                                                     {YPOS, glm::vec4(0.f, 14.f, 1, 0)},
                                                                     {YNEG, glm::vec4(0.f, 14.f, 1, 0)}}},
    {SNOW, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(2.f, 11.f, 1, 0)},
                                                              {XNEG, glm::vec4(2.f, 11.f, 1, 0)},
                                                              {ZPOS, glm::vec4(2.f, 11.f, 1, 0)},
                                                              {ZNEG, glm::vec4(2.f, 11.f, 1, 0)},
                                                              {YPOS, glm::vec4(2.f, 11.f, 1, 0)},
                                                              {YNEG, glm::vec4(2.f, 11.f, 1, 0)}}},
    {BEDROCK, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(1.f, 14.f, 1, 0)},
                                                                 {XNEG, glm::vec4(1.f, 14.f, 1, 0)},
                                                                 {ZPOS, glm::vec4(1.f, 14.f, 1, 0)},
                                                                 {ZNEG, glm::vec4(1.f, 14.f, 1, 0)},
                                                                 {YPOS, glm::vec4(1.f, 14.f, 1, 0)},
                                                                 {YNEG, glm::vec4(1.f, 14.f, 1, 0)}}},
    {WOOD, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(4.f, 14.f, 1, 0)},
                                                              {XNEG, glm::vec4(4.f, 14.f, 1, 0)},
                                                              {ZPOS, glm::vec4(4.f, 14.f, 1, 0)},
                                                              {ZNEG, glm::vec4(4.f, 14.f, 1, 0)},
                                                              {YPOS, glm::vec4(5.f, 14.f, 1, 0)},
                                                              {YNEG, glm::vec4(5.f, 14.f, 1, 0)}}},
    {LEAF, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(5.f, 12.f, 1, 0)},
                                                              {XNEG, glm::vec4(5.f, 12.f, 1, 0)},
                                                              {ZPOS, glm::vec4(5.f, 12.f, 1, 0)},
                                                              {ZNEG, glm::vec4(5.f, 12.f, 1, 0)},
                                                              {YPOS, glm::vec4(5.f, 12.f, 1, 0)},
                                                              {YNEG, glm::vec4(5.f, 12.f, 1, 0)}}},
    {ICE, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(3.f, 11.f, 0.5f, 0)},
                                                             {XNEG, glm::vec4(3.f, 11.f, 0.5f, 0)},
                                                             {ZPOS, glm::vec4(3.f, 11.f, 0.5f, 0)},
                                                             {ZNEG, glm::vec4(3.f, 11.f, 0.5f, 0)},
                                                             {YPOS, glm::vec4(3.f, 11.f, 0.5f, 0)},
                                                             {YNEG, glm::vec4(3.f, 11.f, 0.5f, 0)}}},
    {SAND, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(2.f, 14.f, 1.f, 0)},
                                                              {XNEG, glm::vec4(2.f, 14.f, 1.f, 0)},
                                                              {ZPOS, glm::vec4(2.f, 14.f, 1.f, 0)},
                                                              {ZNEG, glm::vec4(2.f, 14.f, 1.f, 0)},
                                                              {YPOS, glm::vec4(2.f, 14.f, 1.f, 0)},
                                                              {YNEG, glm::vec4(2.f, 14.f, 1.f, 0)}}},
    {SANDSTONE, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(0.f, 3.f, 1.f, 0)},
                                                                   {XNEG, glm::vec4(0.f, 3.f, 1.f, 0)},
                                                                   {ZPOS, glm::vec4(0.f, 3.f, 1.f, 0)},
                                                                   {ZNEG, glm::vec4(0.f, 3.f, 1.f, 0)},
                                                                   {YPOS, glm::vec4(0.f, 4.f, 1.f, 0)},
                                                                   {YNEG, glm::vec4(0.f, 4.f, 1.f, 0)}}},
    {WATER, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(14.f, 3.f, 0.5f, 1)},
                                                               {XNEG, glm::vec4(14.f, 3.f, 0.5f, 1)},
                                                               {ZPOS, glm::vec4(14.f, 3.f, 0.5f, 1)},
                                                               {ZNEG, glm::vec4(14.f, 3.f, 0.5f, 1)},
                                                               {YPOS, glm::vec4(14.f, 3.f, 0.5f, 1)},
                                                               {YNEG, glm::vec4(14.f, 3.f, 0.5f, 1)}}},
    {LAVA, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(14.f, 1.f, 0.5f, 1)},
                                                              {XNEG, glm::vec4(14.f, 1.f, 0.5f, 1)},
                                                              {ZPOS, glm::vec4(14.f, 1.f, 0.5f, 1)},
                                                              {ZNEG, glm::vec4(14.f, 1.f, 0.5f, 1)},
                                                              {YPOS, glm::vec4(14.f, 1.f, 0.5f, 1)},
                                                              {YNEG, glm::vec4(14.f, 1.f, 0.5f, 1)}}}
};

const std::array<BlockFaceData, 6> Chunk::blockFaces {
    BlockFaceData( XPOS, glm::vec3(1, 0, 0), VertexData(glm::vec4(1, 0, 1, 1), glm::vec2(0, 0)),
                                             VertexData(glm::vec4(1, 0, 0, 1), glm::vec2(BLK_UV, 0)),
                                             VertexData(glm::vec4(1, 1, 0, 1), glm::vec2(BLK_UV, BLK_UV)),
                                             VertexData(glm::vec4(1, 1, 1, 1), glm::vec2(0, BLK_UV))),

    BlockFaceData( XNEG, glm::vec3(-1, 0, 0), VertexData(glm::vec4(0, 0, 0, 1), glm::vec2(0, 0)),
                                              VertexData(glm::vec4(0, 0, 1, 1), glm::vec2(BLK_UV, 0)),
                                              VertexData(glm::vec4(0, 1, 1, 1), glm::vec2(BLK_UV, BLK_UV)),
                                              VertexData(glm::vec4(0, 1, 0, 1), glm::vec2(0, BLK_UV))),

    BlockFaceData( ZPOS, glm::vec3(0, 0, 1), VertexData(glm::vec4(0, 0, 1, 1), glm::vec2(0, 0)),
                                             VertexData(glm::vec4(1, 0, 1, 1), glm::vec2(BLK_UV, 0)),
                                             VertexData(glm::vec4(1, 1, 1, 1), glm::vec2(BLK_UV, BLK_UV)),
                                             VertexData(glm::vec4(0, 1, 1, 1), glm::vec2(0, BLK_UV))),

    BlockFaceData( ZNEG, glm::vec3(0, 0, -1), VertexData(glm::vec4(1, 0, 0, 1), glm::vec2(0, 0)),
                                              VertexData(glm::vec4(0, 0, 0, 1), glm::vec2(BLK_UV, 0)),
                                              VertexData(glm::vec4(0, 1, 0, 1), glm::vec2(BLK_UV, BLK_UV)),
                                              VertexData(glm::vec4(1, 1, 0, 1), glm::vec2(0, BLK_UV))),

    BlockFaceData( YPOS, glm::vec3(0, 1, 0), VertexData(glm::vec4(0, 1, 1, 1), glm::vec2(0, 0)),
                                             VertexData(glm::vec4(1, 1, 1, 1), glm::vec2(BLK_UV, 0)),
                                             VertexData(glm::vec4(1, 1, 0, 1), glm::vec2(BLK_UV, BLK_UV)),
                                             VertexData(glm::vec4(0, 1, 0, 1), glm::vec2(0, BLK_UV))),

    BlockFaceData( YNEG, glm::vec3(0, -1, 0), VertexData(glm::vec4(0, 0, 0, 1), glm::vec2(0, 0)),
                                              VertexData(glm::vec4(1, 0, 0, 1), glm::vec2(BLK_UV, 0)),
                                              VertexData(glm::vec4(1, 0, 1, 1), glm::vec2(BLK_UV, BLK_UV)),
                                              VertexData(glm::vec4(0, 0, 1, 1), glm::vec2(0, BLK_UV))),
};

void Chunk::generateFace(std::vector<GLuint>& idx, std::vector<glm::vec4>& combined,
                         glm::vec3& pos, const BlockFaceData& faceData, BlockType blockType)
{
    // there are 6 indices, but 4 vertices per block face
    int currentIdx = idx.size() / 6 * 4;
    // add index
    idx.push_back(currentIdx);
    idx.push_back(currentIdx + 1);
    idx.push_back(currentIdx + 2);
    idx.push_back(currentIdx);
    idx.push_back(currentIdx + 2);
    idx.push_back(currentIdx + 3);

    std::vector<glm::vec2> offsets;
    offsets.push_back(glm::vec2(0, 0));
    offsets.push_back(glm::vec2(1/16.f, 0));
    offsets.push_back(glm::vec2(1/16.f, 1/16.f));
    offsets.push_back(glm::vec2(0, 1/16.f));

    for (unsigned int i = 0; i < faceData.verts.size(); i++) {
        combined.push_back(faceData.verts[i].pos + glm::vec4(pos, 0));
        // add normal
        combined.push_back(glm::vec4(faceData.normal, 0));
        // add block color
        glm::vec4 uvData = blockFaceUVs.at(blockType).at(faceData.direction) ;
        glm::vec4 uv = glm::vec4(uvData.x/16.f + offsets[i][0], uvData.y/16.f + offsets[i][1], uvData.z, uvData.w) ;
        combined.push_back(uv);
    }
}

void Chunk::getInterleavedVBOdata(std::vector<GLuint>& idx_o, std::vector<glm::vec4>& combined_o,
                                  std::vector<GLuint>& idx_t, std::vector<glm::vec4>& combined_t)
{
    // iterate through all blocks
    for (int x = 0; x < WIDTH; ++x)
    {
        for (int y = 0; y < HEIGHT; ++y)
        {
            for (int z = 0; z < WIDTH; ++z)
            {
                // if empty, we don't care
                BlockType blockType = getBlockAt(x, y, z);
                if (!isSolid(blockType) && !isTransparent(blockType)) { continue; }

                // use transparent or opaque depending on transparency
                std::vector<GLuint>& idx = isTransparent(blockType) ? idx_t : idx_o;
                std::vector<glm::vec4>& combined = isTransparent(blockType) ? combined_t : combined_o;

                // if solid, go through faces and check for solid block
                for (auto& faceData : blockFaces)
                {
                    // get our offset position
                    auto& offset = directionVector.at(faceData.direction);
                    int posX = x + offset[0];
                    int posY = y + offset[1];
                    int posZ = z + offset[2];

                    BlockType neighborBlockType = EMPTY;

                    // if we're OOB in x or z direction, get neighbor chunk
                    if (posX < 0 || posX >= WIDTH || posZ < 0 || posZ >= WIDTH)
                    {
                        auto& neighborChunk = m_neighbors.at(faceData.direction);

                        // if neighbor chunk DNE, set block type to stone
                        // TODO: update this check to reflect how we actually know if chunks have generated blocks
                        if (neighborChunk == nullptr)
                        { neighborBlockType = STONE; }

                        else // chunk exists - get neighbor block in other chunk
                        {
                            neighborBlockType = neighborChunk->getBlockAt((posX + WIDTH) % WIDTH, posY, (posZ + WIDTH) % WIDTH);
                        }
                    }
                    else // inside XZ bounds
                    {
                        // if we're in bounds in Y we're good, get neighbor block
                        if (posY >= 0 && posY < HEIGHT)
                        {
                            neighborBlockType = getBlockAt(posX, posY, posZ);
                        }
                        // otherwise we default to EMPTY
                    }

                    // if neighbor is solid, never add face
                    if (isSolid(neighborBlockType))
                    {
                        continue;
                    }

                    // if we're transparent, add face only if neighbor is empty
                    if (isTransparent(blockType) &&
                            (isSolid(neighborBlockType) || isTransparent(neighborBlockType)))
                    {
                        continue;
                    }

                    // generate the face!!
                    glm::vec3 pos = glm::vec3(x, y, z);
                    generateFace(idx, combined, pos, faceData, blockType);
                }
            }
        }
    }
}

void Chunk::bufferInterleavedVBOdata(std::vector<GLuint>& idxOpq, std::vector<glm::vec4>& combinedOpq,
                                     std::vector<GLuint>& idxTra, std::vector<glm::vec4>& combinedTra)
{
    m_countOpq = idxOpq.size();
    m_countTra = idxTra.size();

    generateIdxOpq();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxOpq.size() * sizeof(GLuint), idxOpq.data(), GL_STATIC_DRAW);
    generatePosOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosOpq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, combinedOpq.size() * sizeof(glm::vec4), combinedOpq.data(), GL_STATIC_DRAW);

    generateIdxTra();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxTra);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxTra.size() * sizeof(GLuint), idxTra.data(), GL_STATIC_DRAW);
    generatePosTra();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosTra);
    mp_context->glBufferData(GL_ARRAY_BUFFER, combinedTra.size() * sizeof(glm::vec4), combinedTra.data(), GL_STATIC_DRAW);

    isBuffered = true;
}

ChunkVBOdata::ChunkVBOdata(Chunk* c) : mp_chunk(c),
    m_vboDataOpaque{}, m_vboDataTransparent{},
    m_idxDataOpaque{}, m_idxDataTransparent{}
{}
