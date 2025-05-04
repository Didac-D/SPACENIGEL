#pragma once
#include <glm/glm.hpp>

class Player; 
class Game;

class Camera {
public:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    
    float m_distance;
    float m_yaw;
    float m_pitch;
    float m_mouseSensitivity;

    Camera();
    void Update(const Game& game, const glm::vec3& targetPosition, 
        const glm::vec3& targetFront, float deltaTime, const Player& player);
    void ResetFollow() {
        m_position = glm::vec3(0.0f, 2.0f, 5.0f);
        m_front = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    void ProcessMouseMovement(float xoffset, float yoffset);
    void HandleCollision(const Game& game);

    glm::mat4 GetViewMatrix() const;
    glm::vec3 GetPosition() const { return m_position; }
    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
};