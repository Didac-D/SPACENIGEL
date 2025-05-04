#pragma once
#include <vector>
#include <glm/glm.hpp>

class Game;
class Enemy;


class Projectile {
private:

public:
    Projectile(glm::vec3 startPos, glm::vec3 dir) 
    : position(startPos), direction(glm::normalize(dir)) {}

    glm::vec3 position;
    glm::vec3 direction;
    float speed = 30.0f;
    float lifetime = 3.0f;
    float collisionRadius = 0.1f;
    bool m_shouldDestroy = false;

    void Update(float deltaTime, std::vector<Enemy>& enemies, Game& game);
    void EnemyCollisionDetection(std::vector<Enemy>& enemies, Game& game);
    void TerrainCollisionDetection(const Game& game);
    bool ShouldDestroy() const { return m_shouldDestroy; }
    void MarkForDestruction() { m_shouldDestroy = true; }
};