#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

class Game;
class Player;

struct ProjectileParams {
    std::string name;

    float speed;
    float baseDamage;
    float lifetime;
    float collisionRadius;

    // Laser-specific properties
    float damagePerTick;
    float tickInterval;
    float timeSinceLastTick;

    // Explosive-specific properties
    float explosionRadius;
};

enum class ProjectileType : int {
    BULLET,
    LASER,
    EXPLOSIVE,
    NONE
};

inline const std::unordered_map<ProjectileType, ProjectileParams> PROJECTILE_STATS = {
    {ProjectileType::BULLET, {
        .name = "Bullets",
        .speed = 75.0f,
        .baseDamage = 7.5f,
        .lifetime = 2.0f,
        .collisionRadius = 0.5f
    }},

    {ProjectileType::LASER, {
        .name = "Laser",
        .speed = 750.0f,
        .baseDamage = 2.5f,
        .lifetime = 0.15f,
        .collisionRadius = 0.8f,
        .tickInterval = 0.1f
    }},

    {ProjectileType::EXPLOSIVE, {
        .name = "Explosive",
        .speed = 60.0f,
        .baseDamage = 50.0f,
        .lifetime = 2.5f,
        .collisionRadius = 0.6f,
        .explosionRadius = 8.0f
    }},

    {ProjectileType::NONE, {
        .name = "NONE",
        .speed = 0.0f,
        .baseDamage = 0.0f,
        .lifetime = 0.0f,
        .collisionRadius = 0.0f
    }}
};

struct Projectile {
public:
    Projectile(glm::vec3 pos, glm::vec3 dir, ProjectileType t, float damage);

    std::vector<Player*> hitPlayers;
    Player* sourcePlayer = nullptr;
    
    glm::vec3 position;
    glm::vec3 direction;
    ProjectileType type;

    float speed;
    float baseDamage;
    float lifetime;
    float collisionRadius;

    // Laser-specific properties
    float damagePerTick;
    float tickInterval;
    float timeSinceLastTick;

    // Explosive-specific properties
    float explosionRadius;

    float damage;
    bool m_shouldDestroy;

    void Update(float deltaTime, std::vector<Player>& players, Game& game);
    void PlayerCollisionDetection(std::vector<Player>& players, Game& game);
    void TerrainCollisionDetection(const Game& game);
    bool ShouldDestroy() const { return m_shouldDestroy; }
    void MarkForDestruction() { m_shouldDestroy = true; }
};