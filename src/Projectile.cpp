#include "Projectile.hpp"
#include "Enemy.hpp"
#include "Game.hpp"
#include <iostream>

void Projectile::Update(float deltaTime, std::vector<Enemy>& enemies, Game& game) {
    if(m_shouldDestroy) return;

    position += direction * speed * deltaTime;
    lifetime -= deltaTime;

    EnemyCollisionDetection(enemies, game);
    TerrainCollisionDetection(game);

    if(lifetime <= 0.0f) {
        MarkForDestruction();
    }
}

void Projectile::EnemyCollisionDetection(std::vector<Enemy>& enemies, Game& game) {
    for(auto& enemy : enemies) {
        glm::vec3 delta = enemy.position - position;
        float distanceSq = glm::dot(delta, delta);
        float radiusSum = enemy.collisionRadius + collisionRadius;
        float threshold = radiusSum * radiusSum;

        if(distanceSq < threshold) {
            bool killed = enemy.TakeDamage(5.0f, game);
            
            if(killed) {
                game.ReportEnemyKilled();
            } else {
                game.ReportEnemyHit();
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