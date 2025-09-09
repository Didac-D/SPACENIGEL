#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include "Game.hpp"
#include "Player.hpp"
#include "Projectile.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/random.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/intersect.hpp>

Player::Player() 
    : position(glm::vec3(10.0f)), velocity(0.0f), rotation(glm::identity<glm::quat>()) {}

void Player::InitializeStats() {
    // STATS
    const auto& stats = SHIP_STATS.at(m_shipType);

    maxHealth = stats.maxHealth;
    health = maxHealth;

    projectileType = stats.projectileType;
    baseDamage = stats.projectileDamage;
    damage = baseDamage;
    fireRate = stats.fireRate;

    maxSpeed = stats.maxSpeed;
    acceleration = stats.acceleration;
    pitchSpeed = stats.pitchSpeed;
    yawSpeed = stats.yawSpeed;
    rollSpeed = stats.rollSpeed;

    collisionRadius = stats.collisionRadius;

    // ABILITIES
    abilityCooldown1 = ABILITY_PARAMS.at(m_ability1).cooldown;
    abilityCooldown2 = ABILITY_PARAMS.at(m_ability2).cooldown;
    m_timeSinceLastAbility1 = abilityCooldown1;
    m_timeSinceLastAbility2 = abilityCooldown1;
}

float Player::GetRoll() const {
    glm::vec3 euler = glm::eulerAngles(rotation);
    return glm::degrees(euler.z);
}

void Player::ProcessKeyboardInput(GLFWwindow* window, float deltaTime) {
    // Forward/backward movement
    glm::vec3 forward = GetForward();
    if(glfwGetKey(window, GLFW_KEY_W) && glm::length(velocity) < maxSpeed) 
        velocity += forward * acceleration * deltaTime;
    if(glfwGetKey(window, GLFW_KEY_S) && glm::length(velocity) < maxSpeed) 
        velocity -= forward * acceleration * deltaTime;

    // Roll from A/D keys
    float rollInput = 0.0f;
    if(glfwGetKey(window, GLFW_KEY_A)) rollInput -= 1.0f;
    if(glfwGetKey(window, GLFW_KEY_D)) rollInput += 1.0f;
    roll = glm::angleAxis(
        glm::radians(rollInput * rollSpeed * deltaTime),
        GetForward()
    );
    
    if (glfwGetKey(window, GLFW_KEY_SPACE) && m_timeSinceLastAbility2 >= abilityCooldown2) {
        m_ability.Activate(m_ability2, *this);
        m_timeSinceLastAbility2 = 0.0f;

        m_ability.TransferProjectiles(m_projectiles);
    }
}

void Player::ProcessMouseInput(GLFWwindow* window, float deltaTime, Game& game) {
    // Get current mouse position from GLFW
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Only process if cursor is disabled (in gameplay)
    if(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        if(m_firstMouse) {
            m_lastMouseX = xpos;
            m_lastMouseY = ypos;
            m_firstMouse = false;
        }

        // Calculate delta using current and last positions
        float xoffset = static_cast<float>(xpos - m_lastMouseX);
        float yoffset = static_cast<float>(m_lastMouseY - ypos);
        
        // Store delta for rotation
        m_mouseDelta = glm::vec2(xoffset, yoffset);

        // Update last position
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
    }
    else {
        m_firstMouse = true;
        m_mouseDelta = glm::vec2(0.0f);
    }

    // Shooting logic remains the same
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (m_timeSinceLastShot >= fireRate) {
            Shoot(deltaTime, game);
            m_timeSinceLastShot = 0.0f;
        }
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && m_timeSinceLastAbility1 >= abilityCooldown1) {
        m_ability.Activate(m_ability1, *this);
        m_timeSinceLastAbility1 = 0;

        m_ability.TransferProjectiles(m_projectiles);
    }
}

void Player::Update(GLFWwindow* window, float deltaTime, Game& game) {
    if(!isMainPlayer) return;
    collisionDetected = false;

    ProcessKeyboardInput(window, deltaTime);
    ProcessMouseInput(window, deltaTime, game);

    UpdateRotation(window, deltaTime, m_reticleOffset);
    
    HandleCollisions(game, deltaTime);
    HandleEntityCollisions(game);

    // Smoke
    float damageRatio = 1.0f - (health / maxHealth);
    glm::vec3 startColor = (health / maxHealth < 0.3f) ? glm::vec3(1.0f, 0.6f, 0.0f) : glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 endColor = glm::vec3(0.2f, 0.2f, 0.2f); // Gray
    game.GetParticles().CreateEmitter(
        ParticleType::SMOKE, 
        position,
        8.0f, 
        static_cast<int>(damageRatio * 10),
        startColor,
        endColor
    );

    // Update Physics
    position += velocity * deltaTime;
    velocity *= dragCoefficient;

    m_timeSinceLastShot += deltaTime;
    m_timeSinceLastAbility1 += deltaTime;
    m_timeSinceLastAbility2 += deltaTime;

    if(glm::length(velocity) > maxSpeed) {
        float speed = glm::length(velocity);
        glm::vec3 velDir = glm::normalize(velocity);
        float lerpFactor = 0.8f; // Adjust for smoothness (0.0 = no change, 1.0 = instant)
        float targetSpeed = glm::mix(speed, maxSpeed, lerpFactor);
        velocity = velDir * targetSpeed;
    }

    m_reticleOffset = glm::vec2(0.0f);
}

void Player::UpdateRotation(GLFWwindow* window, float deltaTime, const glm::vec2& mouseDelta) {
    // Rotation calculations from original Update method
    float yawDelta = mouseDelta.x * yawSpeed * deltaTime;
    float pitchDelta = mouseDelta.y * pitchSpeed * deltaTime;

    // Yaw (local up axis)
    glm::vec3 localUp = GetUp();
    glm::quat yawQuat = glm::angleAxis(glm::radians(yawDelta), localUp);

    // Pitch (local right axis)
    glm::vec3 localRight = GetRight();
    glm::quat pitchQuat = glm::angleAxis(glm::radians(-pitchDelta), localRight);

    // Combine and normalize rotations
    rotation = yawQuat * pitchQuat * roll * rotation;
    rotation = glm::normalize(rotation);
}

void Player::UpdateAI(float deltaTime, const glm::vec3& targetPos, Game& game) {
    glm::vec3 direction = targetPos - position;
    float distanceToTarget = glm::length(direction);

    bool isTargetValid = (game.GetPlayers().size() > game.m_mainPlayerIndex) 
                      && game.GetPlayers()[game.m_mainPlayerIndex].IsAlive();
    if (!isTargetValid) return;

    if(distanceToTarget > 0.1f) {
        direction = glm::normalize(direction);
        
        if(distanceToTarget > AI_AGGRESSION_RANGE) {
            velocity += direction * acceleration * 0.5f * deltaTime;
        }
        
        UpdateAIRotation(direction);
    }
    
    if(distanceToTarget < AI_AGGRESSION_RANGE) {
        AiShooting(deltaTime, direction, game);
    }

    float damageRatio = 1.0f - (health / maxHealth);
    glm::vec3 startColor = (health / maxHealth < 0.3f) ? glm::vec3(1.0f, 0.6f, 0.0f) : glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 endColor = glm::vec3(0.2f, 0.2f, 0.2f); // Gray
    game.GetParticles().CreateEmitter(
        ParticleType::SMOKE, 
        position,
        8.0f, 
        static_cast<int>(damageRatio * 10),
        startColor,
        endColor
    );
    
    HandleCollisions(game, deltaTime);
}

void Player::UpdateAIRotation(const glm::vec3& direction) {
    glm::vec3 forward = glm::normalize(-direction);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    rotation = glm::quatLookAt(forward, up);
}

void Player::Shoot(float deltaTime, Game& game) {
    if (m_shipType == ShipType::XR9) {
            const glm::vec3 right = GetRight();
            const float SPAWN_OFFSET = 0.5f;

            m_projectiles.emplace_back(
                position + right * SPAWN_OFFSET + GetForward() * 1.0f,
                GetForward(), 
                projectileType,
                baseDamage
            );
            m_projectiles.back().sourcePlayer = this;
            
            m_projectiles.emplace_back(
                position - right * SPAWN_OFFSET + GetForward() * 1.0f,
                GetForward(), 
                projectileType,
                baseDamage
            );
            m_projectiles.back().sourcePlayer = this;
    }

    else if(m_shipType == ShipType::HellFire) {
        float SIDE_OFFSET = 0.3f;
        float DEPTH_OFFSET = -5.0f;
        float HEIGHT_OFFSET = -0.1f;
        m_projectiles.emplace_back(
            position + SIDE_OFFSET * GetRight() + DEPTH_OFFSET * GetForward() + HEIGHT_OFFSET * GetUp(),
            GetForward(),
            projectileType,
            baseDamage
        );
        m_projectiles.back().sourcePlayer = this;
    }

        else if(m_shipType == ShipType::HYDRA) {
            float SIDE_OFFSET = 0.45f;
            float DEPTH_OFFSET = 0.0f;
            float HEIGHT_OFFSET = -0.2f;
            m_projectiles.emplace_back(
                position + SIDE_OFFSET * GetRight() + DEPTH_OFFSET * GetForward() + HEIGHT_OFFSET * GetUp(),
                GetForward(),
                projectileType,
                baseDamage
            );
            m_projectiles.back().sourcePlayer = this;

            m_projectiles.emplace_back(
                position - SIDE_OFFSET * GetRight() + DEPTH_OFFSET * GetForward() + HEIGHT_OFFSET * GetUp(),
                GetForward(),
                projectileType,
                baseDamage
            );
            m_projectiles.back().sourcePlayer = this;
        }
}

void Player::AiShooting(float deltaTime, const glm::vec3& targetDir, Game& game) {
    const auto& stats = SHIP_STATS.at(m_shipType);
    m_timeSinceLastShot += deltaTime;

    float effectiveFireRate = stats.fireRate * 1.1f;
    float randomDelay = glm::linearRand(0.0f, 0.15f);
    
    if(m_timeSinceLastShot >= (effectiveFireRate + randomDelay)) {
        const glm::vec3 right = GetRight();
        const float SPAWN_OFFSET = 0.5f;

        // Add some inaccuracy to AI aiming
        glm::vec3 inaccurateDir = targetDir;
        inaccurateDir.x += glm::linearRand(-0.1f, 0.1f);
        inaccurateDir.y += glm::linearRand(-0.1f, 0.1f);
        inaccurateDir.z += glm::linearRand(-0.1f, 0.1f);
        inaccurateDir = glm::normalize(inaccurateDir);

        if (m_shipType == ShipType::XR9) {
                const glm::vec3 right = GetRight();
                const float SPAWN_OFFSET = 0.5f;

                m_projectiles.emplace_back(
                    position + right * SPAWN_OFFSET + GetForward() * 1.0f,
                    GetForward(), 
                    projectileType,
                    baseDamage
                );
                m_projectiles.back().sourcePlayer = this;
                
                m_projectiles.emplace_back(
                    position - right * SPAWN_OFFSET + GetForward() * 1.0f,
                    GetForward(), 
                    projectileType,
                    baseDamage
                );
                m_projectiles.back().sourcePlayer = this;
        }

        else if(m_shipType == ShipType::HellFire) {
            float SIDE_OFFSET = 0.3f;
            float DEPTH_OFFSET = -5.0f;
            float HEIGHT_OFFSET = -0.1f;
            m_projectiles.emplace_back(
                position + SIDE_OFFSET * GetRight() + DEPTH_OFFSET * GetForward() + HEIGHT_OFFSET * GetUp(),
                GetForward(),
                projectileType,
                baseDamage
            );
            m_projectiles.back().sourcePlayer = this;
        }

        else if(m_shipType == ShipType::HYDRA) {
            float SIDE_OFFSET = 0.45f;
            float DEPTH_OFFSET = 0.0f;
            float HEIGHT_OFFSET = -0.2f;
            m_projectiles.emplace_back(
                position + SIDE_OFFSET * GetRight() + DEPTH_OFFSET * GetForward() + HEIGHT_OFFSET * GetUp(),
                GetForward(),
                projectileType,
                baseDamage
            );
            m_projectiles.back().sourcePlayer = this;

            m_projectiles.emplace_back(
                position - SIDE_OFFSET * GetRight() + DEPTH_OFFSET * GetForward() + HEIGHT_OFFSET * GetUp(),
                GetForward(),
                projectileType,
                baseDamage
            );
            m_projectiles.back().sourcePlayer = this;
        }
        
        m_timeSinceLastShot = 0.0f;
    }
}

bool Player::TakeDamage(float damage, Game& game) {
    health -= damage;
    //std::cout << "Damage received = " << damage << std::endl;
    //std::cout << "Current health = " << health << std::endl;
    if(health <= 0) {
        game.GetParticles().CreateEmitter(
            ParticleType::EXPLOSION_BIG,
            position, 
            0.4f,
            100,
            glm::vec3(1.0f, 0.8f, 0.0f), // Start color (yellow)
            glm::vec3(1.0f, 0.0f, 0.0f) // End color (red)
        );
        isAlive = false;
        return true;
    }
    else {
        game.GetParticles().CreateEmitter(
            ParticleType::EXPLOSION_SMALL,
            position,
            0.05f,
            10,
            glm::vec3(1.0f, 0.9f, 0.0f), // Start color (bright yellow)
            glm::vec3(1.0f, 0.5f, 0.0f) // End color (orange)
        );
        return false;
    }
}

void Player::HandleCollisions(Game& game, const float deltaTime) {
    const float terrainHeight = game.GetTerrainHeight(position.x, position.z);
    const float verticalDist = position.y - terrainHeight;

    if(verticalDist < collisionRadius) {
        glm::vec3 normal = game.GetTerrainNormal(position.x, position.z);
        
        // Continuous collision detection
        glm::vec3 prevPos = position - velocity * deltaTime;
        for(int i = 0; i <= 5; i++) {
            float t = i/5.0f;
            glm::vec3 checkPos = glm::mix(prevPos, position, t);
            float checkHeight = game.GetTerrainHeight(checkPos.x, checkPos.z);
            
            if(checkPos.y - checkHeight < collisionRadius and 
            game.m_totalTime - m_lastCollisionTime > COLLISION_DAMAGE_COOLDOWN) {
                m_lastCollisionTime = game.m_totalTime;   
                position = checkPos;
                position.y = checkHeight + collisionRadius;
                
                // Physics response
                glm::vec3 velocityDir = glm::normalize(velocity);
                glm::vec3 reflection = velocityDir - 2.0f * glm::dot(velocityDir, normal) * normal;
                velocity = reflection * glm::length(velocity) * BOUNCE_FACTOR;
                
                // Friction
                glm::vec3 tangentVel = velocity - glm::dot(velocity, normal) * normal;
                velocity -= tangentVel * FRICTION_FACTOR;
                
                collisionDetected = true;
                break;
            }
        }
    }

    const float boundary = MAP_BOUNDARY - collisionRadius;
    if(position.x > boundary || position.x < -boundary) {
        velocity.x = -velocity.x * BOUNCE_FACTOR;
        position.x = glm::clamp(position.x, -boundary, boundary);
        collisionDetected = true;
    }
    if(position.z > boundary || position.z < -boundary) {
        velocity.z = -velocity.z * BOUNCE_FACTOR;
        position.z = glm::clamp(position.z, -boundary, boundary);
        collisionDetected = true;
    }

    if (collisionDetected) {
        TakeDamage(COLLISION_BASE_DAMAGE + glm::length(velocity) * COLLISION_DAMAGE_MULTIPLIER, game);
    }
} 

void Player::HandleEntityCollisions(Game& game) {
    for(auto& enemy : game.GetPlayers()) {
        if (&enemy == this || enemy.team == team || !enemy.isAlive) continue;

        float distance = glm::distance(position, enemy.position);
        float minDistance = collisionRadius + enemy.collisionRadius;
        
        if(distance < minDistance) {
            // Check cooldown
            if((game.m_totalTime - m_lastCollisionTime) > COLLISION_DAMAGE_COOLDOWN) {
                // Apply damage and update cooldown
                bool killed = enemy.TakeDamage(glm::length(velocity+enemy.velocity) * 
                COLLISION_DAMAGE_MULTIPLIER, game);
                m_lastCollisionTime = game.m_totalTime;
                
                if (isMainPlayer) {
                    if (killed) {
                        lastKillTime = game.m_totalTime;
                        game.ReportPlayerKilled();
                    } else {
                        lastHitTime = game.m_totalTime;
                        game.ReportPlayerHit();
                    }
                }
            }
                        
            // Physics response
            glm::vec3 dir = glm::normalize(position - enemy.position);
            velocity += dir * BOUNCE_FACTOR;
            enemy.velocity -= dir * enemy.BOUNCE_FACTOR;
        }
    }
}