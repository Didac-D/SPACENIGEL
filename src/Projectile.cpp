#include "Projectile.hpp"
#include "Enemy.hpp"
#include "Game.hpp"
#include <iostream>

Projectile::Projectile(glm::vec3 pos, glm::vec3 dir, ProjectileType t)
    : position(pos), direction(glm::normalize(dir)), type(t), m_shouldDestroy(false) {
    if (type == ProjectileType::BULLET) {
        speed = 75.0f;
        lifetime = 2.0f;
        collisionRadius = 0.2f;
        damage = 10.0f;
    } else if (type == ProjectileType::LASER) {
        speed = 1000.0f;
        lifetime = 0.1f;
        collisionRadius = 0.5f;
        damage = 35.0f;
    }
}

void Projectile::Update(float deltaTime, std::vector<Player>& players, Game& game) {
    if (m_shouldDestroy) return;

    // Update position for both projectile types
    position += direction * speed * deltaTime;
    lifetime -= deltaTime;

    // Check collisions for all projectiles
    PlayerCollisionDetection(players, game);
    TerrainCollisionDetection(game);

    if (lifetime <= 0.0f) {
        MarkForDestruction();
    }
}

void Projectile::PlayerCollisionDetection(std::vector<Player>& players, Game& game) {
    for(auto& player : players) {
        if(&player == sourcePlayer || !player.isAlive) continue;
        
        glm::vec3 delta = player.position - position;
        float distanceSq = glm::dot(delta, delta);
        float radiusSum = player.collisionRadius + collisionRadius;
        
        if(distanceSq < (radiusSum * radiusSum)) {
            bool killed = player.TakeDamage(damage, game);
            if(killed) {
                game.ReportPlayerKilled();
                player.lastKillTime = game.m_totalTime;
            } else {
                game.ReportPlayerHit();
                player.lastHitTime = game.m_totalTime;
            }
            // Destroy bullet on hit, laser continues
            if (type == ProjectileType::BULLET) {
                MarkForDestruction();
            }
            break;
        }
    }
}

void Projectile::TerrainCollisionDetection(const Game& game) {
    const float terrainHeight = game.GetTerrainHeight(position.x, position.z);
    if(position.y - collisionRadius <= terrainHeight) {
        MarkForDestruction();
    }
}