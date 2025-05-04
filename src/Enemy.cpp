#define GLM_ENABLE_EXPERIMENTAL
#include "Enemy.hpp"
#include "Game.hpp"
#include <iostream>
#include <glm/gtx/quaternion.hpp>

Enemy::Enemy(const glm::vec3& startPosition) 
    : position(startPosition), 
      rotation(glm::identity<glm::quat>()), 
      health(100.0f),
      isAlive(true) {}

void Enemy::Update(float deltaTime, const glm::vec3& playerPos, Game& game) {
    if(!isAlive) return;
    collisionDetected = false;
    AiMovement(deltaTime, playerPos);
    position += velocity * deltaTime;
    
    HandleCollisions(game);
    HandleEntityCollisions(game);

    glm::vec3 direction = playerPos - position;
    if(glm::length(direction) > 0.01f) {
        UpdateRotation(glm::normalize(direction));
    }
    
    // Continuous smoke based on damage
    float damageRatio = 1.0f - (health / maxHealth);
    game.GetParticles().CreateEmitter(
        position,
        ParticleType::SMOKE, 
        8.0f, 
        glm::vec3(0.5f, 0.5f, 0.5f), // Start color (gray)
        glm::vec3(0.2f, 0.2f, 0.2f), // End color (dark gray)
        static_cast<int>(damageRatio * 10) // Particle count based on damage ratio
    );

    velocity *= FRICTION_FACTOR;
}

void Enemy::UpdateRotation(const glm::vec3& direction) {
    glm::vec3 forward = glm::normalize(direction);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(up, forward));
    
    // Create rotation matrix from forward vector
    rotation = glm::quatLookAt(forward, up);
}

void Enemy::HandleCollisions(Game& game) {
    // Terrain collision with normal-based response
    const float terrainHeight = game.GetTerrainHeight(position.x, position.z);
    const float verticalDist = position.y - terrainHeight;
    
    if(verticalDist < collisionRadius) {
        glm::vec3 normal = game.GetTerrainNormal(position.x, position.z);
        
        // Reflect velocity using terrain normal
        float normalVelocity = glm::dot(velocity, normal);
        
        // Apply surface-parallel friction
        glm::vec3 tangentVel = velocity - glm::dot(velocity, normal) * normal;
        velocity -= tangentVel * FRICTION_FACTOR;
        
        // Position correction
        position.y = terrainHeight + collisionRadius;
        collisionDetected = true;
    }

    // Wall collision (axis-aligned boundaries)
    const float boundary = MAP_BOUNDARY - collisionRadius;
    if(position.x > boundary || position.x < -boundary) {
        velocity.x = -velocity.x * BOUNCE_FACTOR;
        position.x = glm::clamp(position.x, -boundary, boundary);
        collisionDetected = true;
    }
    if(position.z > boundary || position.z < -boundary) {
        velocity.z = -velocity.z * BOUNCE_FACTOR;
        position.z = glm::clamp(position.z, -boundary, boundary);
        collisionDetected = true;
    }

    if(collisionDetected) {
        TakeDamage(BASE_DAMAGE + fabs(glm::length(velocity)) * DAMAGE_MULTIPLIER, game);
    }
}

void Enemy::HandleEntityCollisions(Game& game) {
    Player& player = game.GetPlayer();
    
    // Player collision
    const float playerDistance = glm::distance(position, player.position);
    const float playerMinDist = collisionRadius + player.collisionRadius;
    
    if(playerDistance < playerMinDist) {
        // Physics response
        glm::vec3 dir = glm::normalize(position - player.position);
        velocity += dir * BOUNCE_FACTOR;
        player.velocity -= dir * player.BOUNCE_FACTOR;

        // Damage handling with cooldown
        if((game.m_totalTime - m_lastCollisionTime) > COLLISION_DAMAGE_COOLDOWN) {
            const float damage = BASE_DAMAGE + fabs(velocity.y) * DAMAGE_MULTIPLIER;
            bool killed = TakeDamage(damage, game);
            m_lastCollisionTime = game.m_totalTime;
            
            if(killed) {
                game.ReportEnemyKilled();
            }
        }
    }

    // Other enemy collisions
    for(auto& other : game.GetEnemies()) {
        if(&other != this) {
            const float distance = glm::distance(position, other.position);
            const float minDistance = collisionRadius + other.collisionRadius;
            
            if(distance < minDistance) {
                const glm::vec3 dir = glm::normalize(position - other.position);
                velocity += dir * BOUNCE_FACTOR;
                other.velocity -= dir * other.BOUNCE_FACTOR;
            }
        }
    }
}

bool Enemy::TakeDamage(float amount, Game& game) {
    health -= amount;

    if(health <= 0) {
        game.GetParticles().CreateEmitter(
            position, 
            ParticleType::EXPLOSION_BIG,
            0.4f,
            glm::vec3(1.0f, 0.8f, 0.0f), // Start color (yellow)
            glm::vec3(1.0f, 0.0f, 0.0f), // End color (red)
            100
        );
        game.ReportEnemyKilled();
        isAlive = false;
        return true;
    } else {
        game.GetParticles().CreateEmitter(
            position,
            ParticleType::EXPLOSION_SMALL,
            0.2f,
            glm::vec3(1.0f, 0.9f, 0.0f), // Start color (bright yellow)
            glm::vec3(1.0f, 0.5f, 0.0f), // End color (orange)
            30
        );
        game.ReportEnemyHit();
        return false;
    }
}

void Enemy::AiMovement(float deltaTime, const glm::vec3& playerPosition) {
    glm::vec3 direction = playerPosition - position;
    float distance = glm::length(direction);

    if(distance > 0.1f) {
        // Move towards player
        direction = glm::normalize(direction);
        position += direction * speed * deltaTime;
        
        // Update rotation to face direction
        UpdateRotation(direction);
    }
}
