#pragma once
#include "Ability.hpp"
#include "Ship.hpp"
#include "Projectile.hpp"
#include <vector>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Game;
class Camera;

class Player {
private:
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_firstMouse = true;
    glm::vec2 m_mouseDelta{0.0f, 0.0f};
    glm::vec2 m_reticleOffset{0.0f, 0.0f};

    // Player's projectiles (to be transferred to the game)
    std::vector<Projectile> m_projectiles;

public:
    Player();

    bool isMainPlayer = false;
    bool isAI = false;
    int team = 0;

    float lastHitTime = -1.0f;
    float lastKillTime = -1.0f;

    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat rotation;

    ShipType m_shipType = ShipType::XR9;

    // Stats
    float maxHealth = 300;
    float health = maxHealth;
    bool isAlive = true;

    // Basic shooting
    ProjectileType projectileType = ProjectileType::BULLET;
    float baseDamage = 7.5f;
    float damage = baseDamage;
    float m_fireCooldown = 0.1f;
    float m_timeSinceLastShot = 0.0f;
    float fireRate = 0.2f; 
    float SPAWN_OFFSET = 0.5f; // Distance from ship center

    // Movement properties
    float maxSpeed = 75.0f;         // Maximum speed limit
    float acceleration = 25.0f;     // Forward/backward thrust
    float dragCoefficient = 0.9975f;  // Air resistance
    float lastImpactSpeed = 0.0f;

    // Ability properties
    Ability m_ability;
    AbilityType m_ability1 = AbilityType::BOMB, m_ability2 = AbilityType::TURBO;
    float abilityCooldown1 = 10.0f, abilityCooldown2 = 5.0f;
    float m_timeSinceLastAbility1 = abilityCooldown1, m_timeSinceLastAbility2 = abilityCooldown1;

    // Collision parameters
    bool collisionDetected = false;
    float collisionRadius = 0.6f;
    float BOUNCE_FACTOR = 0.4f;
    float FRICTION_FACTOR = 0.7f;
    float COLLISION_DAMAGE_MULTIPLIER = 0.8f;
    float COLLISION_BASE_DAMAGE = 15.0f;
    float MAP_BOUNDARY = 150.0f;
    float m_lastCollisionTime = -1.0f;
    static constexpr float COLLISION_DAMAGE_COOLDOWN = 0.5f;

    // Rotation properties
    glm::quat roll;
    float pitchSpeed = 45.0f;     // Mouse Y sensitivity
    float yawSpeed = 45.0f;       // Mouse X sensitivity
    float rollSpeed = 90.0f;      // A/D key roll speed
    float maxRollAngle = 90.0f;   // Maximum banking angle
    float minPitch = -89.0f;
    float maxPitch = 89.0f;

    // AI parameters
    float AI_AGGRESSION_RANGE = 30.0f;
    float AI_FIRE_RATE_MULTIPLIER = 1.5f;

    // Stats
    void InitializeStats();

    // Abilities
    void Shoot(float deltaTime, Game& game);
    void AiShooting(float deltaTime, const glm::vec3& targetDir, Game& game);

    // Update functions
    void Update(GLFWwindow* window, float deltaTime, Game& game);
    void UpdateAI(float deltaTime, const glm::vec3& targetPos, Game& game);

    void UpdatePhysics(float deltaTime);    
    void UpdateRotation(GLFWwindow* window, float deltaTime, const glm::vec2& mouseDelta);

    void UpdateAIRotation(const glm::vec3& direction);
    void HandleCollisions(Game& game, const float deltaTime);
    void HandleAICollisions(Game& game);
    void HandleEntityCollisions(Game& game);
    
    bool TakeDamage(float damage, Game& game);
    bool IsAlive() const { return isAlive; }

    void ProcessKeyboardInput(GLFWwindow* window, float deltaTime);
    void ProcessMouseInput(GLFWwindow* window, float deltaTime, Game& game);

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

    float GetRoll() const;
    void SetReticleOffset(const glm::vec2& offset) {m_reticleOffset = offset; }
};