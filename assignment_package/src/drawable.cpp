#include "drawable.h"
#include <glm_includes.h>

Drawable::Drawable(OpenGLContext* context)
    : m_countOpq(-1), m_countTra(-1),
      m_bufIdxOpq(), m_bufIdxTra(), m_bufPosOpq(), m_bufPosTra(),
      m_bufNorOpq(), m_bufNorTra(), m_bufUVOpq(),  m_bufUVTra(),
      m_idxGeneratedOpq(false), m_idxGeneratedTra(false), m_posGeneratedOpq(false), m_posGeneratedTra(false),
      m_norGeneratedOpq(false), m_norGeneratedTra(false), m_uvGeneratedOpq(false), m_uvGeneratedTra(false),
      mp_context(context)
{}

Drawable::~Drawable()
{}


void Drawable::destroyVBOdata()
{
    mp_context->glDeleteBuffers(1, &m_bufIdxOpq);
    mp_context->glDeleteBuffers(1, &m_bufIdxTra);
    mp_context->glDeleteBuffers(1, &m_bufPosOpq);
    mp_context->glDeleteBuffers(1, &m_bufPosTra);
    mp_context->glDeleteBuffers(1, &m_bufNorOpq);
    mp_context->glDeleteBuffers(1, &m_bufNorTra);
    mp_context->glDeleteBuffers(1, &m_bufUVOpq);
    mp_context->glDeleteBuffers(1, &m_bufUVTra);
    m_idxGeneratedOpq = m_posGeneratedOpq = m_norGeneratedOpq = m_uvGeneratedOpq = false;
    m_idxGeneratedTra = m_posGeneratedTra = m_norGeneratedTra = m_uvGeneratedTra = false;
    m_countOpq = m_countTra = -1;
}

GLenum Drawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int Drawable::elemCountOpq()
{
    return m_countOpq;
}

int Drawable::elemCountTra()
{
    return m_countTra;
}

void Drawable::generateIdxOpq()
{
    m_idxGeneratedOpq = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdxOpq);
}

void Drawable::generateIdxTra()
{
    m_idxGeneratedTra = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdxTra);
}

void Drawable::generatePosOpq()
{
    m_posGeneratedOpq = true;
    // Create a VBO on our GPU and store its handle in bufPos
    mp_context->glGenBuffers(1, &m_bufPosOpq);
}

void Drawable::generatePosTra()
{
    m_posGeneratedTra = true;
    // Create a VBO on our GPU and store its handle in bufPos
    mp_context->glGenBuffers(1, &m_bufPosTra);
}

void Drawable::generateNorOpq()
{
    m_norGeneratedOpq = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &m_bufNorOpq);
}

void Drawable::generateNorTra()
{
    m_norGeneratedTra = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &m_bufNorTra);
}

void Drawable::generateUVOpq()
{
    m_uvGeneratedOpq = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &m_bufUVOpq);
}

void Drawable::generateUVTra()
{
    m_uvGeneratedTra = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &m_bufUVTra);
}

bool Drawable::bindIdxOpq()
{
    if(m_idxGeneratedOpq) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    }
    return m_idxGeneratedOpq;
}

bool Drawable::bindIdxTra()
{
    if(m_idxGeneratedTra) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxTra);
    }
    return m_idxGeneratedTra;
}

bool Drawable::bindPosOpq()
{
    if(m_posGeneratedOpq){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosOpq);
    }
    return m_posGeneratedOpq;
}

bool Drawable::bindPosTra()
{
    if(m_posGeneratedTra){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosTra);
    }
    return m_posGeneratedTra;
}

bool Drawable::bindNorOpq()
{
    if(m_norGeneratedOpq){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNorOpq);
    }
    return m_norGeneratedOpq;
}

bool Drawable::bindNorTra()
{
    if(m_norGeneratedTra){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNorTra);
    }
    return m_norGeneratedTra;
}

bool Drawable::bindUVOpq()
{
    if(m_uvGeneratedOpq){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUVOpq);
    }
    return m_uvGeneratedOpq;
}

bool Drawable::bindUVTra()
{
    if(m_uvGeneratedTra){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUVTra);
    }
    return m_uvGeneratedTra;
}

InstancedDrawable::InstancedDrawable(OpenGLContext *context)
    : Drawable(context), m_numInstances(0), m_bufPosOffset(-1), m_offsetGenerated(false)
{}

InstancedDrawable::~InstancedDrawable(){}

int InstancedDrawable::instanceCount() const {
    return m_numInstances;
}

void InstancedDrawable::generateOffsetBuf() {
    m_offsetGenerated = true;
    mp_context->glGenBuffers(1, &m_bufPosOffset);
}

bool InstancedDrawable::bindOffsetBuf() {
    if(m_offsetGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosOffset);
    }
    return m_offsetGenerated;
}


void InstancedDrawable::clearOffsetBuf() {
    if(m_offsetGenerated) {
        mp_context->glDeleteBuffers(1, &m_bufPosOffset);
        m_offsetGenerated = false;
    }
}
