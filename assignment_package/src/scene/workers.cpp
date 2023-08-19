#include "workers.h"
#include "noise.h"

BDWorker::BDWorker(int x, int z, std::vector<Chunk*> toDo,
                   std::unordered_set<Chunk*>* complete, QMutex* completedLock) :
    m_xCorner(x), m_zCorner(z), m_chunksToDo(toDo), mp_chunksDone(complete), mp_chunksCompletedLock(completedLock)
{}

void BDWorker::run() {
    // Construct chunks to do
    for (Chunk* c : m_chunksToDo) {
        c->generateTerrain();
    }
    mp_chunksCompletedLock->lock();
    for (Chunk* c : m_chunksToDo) {
        mp_chunksDone->insert(c);
    }
    mp_chunksCompletedLock->unlock();
}

VBOWorker::VBOWorker(Chunk* c, std::vector<ChunkVBOdata>* data, QMutex *dataLock) :
    mp_chunk(c), mp_VBOsCompleted(data), mp_VBOsCompletedLock(dataLock)
{}

void VBOWorker::run() {
    mp_chunk->chunkLock.lock();
    ChunkVBOdata c(mp_chunk);
    // call function to build VBO Data
    mp_chunk->getInterleavedVBOdata(c.m_idxDataOpaque, c.m_vboDataOpaque,
                                    c.m_idxDataTransparent, c.m_vboDataTransparent);
    mp_VBOsCompletedLock->lock();
    mp_VBOsCompleted->push_back(c);
    mp_VBOsCompletedLock->unlock();
    mp_chunk->chunkLock.unlock();
}
