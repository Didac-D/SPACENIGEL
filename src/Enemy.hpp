#pragma once
#include "Projectile.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Game;

class Enemy {
public:
    Enemy(const glm::vec3& startPosition);
    
    Enemy(Enemy&& other) = default;
    Enemy& operator=(Enemy&& other) = default;

    float maxHealth = 100.0f;
    mutable float health = maxHealth;
    bool isAlive = true;

    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat rotation;

    float collisionRadius = 0.6f;
    bool collisionDetected = false;
    float lastImpactSpeed = 0.0f;
    float m_lastCollisionTime = -1.0f;
    static constexpr float COLLISION_DAMAGE_COOLDOWN = 0.5f;

    float BOUNCE_FACTOR = 0.8f;
    float FRICTION_FACTOR = 0.8f;
    float DAMAGE_MULTIPLIER = 0.1f;
    float BASE_DAMAGE = 10.0f;
    float MAP_BOUNDARY = 150.0f;

    void Update(float deltaTime, const glm::vec3& playerPosition, Game& game);
    void UpdateRotation(const glm::vec3& direction);
    void HandleCollisions(Game& game);
    void HandleEntityCollisions(Game& game);
    bool TakeDamage(float damage, Game& game);
    bool IsAlive() const { return isAlive; }
    void markForDestruction() { !isAlive; }
    void AiMovement(float deltaTime, const glm::vec3& playerPosition);

private:
    float speed = 3.0f;
};