#pragma once
#include "Projectile.hpp"
#include <vector>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Game;
class Camera;

class Player {
private:
    glm::vec2 m_reticleOffset{0.0f};

    // Player's projectiles (to be transferred to the game)
    std::vector<Projectile> m_projectiles;

public:
    Player();

    void Reset() {
        position = glm::vec3(10.0f);
        velocity = glm::vec3(0.0f);
        rotation = glm::identity<glm::quat>();
        health = maxHealth;
        isAlive = true;
    }

    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat rotation;

    // Stats
    float maxHealth = 300.0f;
    float health = maxHealth;
    bool isAlive = true;

    // Movement properties
    float maxSpeed = 75.0f;         // Maximum speed limit
    float acceleration = 25.0f;     // Forward/backward thrust
    float dragCoefficient = 0.9975f;  // Air resistance
    float lastImpactSpeed = 0.0f;

    // Collision parameters
    bool collisionDetected = false;
    float collisionRadius = 0.6f;
    float BOUNCE_FACTOR = 0.4f;
    float FRICTION_FACTOR = 0.7f;
    float DAMAGE_MULTIPLIER = 0.1f;
    float BASE_DAMAGE = 15.0f;
    float MAP_BOUNDARY = 150.0f;
    float m_lastCollisionTime = -1.0f;
    static constexpr float COLLISION_DAMAGE_COOLDOWN = 0.5f;

    // Rotation properties
    float pitchSpeed = 45.0f;     // Mouse Y sensitivity
    float yawSpeed = 45.0f;       // Mouse X sensitivity
    float rollSpeed = 90.0f;      // A/D key roll speed
    float maxRollAngle = 90.0f;   // Maximum banking angle
    float minPitch = -89.0f;
    float maxPitch = 89.0f;

    // Ability properties
    // Basic shooting
    float m_fireCooldown = 0.1f;
    float m_timeSinceLastShot = 0.0f;
    const float FIRE_RATE = 0.1f; // Shots per second
    const float SPAWN_OFFSET = 0.5f; // Distance from ship center

    // Abilities
    void Shoot();

    void Update(GLFWwindow* window, float deltaTime, Game& game);
    void UpdatePhysics(float deltaTime);    
    void UpdateRotation(GLFWwindow* window, float deltaTime, const glm::vec2& mouseDelta);
    void HandleCollisions(Game& game, const float deltaTime);
    void HandleEntityCollisions(Game& game);
    bool TakeDamage(float damage, Game& game);
    bool IsAlive() const { return isAlive; }

    void ProcessKeyboardInput(GLFWwindow* window, float deltaTime);
    void ProcessMouseInput(GLFWwindow* window, float deltaTime);

    // Getters
    glm::vec3 GetForward() const { return rotation * glm::vec3(0.0f, 0.0f, 1.0f); }
    glm::vec3 GetRight() const   { return rotation * glm::vec3(1.0f, 0.0f, 0.0f); }
    glm::vec3 GetUp() const      { return rotation * glm::vec3(0.0f, 1.0f, 0.0f); }

    void TransferProjectiles(std::vector<Projectile>& gameProjectiles) {
        gameProjectiles.insert(gameProjectiles.end(), 
            std::make_move_iterator(m_projectiles.begin()), 
            std::make_move_iterator(m_projectiles.end())
        );
        m_projectiles.clear();
    }

    // For camera roll extraction
    float GetRoll() const;
    void SetReticleOffset(const glm::vec2& offset) {m_reticleOffset = offset; }
};