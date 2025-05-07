#pragma once
#include "Model.hpp"
#include <unordered_map>

enum class ShipType {
    XR9,
    HellFire
};

enum class ProjectileType {
    BULLET,
    LASER
};

struct ShipStats {
    float maxHealth;
    float fireRate;
    ProjectileType projectileType;
    Model* model;

    float maxSpeed;
    float acceleration;
    float pitchSpeed;
    float yawSpeed;
    float rollSpeed;
};

inline std::unordered_map<ShipType, ShipStats> SHIP_STATS = {
    {ShipType::XR9, {
        .maxHealth = 300.0f,
        .fireRate = 0.1f,
        .projectileType = ProjectileType::BULLET,

        .model = nullptr, // Set during model load

        .maxSpeed = 75.0f,
        .acceleration = 25.0f,
        .pitchSpeed = 45.0f,
        .yawSpeed = 45.0f,
        .rollSpeed = 90.0f
    }},
    {ShipType::HellFire, {
        .maxHealth = 200.0f,
        .fireRate = 0.1f,
        .projectileType = ProjectileType::LASER,

        .model = nullptr,

        .maxSpeed = 90.0f,
        .acceleration = 30.0f,
        .pitchSpeed = 50.0f,
        .yawSpeed = 50.0f,
        .rollSpeed = 95.0f
    }}
};