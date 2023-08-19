#include "raindrop.h"

Raindrop::Raindrop(OpenGLContext *context) : Drawable(context)
{}

void Raindrop::createVBOdata()
{
    GLuint idx[6]{0, 1, 2, 0, 2, 3};
    glm::vec4 vert_pos[4] {glm::vec4(-0.5f, -5.f, 1.f, 1.f),
                           glm::vec4(0.5f, -5.f, 1.f, 1.f),
                           glm::vec4(0.5f, 15.f, 1.f, 1.f),
                           glm::vec4(-0.5f, 15.f, 1.f, 1.f)};

    glm::vec2 vert_UV[4] {glm::vec2(1.f, 0.f),
                          glm::vec2(0.f, 0.f),
                          glm::vec2(0.f, 1.f),
                          glm::vec2(1.f, 1.f)};

    m_countTra = 6;

    // Create a VBO on our GPU and store its handle in bufIdx
    generateIdxTra();
    // Tell OpenGL that we want to perform subsequent operations on the VBO referred to by bufIdx
    // and that it will be treated as an element array buffer (since it will contain triangle indices)
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxTra);
    // Pass the data stored in cyl_idx into the bound buffer, reading a number of bytes equal to
    // CYL_IDX_COUNT multiplied by the size of a GLuint. This data is sent to the GPU to be read by shader programs.
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);

    // The next few sets of function calls are basically the same as above, except bufPos and bufNor are
    // array buffers rather than element array buffers, as they store vertex attributes like position.
    generatePosTra();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosTra);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_pos, GL_STATIC_DRAW);
    generateUVTra();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUVTra);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), vert_UV, GL_STATIC_DRAW);
}

void Raindrop::drawPlanes(ShaderProgram* rainShader, ShaderProgram* snowShader, Player* player) {
    glm::vec3 cameraBlock = glm::floor(player->mcr_camera.mcr_position);
    cameraBlock.y = player->mcr_camera.mcr_position.y;
    glm::mat4 baseTransform = glm::translate(glm::mat4(), cameraBlock);

    ShaderProgram* shader = player->mcr_position.y >= SNOW_HEIGHT ? snowShader : rainShader;
    int texSlot = player->mcr_position.y >= SNOW_HEIGHT ? 2 : 1;

    for (auto& transform : planeTransforms) {
        shader->setModelMatrix(baseTransform * transform);
        shader->drawTra(*this, texSlot);
    }
}

const std::array<glm::mat4, 12> Raindrop::planeTransforms {
    glm::translate(glm::mat4(), glm::vec3(0.5, 0, 1)) * glm::rotate(glm::mat4(), 0.f, glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0.5, 0, 0)) * glm::rotate(glm::mat4(), glm::radians(180.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(1, 0, 0.5)) * glm::rotate(glm::mat4(), glm::radians(90.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0, 0, 0.5)) * glm::rotate(glm::mat4(), glm::radians(-90.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0.5, 0, 0.5)) * glm::rotate(glm::mat4(), glm::radians(45.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0.5, 0, 0.5)) * glm::rotate(glm::mat4(), glm::radians(135.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0.5, 0, 0.5)) * glm::rotate(glm::mat4(), glm::radians(225.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0.5, 0, 0.5)) * glm::rotate(glm::mat4(), glm::radians(315.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0.5, 0, 0)) * glm::rotate(glm::mat4(), 0.f, glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0, 0, 0.5)) * glm::rotate(glm::mat4(), glm::radians(90.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(0.5, 0, 1)) * glm::rotate(glm::mat4(), glm::radians(180.f), glm::vec3(0, 1, 0)),
    glm::translate(glm::mat4(), glm::vec3(1, 0, 0.5)) * glm::rotate(glm::mat4(), glm::radians(-90.f), glm::vec3(0, 1, 0)),
};
