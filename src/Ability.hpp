#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <glm/vec3.hpp>

class Game;
class Player;
class Projectile;

enum AbilityType {
    BOMB,
    TURBO
};

inline const std::vector<AbilityType> ABILITY_ORDER = {
    AbilityType::BOMB,
    AbilityType::TURBO
};

struct AbilityInfo {
    std::string name;
    float cooldown;
};

class Ability {
public:
    std::vector<Projectile> m_projectiles;

    void Activate(AbilityType abilityType, Player &sourcePlayer);
    void Update(float deltaTime);

    void TransferProjectiles(std::vector<Projectile>& gameProjectiles) {
        gameProjectiles.insert(gameProjectiles.end(), 
            std::make_move_iterator(m_projectiles.begin()), 
            std::make_move_iterator(m_projectiles.end())
        );
        m_projectiles.clear();
    }
};

inline const std::unordered_map<AbilityType, AbilityInfo> ABILITY_PARAMS = {
    { AbilityType::BOMB, {
        .name = "Bomb",
        .cooldown = 10.0f
    }}, 

    {AbilityType::TURBO, {
        .name = "Turbo",
        .cooldown = 5.0f
    }}
};