#pragma once
#include "chunk.h"
#include <QRunnable>
#include <QMutex>
#include <unordered_set>

class BDWorker : public QRunnable {
private:
    int m_xCorner, m_zCorner;
    std::vector<Chunk*> m_chunksToDo;
    std::unordered_set<Chunk*>* mp_chunksDone;
    QMutex* mp_chunksCompletedLock;

public:
    BDWorker(int x, int z, std::vector<Chunk*> toDo,
             std::unordered_set<Chunk*>* complete, QMutex* completed);
    void run() override;

};

class VBOWorker : public QRunnable {
private:
    Chunk* mp_chunk;
    std::vector<ChunkVBOdata>* mp_VBOsCompleted;
    QMutex* mp_VBOsCompletedLock;

public:
    VBOWorker(Chunk* c, std::vector<ChunkVBOdata>* data, QMutex* lock);
    void run() override;
};
