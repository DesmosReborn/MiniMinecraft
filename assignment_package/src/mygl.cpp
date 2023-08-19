#include "mygl.h"
#include <glm_includes.h>
#include "noise.h"
#include <QApplication>
#include <QKeyEvent>


#define POST_TEXTURE_SLOT 0

#define MOUSE_SENSITIVITY 0.25f


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_quad(this),
      m_geomQuad(new Quad(this)), m_rainPlane(this), m_progLambert(this), m_progFlat(this), m_progInstanced(this),
      m_progRain(this), m_progSky(this), m_progSnow(this), m_progRainPlane(this), m_progSnowPlane(this), m_blockShaders(),
      m_frameBuffer(this, this->width(), this->height(), this->devicePixelRatio()), m_terrain(this),
      m_player(glm::vec3(0, 175, 0), m_terrain),
      lastTickTime(QDateTime::currentMSecsSinceEpoch()), elapsedTime(0), timeOfDay(0.f), pastWeather(0)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    // Initializes frame buffer on the GPU
    m_frameBuffer.create();

    //Create Quad instance
    m_quad.createVBOdata();
    //Create rainPlane data
    m_rainPlane.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    m_progRain.create(":/glsl/rain.vert.glsl", ":/glsl/rain.frag.glsl");
    m_progSnow.create(":/glsl/snow.vert.glsl", ":/glsl/snow.frag.glsl");

    m_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");
    m_geomQuad->createVBOdata();

    // rain and snow plane shaders
    m_progRainPlane.create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/rainpost.frag.glsl");
    m_progSnowPlane.create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/snowpost.frag.glsl");

    // Texture loading
    mp_texture = std::unique_ptr<Texture>(new Texture(this)) ;
    mp_texture->create(":/textures/minecraft_textures_all.png") ;
    mp_texture->load(0);

    mp_texture2 = std::unique_ptr<Texture>(new Texture(this)) ;
    mp_texture2->create(":/textures/rain.png") ;
    mp_texture2->load(1);

    mp_texture3 = std::unique_ptr<Texture>(new Texture(this)) ;
    mp_texture3->create(":/textures/snow.png") ;
    mp_texture3->load(2);

    // Post-processing shader map
    auto s_water = mkU<ShaderProgram>(this);
    s_water->create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/water.frag.glsl");
    m_blockShaders.insert(std::make_pair(WATER, std::move(s_water)));

    auto s_lava = mkU<ShaderProgram>(this);
    s_lava->create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/lava.frag.glsl");
    m_blockShaders.insert(std::make_pair(LAVA, std::move(s_lava)));

    auto s_default = mkU<ShaderProgram>(this);
    s_default->create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/passthrough.frag.glsl");
    m_blockShaders.insert(std::make_pair(EMPTY, std::move(s_default)));

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);
    m_terrain.initializeNearbyChunks(0, 0, 3, true);
    //m_terrain.CreateTestScene();
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    m_progInstanced.setViewProjMatrix(viewproj);
    m_progRain.setViewProjMatrix(viewproj);
    m_progSnow.setViewProjMatrix(viewproj);
    m_progSky.setViewProjMatrix(glm::inverse(viewproj));

    m_progSky.useMe();
    this->glUniform2i(m_progSky.unifDimensions, width(), height());
    this->glUniform3f(m_progSky.unifEye, m_player.mcr_camera.mcr_position.x, m_player.mcr_camera.mcr_position.y, m_player.mcr_camera.mcr_position.z);
    this->glUniform1f(m_progSky.unifWeather, pastWeather.x);

    m_progRainPlane.useMe();
    this->glUniform1f(m_progRainPlane.unifWeather, pastWeather.x);

    m_progSnowPlane.useMe();
    this->glUniform1f(m_progSnowPlane.unifWeather, pastWeather.x);

    m_progRain.useMe();
    this->glUniform1f(m_progRain.unifWeather, pastWeather.x);

    m_frameBuffer.resize(w, h, this->devicePixelRatio());
    m_frameBuffer.create();

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    int dtMillis = (int) glm::clamp((QDateTime::currentMSecsSinceEpoch() - lastTickTime) * 1.f, 0.f, 100.f);
    float dt = dtMillis * 0.001f;

    elapsedTime += dtMillis;
    m_player.tick(dt, m_inputs);
    lastTickTime = QDateTime::currentMSecsSinceEpoch();

    timeOfDay = glm::mod(timeOfDay + dt / 6.f, 24.f);

    // Does multithreading work
    m_terrain.multithread(m_player.mcr_position, m_player.getPrevPosition(), dt);

    auto& pos = m_player.mcr_camera.mcr_position;

    glm::vec3 caves = (pos + glm::vec3(0.5, 0.5, 0.2));
    caves *= 0.00000012 * elapsedTime * 0.000001;

    float weather = Noise::perlin(glm::vec2(caves.x, caves.z), caves.y);

    if (weather < 0) {
        if (pastWeather.y == 0) {
        } else {
            pastWeather.y = 0;
        }
    } else {
        if (pastWeather.y == 1) {
        } else {          
            pastWeather.y = 0;
        }
    }

    pastWeather.x = glm::clamp((pastWeather.x + 0.01f * (pastWeather.y - 0.5f)), 0.f, 1.f);

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
}

ShaderProgram* MyGL::getPostShader(BlockType block) {
    auto shader = m_blockShaders.find(block);
    if (shader == m_blockShaders.end()) {
        return m_blockShaders.find(EMPTY)->second.get();
    } else {
        return shader->second.get();
    }
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(Chunk::WIDTH * glm::ivec2(glm::floor(pPos / (float) Chunk::WIDTH)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    // normal 3d rendering to buffer pipeline
    m_frameBuffer.bindFrameBuffer();
    glViewport(0, 0, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());

    // Clear the screen so that we only see newly drawn images
    mp_texture->bind(0);
    mp_texture2->bind(1);
    mp_texture3->bind(2);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    glEnable(GL_DEPTH_TEST);

    // set sky shader in variables
    m_progSky.useMe();
    m_progSky.setViewProjMatrix(glm::inverse(viewproj));
    m_progSky.setTimeOfDay(timeOfDay);
    this->glUniform3f(m_progSky.unifEye, m_player.mcr_camera.mcr_position.x, m_player.mcr_camera.mcr_position.y, m_player.mcr_camera.mcr_position.z);
    this->glUniform1f(m_progSky.unifTime, elapsedTime);
    this->glUniform1f(m_progSky.unifWeather, pastWeather.x);

    // set rain and show plane variables
    m_progRainPlane.useMe();
    this->glUniform1f(m_progRainPlane.unifWeather, pastWeather.x);
    m_progSnowPlane.useMe();
    this->glUniform1f(m_progSnowPlane.unifWeather, pastWeather.x);

    // set rain and snow post shader variables
    m_progRain.useMe();
    this->glUniform1f(m_progRain.unifWeather, pastWeather.x);
    m_progSnow.useMe();
    this->glUniform1f(m_progSnow.unifWeather, pastWeather.x);

    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progInstanced.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progRain.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progSnow.setViewProjMatrix(m_player.mcr_camera.getViewProj());

    m_progLambert.setTime(elapsedTime);
    m_progFlat.setTime(elapsedTime);
    m_progRain.setTime(elapsedTime);
    m_progSnow.setTime(elapsedTime);

    m_progLambert.setTimeOfDay(timeOfDay);
    this->glUniform1f(m_progLambert.unifWeather, pastWeather.x);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw sky, behind everything
    glDepthRange (0.999, 1);
    m_progSky.drawOpq(*m_geomQuad, 0);
    glDepthRange (-1, 1);

    // terrain
    renderTerrain();

    // weather planes
    if (pastWeather.x != 0) {
        glDisable(GL_CULL_FACE);
        m_rainPlane.drawPlanes(&m_progRain, &m_progSnow, &m_player);
        glEnable(GL_CULL_FACE);
    }

    // world axes
    glDisable(GL_DEPTH_TEST);
    m_progFlat.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    //m_progFlat.draw(m_worldAxes);
    glEnable(GL_DEPTH_TEST);

    // post-process logic
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0, 0, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_frameBuffer.bindToTextureSlot(POST_TEXTURE_SLOT);

    // get post shader based on block we're inside of
    ShaderProgram* postShader = getPostShader(m_terrain.getBlockAt(m_player.mcr_camera.mcr_position));

    if (m_terrain.getBlockAt(m_player.mcr_camera.mcr_position) == EMPTY && pastWeather.x != 0) {
        if (m_player.mcr_position.y >= SNOW_HEIGHT) {
            postShader = &m_progSnowPlane;
        } else {
            postShader = &m_progRainPlane;
        }
    }
    // draw post shader
    postShader->setTime(elapsedTime);
    postShader->drawPostShader(m_quad, m_frameBuffer.getTextureSlot());
}

void MyGL::renderTerrain() {
    auto& pos = m_player.mcr_camera.mcr_position;
    int radius = Chunk::WIDTH * 24;
    m_terrain.draw(pos.x - radius, pos.x + radius, pos.z - radius, pos.z + radius, &m_progLambert);
}

void MyGL::keyPressEvent(QKeyEvent *e) {
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = true;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = true;
    }
    if (e->key() == Qt::Key_F) {
        m_player.setVelocity(glm::vec3(0));
        m_player.setFlight(!m_player.getFlight()) ;
    }
    if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = true;
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {

    if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = false;
    }
    if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = false;
    }
}

void MyGL::setInputsFalse() {
    m_inputs.wPressed = false;
    m_inputs.sPressed = false;
    m_inputs.aPressed = false;
    m_inputs.dPressed = false;
    m_inputs.qPressed = false;
    m_inputs.ePressed = false;
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    float sensMultiplier = MOUSE_SENSITIVITY / this->devicePixelRatio();
    float xDist = e->pos().x() - width() * 0.5f;
    float yDist = e->pos().y() - height() * 0.5f;
    m_inputs.mouseX = glm::mod((m_inputs.mouseX - sensMultiplier * xDist), 360.f);
    m_inputs.mouseY = glm::clamp(m_inputs.mouseY + sensMultiplier * yDist, -90.f, 90.f);
    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    glm::vec3 rayDirection = 4.5f * glm::normalize(m_player.mcr_camera.m_forward) ;
    glm::vec3 rayOrigin = m_player.mcr_camera.mcr_position ;
    if (e->button() == Qt::LeftButton) {
        float maxLen = sqrt(pow(rayDirection.x, 2) + pow(rayDirection.y, 2) + pow(rayDirection.z, 2)) ; // Farthest we search
        glm::ivec3 currCell = glm::ivec3(rayOrigin);
        rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

        float curr_t = 0.f;
        while(curr_t < maxLen) {
            float min_t = glm::sqrt(4.f);
            float interfaceAxis = -1; // Track axis for which t is smallest
            for(int i = 0; i < 3; ++i) { // Iterate over the three axes
                if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                    float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                    // If the player is *exactly* on an interface then
                    // they'll never move if they're looking in a negative direction
                    if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                        offset = -1.f;
                    }
                    int nextIntercept = currCell[i] + offset;
                    float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                    axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                    if(axis_t < min_t) {
                        min_t = axis_t;
                        interfaceAxis = i;
                    }
                }
            }
            if(interfaceAxis == -1) {
                throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
            }
            curr_t += min_t; // min_t is declared in slide 7 algorithm
            rayOrigin += rayDirection * min_t;
            glm::ivec3 offset = glm::ivec3(0,0,0);
            // Sets it to 0 if sign is +, -1 if sign is -
            offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
            if (!(std::isnan(rayOrigin.x) || std::isnan(rayOrigin.y) || std::isnan(rayOrigin.z))) {
                currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
                // If currCell contains something other than EMPTY, return
                // curr_t
                //std::cout << "rayOrigin: " << rayOrigin.x << " " << rayOrigin.y << " " << rayOrigin.z << std::endl ;
                //std::cout << "currCell: " << currCell.x << " " << currCell.y << " " << currCell.z << std::endl ;
                BlockType cellType = m_terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
                if(cellType != EMPTY) {
                    m_terrain.setBlockAt(currCell.x, currCell.y, currCell.z, EMPTY) ;
                    m_terrain.getChunkAt(currCell.x, currCell.z)->createVBOdata() ;
                    break;
                }
            }
        }
    } else {
        float maxLen = sqrt(pow(rayDirection.x, 2) + pow(rayDirection.y, 2) + pow(rayDirection.z, 2)) ; // Farthest we search
        glm::ivec3 currCell = glm::ivec3(rayOrigin);
        rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

        float curr_t = 0.f;
        while(curr_t < maxLen) {
            float min_t = glm::sqrt(4.f);
            float interfaceAxis = -1; // Track axis for which t is smallest
            for(int i = 0; i < 3; ++i) { // Iterate over the three axes
                if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                    float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                    // If the player is *exactly* on an interface then
                    // they'll never move if they're looking in a negative direction
                    if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                        offset = -1.f;
                    }
                    int nextIntercept = currCell[i] + offset;
                    float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                    axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                    if(axis_t < min_t) {
                        min_t = axis_t;
                        interfaceAxis = i;
                    }
                }
            }
            if(interfaceAxis == -1) {
                throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
            }
            curr_t += min_t; // min_t is declared in slide 7 algorithm
            rayOrigin += rayDirection * min_t;
            glm::ivec3 offset = glm::ivec3(0,0,0);
            // Sets it to 0 if sign is +, -1 if sign is -
            offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
            if (!(std::isnan(rayOrigin.x) || std::isnan(rayOrigin.y) || std::isnan(rayOrigin.z))) {
                currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
                // If currCell contains something other than EMPTY, return
                // curr_t
                //std::cout << "rayOrigin: " << rayOrigin.x << " " << rayOrigin.y << " " << rayOrigin.z << std::endl ;
                //std::cout << "currCell: " << currCell.x << " " << currCell.y << " " << currCell.z << std::endl ;
                BlockType cellType = m_terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
                if(cellType != EMPTY) {
                    float outDist = glm::min(maxLen, curr_t) ;
                    glm::vec3 intersect = m_player.mcr_camera.mcr_position + outDist * m_player.getForward() ;
                    glm::vec3 center = glm::vec3(currCell.x + 0.5f, currCell.y + 0.5f, currCell.z + 0.5f) ;
                    glm::vec3 offset = intersect - center ;
                    if (offset.x > offset.y && offset.x > offset.z) {
                        currCell.z += glm::sign(offset.x) ;
                    } else if (offset.y > offset.x && offset.y > offset.z) {
                        currCell.y += glm::sign(offset.y) ;
                    } else if (offset.z > offset.y && offset.z > offset.x) {
                        currCell.x += glm::sign(offset.z) ;
                    }
                    m_terrain.setBlockAt(currCell.x, currCell.y, currCell.z, GRASS) ;
                    m_terrain.getChunkAt(currCell.x, currCell.z)->createVBOdata() ;
                    break;
                }
            }
        }
    }
}
