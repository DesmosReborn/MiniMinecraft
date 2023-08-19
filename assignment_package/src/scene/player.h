#pragma once
#include "entity.h"
#include "camera.h"
#include "terrain.h"

struct BlockPhysics {
    bool isSolid;
    bool canClimb;
    float drag;

    static BlockPhysics Solid();
    static BlockPhysics Empty();
    static BlockPhysics Liquid(float drag, bool canClimb = true);
};

class Player : public Entity {
private:
    glm::vec3 m_velocity, m_acceleration;
    Camera m_camera;
    const Terrain &mcr_terrain;
    bool flight;
    bool canJump, canClimb;

    static const std::array<glm::vec3, 12> collisionVerts;

    // block physics mapping, where solid is the default if key not in map
    static const std::unordered_map<BlockType, BlockPhysics> blockPhysics;

    static const BlockPhysics getBlockPhysics(BlockType block);

    void processInputs(InputBundle &inputs);
    void computePhysics(float dT, const Terrain &terrain);
    float acceleration;

public:
    // Readonly public reference to our camera
    // for easy access from MyGL
    const Camera& mcr_camera;

    Player(glm::vec3 pos, const Terrain &terrain);
    virtual ~Player() override;

    void setCameraWidthHeight(unsigned int w, unsigned int h);

    void tick(float dT, InputBundle &input) override;

    bool getFlight() ;
    void setFlight(bool newFlight);
    glm::vec3 getForward();
    glm::vec3 getRight();
    glm::vec3 getUp();

    void setVelocity(glm::vec3);

    bool gridMarch(glm::vec3 pos, glm::vec3 dir, const Terrain &terrain) ;

    // For sending the Player's data to the GUI
    // for display
    QString posAsQString() const;
    QString velAsQString() const;
    QString accAsQString() const;
    QString lookAsQString() const;
};
