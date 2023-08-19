#include "terrain.h"
#include "workers.h"
#include <stdexcept>
#include <QThreadPool>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context), m_newChunkTimer(0.499f)
{}

Terrain::~Terrain() {
    for (auto& chunkPair : m_chunks) {
        // do we need this?
        chunkPair.second->destroyVBOdata();
    }
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= Chunk::HEIGHT) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / (float) Chunk::WIDTH) * Chunk::WIDTH,
                                          floor(z / (float) Chunk::WIDTH) * Chunk::WIDTH);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk while getting!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / (float) Chunk::WIDTH));
    int zFloor = static_cast<int>(glm::floor(z / (float) Chunk::WIDTH));
    return m_chunks.find(toKey(Chunk::WIDTH * xFloor, Chunk::WIDTH * zFloor)) != m_chunks.end();
}

uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / (float) Chunk::WIDTH));
    int zFloor = static_cast<int>(glm::floor(z / (float) Chunk::WIDTH));
    return m_chunks[toKey(Chunk::WIDTH * xFloor, Chunk::WIDTH * zFloor)];
}

const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / (float) Chunk::WIDTH));
    int zFloor = static_cast<int>(glm::floor(z / (float) Chunk::WIDTH));
    return m_chunks.at(toKey(Chunk::WIDTH * xFloor, Chunk::WIDTH * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / (float) Chunk::WIDTH) * Chunk::WIDTH,
                                          floor(z / (float) Chunk::WIDTH) * Chunk::WIDTH);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk while setting!");
    }
}

void Terrain::initializeNearbyChunks(int x, int z, int chunkDistance, bool init)
{
    int xFloor = static_cast<int>(glm::floor(x / (float) Chunk::WIDTH));
    int zFloor = static_cast<int>(glm::floor(z / (float) Chunk::WIDTH));

    std::unordered_set<Chunk*> chunksToUpdate;

    for (int chunkOffsetX = -chunkDistance;
         chunkOffsetX <= chunkDistance; ++chunkOffsetX)
    {
        for (int chunkOffsetZ = -chunkDistance;
             chunkOffsetZ <= chunkDistance; ++chunkOffsetZ)
        {
            if(!hasChunkAt((xFloor + chunkOffsetX) * Chunk::WIDTH, (zFloor + chunkOffsetZ) * Chunk::WIDTH))
            {
                Chunk* newChunk = instantiateChunkAt((xFloor + chunkOffsetX) * Chunk::WIDTH,
                                                     (zFloor + chunkOffsetZ) * Chunk::WIDTH, init);

                chunksToUpdate.insert(newChunk);
                auto neighbors = newChunk->getNeighbors();
                for (auto& c : neighbors) {
                    chunksToUpdate.insert(c);
                }
            }
        }
    }

    for (auto* chunk : chunksToUpdate) {
        chunk->createVBOdata();
    }
}

void Terrain::createAllChunkVBOdata()
{
    for (auto& chunkPair : m_chunks)
    {
        chunkPair.second->createVBOdata();
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z, bool init) {
    uPtr<Chunk> chunk;
    chunk = mkU<Chunk>(mp_context, x, z);
    if (init) {
        chunk->generateTerrain();
    }
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + Chunk::WIDTH)) {
        auto &chunkNorth = m_chunks[toKey(x, z + Chunk::WIDTH)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - Chunk::WIDTH)) {
        auto &chunkSouth = m_chunks[toKey(x, z - Chunk::WIDTH)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + Chunk::WIDTH, z)) {
        auto &chunkEast = m_chunks[toKey(x + Chunk::WIDTH, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - Chunk::WIDTH, z)) {
        auto &chunkWest = m_chunks[toKey(x - Chunk::WIDTH, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram) {
    std::list<Chunk*> chunks;

    for(int x = minX; x < maxX; x += Chunk::WIDTH)
    {
        for(int z = minZ; z < maxZ; z += Chunk::WIDTH)
        {
            int xFloor = static_cast<int>(glm::floor(x / (float) Chunk::WIDTH));
            int zFloor = static_cast<int>(glm::floor(z / (float) Chunk::WIDTH));

            if (m_chunks.find(toKey(xFloor * Chunk::WIDTH, zFloor * Chunk::WIDTH)) == m_chunks.end())
            { continue; }

            const uPtr<Chunk> &chunk = m_chunks.at(toKey(xFloor * Chunk::WIDTH, zFloor * Chunk::WIDTH));

            if (!chunk->isBuffered) continue;

            chunks.push_back(chunk.get());
        }
    }

    // draw opaque faces, with backface culling
    glEnable(GL_CULL_FACE);
    for (auto& chunk : chunks) {
        glm::mat4 matrix {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0},
                     {chunk->X, 0, chunk->Z, 1}};

        shaderProgram->setModelMatrix(matrix);
        shaderProgram->drawInterleavedOpq(*chunk);
    }

    // draw transparent faces, without backface culling
    glDisable(GL_CULL_FACE);
    for (auto& chunk : chunks) {
        glm::mat4 matrix {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0},
                     {chunk->X, 0, chunk->Z, 1}};

        shaderProgram->setModelMatrix(matrix);
        shaderProgram->drawInterleavedTra(*chunk);
    }

    // turn it back on for later LOL
    glEnable(GL_CULL_FACE);
}

void Terrain::multithread(glm::vec3 pos, glm::vec3 prevPos, float dT) {
    m_newChunkTimer += dT;
    if (m_newChunkTimer >= 0.5f) {
        tryNewChunk(pos, prevPos);
        m_newChunkTimer = 0.f;
    }
    checkThreadResults();
}

void Terrain::tryNewChunk(glm::vec3 pos, glm::vec3 prevPos) {
    // Find the 64 x 64 zone the player is on
    glm::ivec2 curr(64.f * floor(pos.x/64.f), 64.f * floor(pos.z/64.f));
    glm::ivec2 prev(64.f * floor(prevPos.x/64.f), 64.f * floor(prevPos.z/64.f));
    // Figure out which zones border this zone and the previous zone
    QSet<long long> borderingCurr = borderingZone(curr, TERRAIN_CREATE_RADIUS, false);
    QSet<long long> borderingPrev = borderingZone(prev, TERRAIN_CREATE_RADIUS, false);
    // If previous zones are no longer there, remove their vbo data
    for(long long zone : borderingPrev) {
        if (!borderingCurr.contains(zone)) {
            glm::ivec2 coord = toCoords(zone);
            for (int x = coord.x; x < coord.x + 64; x += 16) {
                for(int z = coord.y; z < coord.y + 64; z += 16) {
                    auto& c = getChunkAt(x, z);
                    c->destroyVBOdata();
                    c->isBuffered = false;
                }
            }
        }
    }
    // Figure out if the current zones need VBO data or Block data
    for (long long zone: borderingCurr) {
        if (m_chunks.find(zone) != m_chunks.end()) {
            if (!borderingPrev.contains(zone)) {
                glm::ivec2 coord = toCoords(zone);
                for (int x = coord.x; x < coord.x + 64; x += 16) {
                    for(int z = coord.y; z < coord.y + 64; z += 16) {
                        auto& c = getChunkAt(x, z);
                        createVBOWorker(c.get());
                    }
                }
            }
        } else {
            createBDWorker(zone);
        }
    }
}

QSet<long long> Terrain::borderingZone(glm::ivec2 coords, int radius, bool atEdge) {
    int radiusScale = radius * 64;
    QSet<long long> result;
    // Adds Zones only at the radius
    if (atEdge) {
        for (int i = -radiusScale; i <= radiusScale; i += 64) {
            result.insert(toKey(coords.x + radiusScale, coords.y + i));
            result.insert(toKey(coords.x - radiusScale, coords.y + i));
            result.insert(toKey(coords.x + i, coords.y + radiusScale));
            result.insert(toKey(coords.x + i, coords.y + radiusScale));
        }
    // Adds Zones upto and including the radius
    } else {
        for (int i = -radiusScale; i <= radiusScale; i += 64) {
            for (int j = -radiusScale; j <= radiusScale; j += 64) {
                result.insert(toKey(coords.x + i, coords.y + j));
            }
        }
    }
    return result;
}

void Terrain::checkThreadResults() {
    // First, send chunks processed by BlockWorkers to VBOWorkers
    if (!m_blockDataChunks.empty()) {
        m_blockDataChunksLock.lock();
        for (auto &c : m_blockDataChunks) {
            for (auto& n : c->getNeighbors()) {
                m_blockDataChunks.insert(n);
            }
        }
        createVBOWorkers(m_blockDataChunks);
        m_blockDataChunks.clear();
        m_blockDataChunksLock.unlock();
    }
    // Second, take the chunks that have VBO data and send data to GPU
    m_VBODataChunksLock.lock();
    for (auto& data: m_vboDataChunks) {
        data.mp_chunk->bufferInterleavedVBOdata(data.m_idxDataOpaque, data.m_vboDataOpaque,
                                                data.m_idxDataTransparent, data.m_vboDataTransparent);
    }
    m_vboDataChunks.clear();
    m_VBODataChunksLock.unlock();
}

void Terrain::CreateTestScene()
{
    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    for(int x = 0; x < 64; x += Chunk::WIDTH) {
        for(int z = 0; z < 64; z += Chunk::WIDTH) {
            instantiateChunkAt(x, z, false);
        }
    }
    // Tell our existing terrain set that
    // the "generated terrain zone" at (0,0)
    // now exists.
    m_generatedTerrain.insert(toKey(0, 0));


    // Create the basic terrain floor

   // Add "walls" for collision testing
   for(int x = 0; x < 64; ++x) {
       setBlockAt(x, 129, 0, GRASS);
       setBlockAt(x, 130, 0, GRASS);
       setBlockAt(x, 129, 63, GRASS);
       setBlockAt(0, 130, x, GRASS);
   }

   // Add a central column
   for(int y = 129; y < 140; ++y) {
       setBlockAt(32, y, 32, GRASS);
   }

    // create all chunk vbo data LOL
    createAllChunkVBOdata();
}

void Terrain::createBDWorkers(const QSet<long long> &zones) {
    for (long long zone: zones) {
        createBDWorker(zone);
    }
}

void Terrain::createBDWorker(long long zone) {
    std::vector<Chunk*> toDo;
    int x = toCoords(zone).x;
    int z = toCoords(zone).y;
    for (int i = x; i < x + 64; i += 16) {
        for (int j = z; j < z + 64; j += 16) {
            toDo.push_back(instantiateChunkAt(i, j, false));
        }
    }
    BDWorker *worker = new BDWorker(x, z, toDo,
                                    &m_blockDataChunks, &m_blockDataChunksLock);
    QThreadPool::globalInstance()->start(worker);
}

void Terrain::createVBOWorkers(const std::unordered_set<Chunk*> &chunks) {
    for (Chunk* chunk: chunks) {
        createVBOWorker(chunk);
    }
}

void Terrain::createVBOWorker(Chunk* chunk) {
    VBOWorker *worker = new VBOWorker(chunk, &m_vboDataChunks, &m_VBODataChunksLock);
    QThreadPool::globalInstance()->start(worker);
}
