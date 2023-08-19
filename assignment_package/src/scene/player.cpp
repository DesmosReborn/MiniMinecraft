#include "player.h"
#include <QString>
#include <cmath>
#include <ostream>

BlockPhysics BlockPhysics::Solid()
{ return {true, false, 0.f}; }

BlockPhysics BlockPhysics::Empty()
{ return {false, false, 0.f}; }

BlockPhysics BlockPhysics::Liquid(float drag, bool canClimb)
{ return {false, canClimb, drag}; }

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      flight(false), canJump(false), canClimb(false), acceleration(45.0f),
      mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

const std::array<glm::vec3, 12> Player::collisionVerts{
    glm::vec3{+ 0.25f, 0.f, + 0.25f},
    glm::vec3{- 0.25f, 0.f, + 0.25f},
    glm::vec3{+ 0.25f, 0.f, - 0.25f},
    glm::vec3{- 0.25f, 0.f, - 0.25f},
    glm::vec3{+ 0.25f, 1.f, + 0.25f},
    glm::vec3{- 0.25f, 1.f, + 0.25f},
    glm::vec3{+ 0.25f, 1.f, - 0.25f},
    glm::vec3{- 0.25f, 1.f, - 0.25f},
    glm::vec3{+ 0.25f, 2.f, + 0.25f},
    glm::vec3{- 0.25f, 2.f, + 0.25f},
    glm::vec3{+ 0.25f, 2.f, - 0.25f},
    glm::vec3{- 0.25f, 2.f, - 0.25f},
};

const std::unordered_map<BlockType, BlockPhysics> Player::blockPhysics{
    {EMPTY, BlockPhysics::Empty()},
    {WATER, BlockPhysics::Liquid(0.15f)},
    {LAVA, BlockPhysics::Liquid(0.15f)},
};

const BlockPhysics Player::getBlockPhysics(BlockType block) {
    auto physics = blockPhysics.find(block);
    if (physics == blockPhysics.end()) {
        return BlockPhysics::Solid();
    } else {
        return physics->second;
    }
}

void Player::processInputs(InputBundle &inputs) {
    glm::mat4 cameraRot = glm::mat4(glm::orientate3(glm::vec3(glm::radians(inputs.mouseY), 0, glm::radians(inputs.mouseX))));
    glm::mat3 groundRot = glm::mat3(glm::eulerAngleY(glm::radians(inputs.mouseX)));
    m_camera.setRotation(cameraRot);

    glm::vec3 playerDI = {(inputs.aPressed ? 1.f : 0.f) + (inputs.dPressed ? -1.f : 0.f),
                          (inputs.ePressed ? 1.f : 0.f) + (inputs.qPressed ? -1.f : 0.f),
                          (inputs.wPressed ? 1.f : 0.f) + (inputs.sPressed ? -1.f : 0.f)};

    playerDI = {playerDI.x * glm::sqrt(1.f - playerDI.z * playerDI.z * 0.5f),
                            playerDI.y,
                            playerDI.z * glm::sqrt(1.f - playerDI.x * playerDI.x * 0.5f)};

    // reset acceleration
    if (!flight) {
        m_acceleration = glm::vec3(0, -20, 0);
    } else {
        m_acceleration = glm::vec3(0, 0, 0);
    }

    if (flight) {
        // only player DI
        m_acceleration += acceleration * playerDI.x * glm::vec3(cameraRot[0]) * 2.f;
        m_acceleration += acceleration * playerDI.y * glm::vec3(groundRot[1]) * 2.f;
        m_acceleration += acceleration * playerDI.z * glm::vec3(cameraRot[2]) * 2.f;
    } else {

        // add player DI
        playerDI.y = 0;
        m_acceleration += acceleration * (groundRot * playerDI);

        // climb liquids and such
        if (inputs.spacePressed) {
            if (canClimb) {
                m_acceleration.y = acceleration * 0.8f;
            } else if (canJump && m_velocity.y <= 0) {
                m_velocity.y = 8;
            }
        }
    }
}

void Player::computePhysics(float dT, const Terrain &terrain) {
    m_velocity += (m_acceleration * dT);
    glm::vec3 moveRay = glm::vec3(m_velocity.x * dT, m_velocity.y * dT, m_velocity.z * dT) ;

    if (glm::dot(moveRay, moveRay) > 0.002f) {

        float maxDrag = 0.f;
        canClimb = false;

        if (!flight) {
            glm::vec3 rayx = glm::vec3(moveRay.x, 0, 0);
            glm::vec3 rayy = glm::vec3(0, moveRay.y, 0);
            glm::vec3 rayz = glm::vec3(0, 0, moveRay.z);

            for (glm::vec3 offset : collisionVerts) {
                glm::vec3 pos = offset + m_position;

                // get block and associated physics at vert
                BlockType block = terrain.getBlockAt(pos);
                BlockPhysics phys = getBlockPhysics(block);
                maxDrag = glm::max(maxDrag, phys.drag);

                canClimb = canClimb || phys.canClimb;

                if (abs(moveRay.x) > 0.001f && gridMarch(pos, rayx, terrain)) {
                    moveRay.x = 0;
                }
                if (abs(moveRay.z) > 0.001f && gridMarch(pos, rayz, terrain)) {
                    moveRay.z = 0;
                }
            }

            float dragCoefficient = 1.f - maxDrag;
            // add side movement drag
            glm::vec2 draggedVel = glm::vec2(m_velocity.x, m_velocity.z) * dragCoefficient * 0.85f;
            float draggedYVel = m_velocity.y * dragCoefficient;

            m_velocity.x = draggedVel.x;
            m_velocity.z = draggedVel.y;
            m_velocity.y = draggedYVel;

            if (abs(moveRay.y) > 0.001f && (gridMarch(m_position, rayy * 0.001f, terrain))) {
                moveRay.y = 0;
                m_velocity.y = 0;
            }
            canJump = (gridMarch(m_position, glm::vec3(0, -0.3f, 0), terrain)) ;
        } else {
            m_velocity *= 0.9f;
        }
        moveAlongVector(moveRay) ;

    }

    auto pos = m_position + glm::vec3(0.f, 1.5f, 0.f);
    m_camera.setPosition(pos);

}

void Player::setVelocity(glm::vec3 newVelocity) {
    m_velocity = newVelocity;
    m_acceleration = glm::vec3(0);
}

bool Player::gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain) {
    float maxLen = sqrt(pow(rayDirection.x, 2) + pow(rayDirection.y, 2) + pow(rayDirection.z, 2)) ; // Farthest we search
    glm::ivec3 currCell = glm::ivec3(rayOrigin);
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
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
            // If currCell contains a solid block, return
            BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
            if(getBlockPhysics(cellType).isSolid) {
                return true;
            }
        }
    }
    return false;
}



void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}

bool Player::getFlight() {
    return flight;
}

void Player::setFlight(bool newFlight) {
    flight = newFlight;
}

glm::vec3 Player::getForward() {
    return m_forward;
}

glm::vec3 Player::getRight() {
    return m_right;
}

glm::vec3 Player::getUp() {
    return m_up;
}
