#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

enum class ParticleType {
    EXPLOSION_SMALL,
    EXPLOSION_BIG,
    SMOKE
};

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float lifetime;
    float startLifetime;
};

struct ParticleVertex {
    glm::vec3 position;
    glm::vec4 color;
};

class ParticleEmitter {
public:
    ParticleType type;
    glm::vec3 position;
    float size;
    glm::vec3 startColor;
    glm::vec3 endColor;
    float maxLifetime;
    float duration = 0.0f;
    std::vector<Particle> particles;

    ParticleEmitter(ParticleType type, glm::vec3 pos, float size, int count,
        glm::vec3 startColor, glm::vec3 Endcolor);
};

class Particles {
    GLuint m_VAO, m_VBO;
    std::vector<ParticleEmitter> m_emitters;
    
public:
    Particles();
    ~Particles();
    
    void CreateEmitter(
        ParticleType type,
        glm::vec3 position, 
        float size,
        int count,
        glm::vec3 startColor,
        glm::vec3 endColor
    );

    void Update(float deltaTime);
    void Render(const glm::mat4& viewProj, GLuint texture);
    void Clear() { m_emitters.clear(); }
};