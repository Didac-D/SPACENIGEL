#include "Ship.hpp"
#include "Projectile.hpp"

inline std::unordered_map<ShipType, ShipStats> SHIP_STATS = {
    {ShipType::XR9, {
        .name = "XR9",
        .model = nullptr, // Set during model load

        .maxHealth = 300.0f,
        .fireRate = 0.1f,
        .projectileType = ProjectileType::BULLET,
        .projectileDamage = PROJECTILE_STATS.at(ProjectileType::BULLET).baseDamage,

        .maxSpeed = 75.0f,
        .acceleration = 25.0f,
        .pitchSpeed = 45.0f,
        .yawSpeed = 50.0f,
        .rollSpeed = 110.0f,

        .collisionRadius = 0.6
    }},

    {ShipType::HellFire, {
        .name = "HellFire",
        .model = nullptr,

        .maxHealth = 200.0f,
        .fireRate = 0.0025f,
        .projectileType = ProjectileType::LASER,
        .projectileDamage = PROJECTILE_STATS.at(ProjectileType::LASER).baseDamage,

        .maxSpeed = 90.0f,
        .acceleration = 30.0f,
        .pitchSpeed = 45.0f,
        .yawSpeed = 50.0f,
        .rollSpeed = 125.0f,

        .collisionRadius = 0.45
    }},

    {ShipType::HYDRA, {
        .name = "HYDRA",
        .model = nullptr,

        .maxHealth = 500.0f,
        .fireRate = 0.75f,
        .projectileType = ProjectileType::EXPLOSIVE,
        .projectileDamage = PROJECTILE_STATS.at(ProjectileType::EXPLOSIVE).baseDamage,

        .maxSpeed = 55.0f,
        .acceleration = 20.0f,
        .pitchSpeed = 45.0f,
        .yawSpeed = 50.0f,
        .rollSpeed = 95.0f,

        .collisionRadius = 0.9
    }},

    {ShipType::SPEAR, {
        .name = "SPEAR",
        .model = nullptr,

        .maxHealth = 3000.0f,
        .fireRate = 0.0f,
        .projectileType = ProjectileType::NONE,
        .projectileDamage = PROJECTILE_STATS.at(ProjectileType::EXPLOSIVE).baseDamage,

        .maxSpeed = 150.0f,
        .acceleration = 50.0f,
        .pitchSpeed = 45.0f,
        .yawSpeed = 50.0f,
        .rollSpeed = 150.0f,

        .collisionRadius = 1.2
    }}
};