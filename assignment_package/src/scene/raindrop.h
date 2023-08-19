#pragma once

#include "drawable.h"
#include "shaderprogram.h"
#include "player.h"

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#define SNOW_HEIGHT 190

class Raindrop : public Drawable
{
public:
    Raindrop(OpenGLContext* context);
    virtual void createVBOdata();
    void drawPlanes(ShaderProgram* sp, ShaderProgram* sp2, Player* p);
    const static std::array<glm::mat4, 12> planeTransforms;
};


