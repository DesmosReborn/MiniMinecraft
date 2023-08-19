#ifndef MYGL_H
#define MYGL_H

#include "openglcontext.h"
#include "shaderprogram.h"
#include "framebuffer.h"
#include "scene/quad.h"
#include "scene/worldaxes.h"
#include "scene/camera.h"
#include "scene/cube.h"
#include "scene/terrain.h"
#include "scene/player.h"
#include <QDateTime>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <smartpointerhelp.h>

#include <scene/raindrop.h>


class MyGL : public OpenGLContext
{
    Q_OBJECT
private:
    Quad m_quad;            // A simple drawable mesh used for post-processing
    Quad* m_geomQuad;
    Raindrop m_rainPlane;

    ShaderProgram m_progLambert;    // A shader program that uses lambertian reflection
    ShaderProgram m_progFlat;       // A shader program that uses "flat" reflection (no shadowing at all)
    ShaderProgram m_progInstanced;  // A shader program that is designed to be compatible with instanced rendering
    ShaderProgram m_progRain;  // A shader program that is designed to be compatible with instanced rendering
    ShaderProgram m_progSky;
    ShaderProgram m_progSnow;
    ShaderProgram m_progRainPlane;
    ShaderProgram m_progSnowPlane;

    std::unordered_map<BlockType, uPtr<ShaderProgram>> m_blockShaders; // Post-processing shaders for when the player is inside some block
    FrameBuffer m_frameBuffer;  // A frame buffer that allow us to apply post-processing shaders

    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
                // Don't worry too much about this. Just know it is necessary in order to render geometry.

    Terrain m_terrain;     // All of the Chunks that currently comprise the world.
    Player m_player;       // The entity controlled by the user. Contains a camera to display what it sees as well.
    InputBundle m_inputs;  // A collection of variables to be updated in keyPressEvent, mouseMoveEvent, mousePressEvent, etc.

    QTimer m_timer;  // Timer linked to tick(). Fires approximately 60 times per second.

    std::unique_ptr<Texture> mp_texture;
    std::unique_ptr<Texture> mp_texture2;
    std::unique_ptr<Texture> mp_texture3;

    void moveMouseToCenter();  // Forces the mouse position to the screen's center. You should call this
                               // from within a mouse move event after reading the mouse movement so that
                               // your mouse stays within the screen bounds and is always read.

    ShaderProgram* getPostShader(BlockType block);

    void sendPlayerDataToGUI() const;

    long long lastTickTime;  // Last tick time, in milliseconds since epoch
    int elapsedTime;         // Elapsed (in-game) time in milliseconds
    float timeOfDay;         // Current in-game time of day, in hours (0-24)

    glm::vec2 pastWeather;

public:
    explicit MyGL(QWidget *parent = nullptr);
    ~MyGL();

    // Called once when MyGL is initialized.
    // Once this is called, all OpenGL function
    // invocations are valid (before this, they
    // will cause segfaults)
    void initializeGL() override;
    // Called whenever MyGL is resized.
    void resizeGL(int w, int h) override;
    // Called whenever MyGL::update() is called.
    // In the base code, update() is called from tick().
    void paintGL() override;

    // Called from paintGL().
    // Calls Terrain::draw().
    void renderTerrain();
    void renderDrops();

    void setInputsFalse() ;

protected:
    // Automatically invoked when the user
    // presses a key on the keyboard
    void keyPressEvent(QKeyEvent *e);
    // Automatically invoked when the user
    // moves the mouse
    void keyReleaseEvent(QKeyEvent *e) ;

    void mouseMoveEvent(QMouseEvent *e);
    // Automatically invoked when the user
    // presses a mouse button
    void mousePressEvent(QMouseEvent *e);

private slots:
    void tick(); // Slot that gets called ~60 times per second by m_timer firing.

signals:
    void sig_sendPlayerPos(QString) const;
    void sig_sendPlayerVel(QString) const;
    void sig_sendPlayerAcc(QString) const;
    void sig_sendPlayerLook(QString) const;
    void sig_sendPlayerChunk(QString) const;
    void sig_sendPlayerTerrainZone(QString) const;
};


#endif // MYGL_H
