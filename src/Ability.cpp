#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <glad/glad.h>
#include "Ability.hpp"
#include "Player.hpp"
#include "Projectile.hpp"

void Ability::Activate(AbilityType abilityType, Player &sourcePlayer) {
    float SIDE_OFFSET = 0.0f;
    float DEPTH_OFFSET = 5.0f;
    float HEIGHT_OFFSET = -0.1f;

    switch (abilityType) {
        case BOMB:
            m_projectiles.emplace_back(
                sourcePlayer.position + SIDE_OFFSET * sourcePlayer.GetRight() + DEPTH_OFFSET * sourcePlayer.GetForward() + HEIGHT_OFFSET * sourcePlayer.GetUp(),
                sourcePlayer.GetForward(),
                ProjectileType::EXPLOSIVE,
                PROJECTILE_STATS.at(ProjectileType::EXPLOSIVE).baseDamage
            );       
            m_projectiles.back().sourcePlayer = &sourcePlayer;
            break;

        case TURBO:
            sourcePlayer.velocity += sourcePlayer.maxSpeed * 0.5f * sourcePlayer.GetForward();
            break;
        default:
        
            std::cout << "Something went wrong. Using default (nothing lmao)" << std::endl;
            break;
    }
}