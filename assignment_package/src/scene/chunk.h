#pragma once

#include "smartpointerhelp.h"
#include "drawable.h"
#include "texture.h"

#include <array>
#include <unordered_map>
#include <cstddef>
#include <QMutex>

class Structure;


#define BLK_UV 0.03125f
#define BLK_UVX * 0.03125f
#define BLK_UVY * 0.03125f

//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, COBBLESTONE, WATER, SNOW, BEDROCK, WOOD, LEAF, SAND, SANDSTONE, ICE, LAVA
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

struct VertexData {
    glm::vec4 pos;
    glm::vec2 uv;
    VertexData(glm::vec4 p, glm::vec2 u);
};

struct BlockFaceData {
    Direction direction;
    glm::vec3 normal;
    std::array<VertexData, 4> verts;

    BlockFaceData(Direction dir, glm::vec3 n,
                  const VertexData &a, const VertexData &b,
                  const VertexData &c, const VertexData &d);
};

enum Biome {
    GRASS_LANDS,
    ARCHIPELAGO,
    MOUNTAINS,
    DESERT,
    OCEAN
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const;
};


// One Chunk is a WIDTH x HEIGHT x WIDTH section of the
// world, containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.
class Chunk : public Drawable
{
public:
    Chunk(OpenGLContext* context, int x, int z);
    ~Chunk();

    const int X, Z;

    // utility for working with Direction enum
    const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection;
    const static std::unordered_map<Direction, std::array<int, 3>> directionVector;

    static const int HEIGHT, WIDTH;

    static bool isSolid(BlockType block);
    static bool isTransparent(BlockType block) ;

    // stores all interleaved VBO data in pos
    void createVBOdata() override;

    void generateTerrain();
    void generateTerrainColumn(int chunkX, int chunkZ);

    // terrain generation helpers
    static std::pair<float, float> getHeightHumidityBlend(int x, int z);
    static float getOceanWeight(int x, int z);
    static int getTerrainHeight(int x, int z, float heightBlend, float humidityBlend, float oceanWeight);
    static int getTerrainHeight(int x, int z); // calls prev functions!
    static Biome getBiome(float height, float humidity, float oceanWeight);
    static Biome getBiome(int x, int z); // calls prev functions!

    // structure generation helpers
    static std::vector<std::pair<glm::ivec2, float>> getVoronoiPoints(int sx, int sz, int ex, int ez, float cellSize, float frequency);
    std::vector<uPtr<Structure>> getStructures();

    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    std::vector<Chunk*> getNeighbors();

    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;

    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    // UV positions for different block types
    const static std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec4, EnumHash>> blockFaceUVs;
    // Block face data for use in rendering, nicely packaged in an array of structs
    const static std::array<BlockFaceData, 6> blockFaces;

    // Add vertex data for one specified face to the given VBO data vector references
    static void generateFace(std::vector<GLuint>& idx, std::vector<glm::vec4>& combined,
                             glm::vec3& pos, const BlockFaceData& faceData, BlockType blockType);

    // Fills VBO data vectors with interleaved data
    void getInterleavedVBOdata(std::vector<GLuint>& idx_o, std::vector<glm::vec4>& combined_o,
                               std::vector<GLuint>& idx_t, std::vector<glm::vec4>& combined_t);

    // Buffers the given data vectors to VBOs for the GPU
    void bufferInterleavedVBOdata(std::vector<GLuint>& idx_o, std::vector<glm::vec4>& combined_o,
                                  std::vector<GLuint>& idx_t, std::vector<glm::vec4>& combined_t);
    QMutex chunkLock;

    bool isBuffered;

    friend class Terrain;
    friend class BDWorker;
    friend class VBOWorker;
};

struct ChunkVBOdata {
    Chunk* mp_chunk;
    std::vector<glm::vec4> m_vboDataOpaque, m_vboDataTransparent;
    std::vector<GLuint> m_idxDataOpaque, m_idxDataTransparent;

    ChunkVBOdata(Chunk* c);
};


























