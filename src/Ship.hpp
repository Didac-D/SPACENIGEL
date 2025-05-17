#pragma once
#include "Model.hpp"
#include <unordered_map>

enum class ProjectileType : int;

enum class ShipType {
    XR9,
    HellFire,
    HYDRA,
    SPEAR
};

inline const std::vector<ShipType> SHIP_ORDER = {
    ShipType::XR9,
    ShipType::HellFire,
    ShipType::HYDRA,
    ShipType::SPEAR
};

struct ShipStats {
    std::string name;
    Model* model;

    float maxHealth;
    float fireRate;
    ProjectileType projectileType;
    float projectileDamage;

    float maxSpeed;
    float acceleration;
    float pitchSpeed;
    float yawSpeed;
    float rollSpeed;

    float collisionRadius;
};

extern std::unordered_map<ShipType, ShipStats> SHIP_STATS;