#include "Projectile.hpp"
#include "Game.hpp"
#include <iostream>

Projectile::Projectile(glm::vec3 pos, glm::vec3 dir, ProjectileType t, float damage)
    : position(pos), direction(glm::normalize(dir)), type(t), damage(damage), m_shouldDestroy(false) {
    const auto& stats = PROJECTILE_STATS.at(type); // Retrieve stats for the given type

    speed = stats.speed;
    lifetime = stats.lifetime;
    collisionRadius = stats.collisionRadius;

    // Specific properties
    if (type == ProjectileType::LASER) {
        tickInterval = stats.tickInterval;
    } else if (type == ProjectileType::EXPLOSIVE) {
        explosionRadius = stats.explosionRadius;
    }
}

void Projectile::Update(float deltaTime, std::vector<Player>& players, Game& game) {
    if (m_shouldDestroy) return;
    
    if (type == ProjectileType::BULLET || type == ProjectileType::LASER || type == ProjectileType::EXPLOSIVE) {
        position += direction * speed * deltaTime;
        lifetime -= deltaTime;
        PlayerCollisionDetection(players, game);
        TerrainCollisionDetection(game);
    }

    if (lifetime <= 0.0f) MarkForDestruction();

    if (m_shouldDestroy) {
        if (type == ProjectileType::EXPLOSIVE) {
            collisionRadius = explosionRadius;
            PlayerCollisionDetection(players, game);
            game.GetParticles().CreateEmitter(
                ParticleType::EXPLOSION_SMALL,
                position,
                0.1f,
                500,
                glm::vec3(1.0f, 0.9f, 0.0f), // Start color (bright yellow)
                glm::vec3(1.0f, 0.5f, 0.0f) // End color (orange)
            );
        }
    }
}

void Projectile::PlayerCollisionDetection(std::vector<Player>& players, Game& game) {
    for (auto& player : players) {
        if (&player == sourcePlayer || player.team == sourcePlayer->team || !player.isAlive) continue;

        glm::vec3 delta = player.position - position;
        float distanceSq = glm::dot(delta, delta);
        float radiusSum = player.collisionRadius + collisionRadius;

        if (distanceSq < (radiusSum * radiusSum)) {
            bool killed = player.TakeDamage(damage, game);

            // Only trigger hitmarker if the MAIN PLAYER is the source
            if (sourcePlayer && sourcePlayer->isMainPlayer) {
                if (killed) {
                    sourcePlayer->lastKillTime = game.m_totalTime;
                    game.ReportPlayerKilled();
                } else {
                    sourcePlayer->lastHitTime = game.m_totalTime;
                    game.ReportPlayerHit();
                }
            }                

            MarkForDestruction();
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