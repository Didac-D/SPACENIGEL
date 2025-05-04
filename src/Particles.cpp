#include "Particles.hpp"
#include <glm/gtc/random.hpp>
#include <iostream>

Particles::Particles() {
    // Initialize OpenGL buffers
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)0);
    
    glEnableVertexAttribArray(1); // Color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, color));
    
    glBindVertexArray(0);
}

Particles::~Particles() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
}

ParticleEmitter::ParticleEmitter(glm::vec3 pos, ParticleType type, float size, 
    glm::vec3 startColor, glm::vec3 endColor, int count) : 
    position(pos), type(type), size(size), startColor(startColor), endColor(endColor), maxLifetime(2.0f) {
    
        // Initialize particles based on type
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = position;
        
        switch(type) {
            case ParticleType::EXPLOSION_SMALL:
            case ParticleType::EXPLOSION_BIG: {
                p.velocity = glm::sphericalRand(1.0f) * 
                    (type == ParticleType::EXPLOSION_BIG ? 8.0f : 4.0f);
                p.lifetime = glm::linearRand(0.5f, 1.0f);
                break;
            }
            case ParticleType::SMOKE: {
                p.velocity = glm::vec3(
                    glm::gaussRand(0.0f, 0.3f),
                    glm::linearRand(1.0f, 3.0f),
                    glm::gaussRand(0.0f, 0.3f)
                );
                p.lifetime = glm::linearRand(0.3f, 0.6f);
                break;
            }
        }
        p.startLifetime = p.lifetime;
        particles.push_back(p);
    }
}

void Particles::CreateEmitter(
    glm::vec3 position,
    ParticleType type,
    float size,
    glm::vec3 startColor,
    glm::vec3 endColor,
    int count
) {
    m_emitters.emplace_back(
        position, 
        type, 
        size, 
        startColor, 
        endColor, 
        count
    );
}

void Particles::Update(float deltaTime) {
    for (auto& emitter : m_emitters) {
        emitter.duration += deltaTime;

        if (emitter.duration >= emitter.maxLifetime) {
            emitter.particles.clear();
            continue;
        }
        
        for (auto& p : emitter.particles) {
            // Physics update
            p.position += p.velocity * deltaTime;
            p.velocity.y += (emitter.type == ParticleType::SMOKE ? -0.5f : +0.0f) * deltaTime;
            p.lifetime -= deltaTime;
        }
        
        // Remove dead particles
        emitter.particles.erase(
            std::remove_if(emitter.particles.begin(), emitter.particles.end(),
                [](const Particle& p) { return p.lifetime <= 0.0f; }),
            emitter.particles.end()
        );
    }
    
    // Remove expired emitters
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
            [](const ParticleEmitter& e) { 
                return e.particles.empty() || e.duration >= e.maxLifetime;
            }),
        m_emitters.end()
    );
}

void Particles::Render(const glm::mat4& viewProj, GLuint texture) {
    std::vector<ParticleVertex> vertices;
    
    // Prepare vertex data
    for (const auto& emitter : m_emitters) {
        for (const auto& p : emitter.particles) {
            float lifeRatio = p.lifetime / p.startLifetime;
            
            ParticleVertex v;
            v.position = p.position;
            // Interpolate color
            glm::vec3 color = glm::mix(emitter.endColor, emitter.startColor, lifeRatio);
            v.color = glm::vec4(color, lifeRatio);  // Alpha fades with lifetime
            vertices.push_back(v);
        }
    }

    // Upload to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ParticleVertex), vertices.data(), GL_STREAM_DRAW);
    
    // Render settings
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindVertexArray(m_VAO);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    // Draw particles
    glDrawArrays(GL_POINTS, 0, vertices.size());
    
    // Cleanup
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glDisable(GL_PROGRAM_POINT_SIZE);
}