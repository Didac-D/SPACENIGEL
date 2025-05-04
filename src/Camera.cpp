#define GLM_ENABLE_EXPERIMENTAL
#include "Game.hpp"
#include "Camera.hpp"
#include "Player.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>

Camera::Camera() 
    : m_position(0.0f, 2.0f, 5.0f),
      m_distance(5.0f),             // Default distance from target 
      m_front(0.0f, 0.0f, -1.0f),   // Initialize to look along -Z
      m_up(0.0f, 1.0f, 0.0f),       // World up axis
      m_right(1.0f, 0.0f, 0.0f),    // Initial right vector
      m_mouseSensitivity(0.1f) {}   // Explicit sensitivity initialization

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw += xoffset;
    m_pitch -= yoffset;

    m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::Update(const Game& game, const glm::vec3& targetPosition, const glm::vec3& targetFront,
    float deltaTime, const Player& player) {
    // Position interpolation code
    glm::vec3 baseOffset = -targetFront * m_distance;
    glm::vec3 verticalOffset = glm::vec3(0.0f, 1.0f, 0.0f); 
    glm::vec3 adjustedTarget = targetPosition - glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 idealPosition = targetPosition + baseOffset + verticalOffset;
    m_position = glm::mix(m_position, idealPosition, 15.0f * deltaTime);

    HandleCollision(game);
    
    // Calculate rolled "up" vector based on player's roll
    m_front = glm::normalize(adjustedTarget - m_position);
    
    float extractedRoll = player.GetRoll();
    glm::quat rollRotation = glm::angleAxis(glm::radians(extractedRoll), m_front);
    glm::vec3 rolledUp = player.GetUp();
    
    // Ensure orthogonality between front/right/up
    m_front = glm::normalize(targetFront);
    m_right = glm::normalize(glm::cross(m_front, rolledUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::HandleCollision(const Game& game) {
    // Prevent camera from clipping through terrain
    const float terrainHeight = game.GetTerrainHeight(m_position.x, m_position.z);
    const float minCameraHeight = terrainHeight + 1.5f; // 1.5m clearance
    
    if(m_position.y < minCameraHeight) {
        m_position.y = minCameraHeight;
    }

    // Optional: Add boundary constraints
    const float boundary = Game::MAP_BOUNDARY - 2.0f;
    m_position.x = glm::clamp(m_position.x, -boundary, boundary);
    m_position.z = glm::clamp(m_position.z, -boundary, boundary);
}