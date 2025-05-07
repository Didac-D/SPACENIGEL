#pragma once
#include "Game.hpp"
#include "Ship.hpp"
#include <vector>
#include <glm/glm.hpp>

class Game;
class Enemy;

struct Projectile {
public:
    Projectile(glm::vec3 pos, glm::vec3 dir, ProjectileType t);

    Player* sourcePlayer = nullptr;
    
    glm::vec3 position;
    glm::vec3 direction;
    ProjectileType type;
    float speed;
    float damage;
    float lifetime;
    bool m_shouldDestroy;
    float collisionRadius;

    // Laser-specific properties
    float damagePerTick;
    float tickInterval;
    float timeSinceLastTick;

    void Update(float deltaTime, std::vector<Player>& players, Game& game);
    void PlayerCollisionDetection(std::vector<Player>& players, Game& game);
    void TerrainCollisionDetection(const Game& game);
    bool ShouldDestroy() const { return m_shouldDestroy; }
    void MarkForDestruction() { m_shouldDestroy = true; }
};