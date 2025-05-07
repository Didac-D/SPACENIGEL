#pragma once
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Camera.hpp"
#include "Font.hpp"
#include "Model.hpp"
#include "Particles.hpp"
#include "Player.hpp"
#include "Projectile.hpp"
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace std {
    template <>
    struct hash<std::pair<int, int>> {
        size_t operator()(const std::pair<int, int>& p) const {
            return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
        }
    };
}

class Projectile;
class Player;
class Enemy;  

class Game {
public:
    float m_totalTime = 0.0f;
    float deltaTime;
    float debugUpdateTimer = 0.0f; 
    float DEBUG_UPDATE_INTERVAL = 3.0f;
    static constexpr float MAP_BOUNDARY = 100.0f;

    enum class GameState {
        START_SCREEN,
        MODE_SELECT,
        SHIP_SELECT,
        PLAYING,
        PAUSED, 
        SETTINGS,
        GAME_OVER,
    };
    GameState currentState = GameState::START_SCREEN;
    GameState previousMenuState = GameState::START_SCREEN;
    GameState preSettingsState = GameState::START_SCREEN;
    bool wasMouseRelative = false;

    // Entities
    std::vector<Player> m_players;
    size_t m_mainPlayerIndex = 0;
    std::vector<Projectile> m_projectiles;

    // Getters
    std::vector<Player>& GetPlayers() { return m_players; }
    Particles& GetParticles() { return m_particles; }

    // Initilizers
    Game(GLFWwindow* window);
    bool Initialize();
    bool InitializeShaders();
    bool LoadModels();
    bool LoadTextures();
    bool LoadFonts();
    bool LoadPlacements();

    void GenerateHeightmap(const Model& terrainModel);
    float GetTerrainHeight(float x, float z) const;
    float GetHeightFromMap(float x, float z) const;
    glm::vec3 GetTerrainNormal(float x, float z) const;
    struct Triangle {
        glm::vec3 v0, v1, v2;
        glm::vec3 normal;
    };
    bool IsPointInTriangleXZ(const glm::vec2& point, const Triangle& tri) const;

    // Updaters
    void HandleEvents();
    void Update(float deltaTime);
    void UpdateAllPlayers(float deltaTime);

    void UpdateStartScreen(float deltaTime);
    void UpdateShipSelectScreen(float deltaTime);
    void UpdateModeSelectScreen(float deltaTime);
    void UpdatePauseScreen(float deltaTime);
    void UpdateSettingsScreen(float deltaTime);
    void UpdateGameOverWinScreen(float deltaTime);

    void UpdatePlaying(float deltaTime);
    void ProcessMouseInput(double xpos, double ypos);
    void ProcessMouseInput(int button, int action);
    void UpdateProjectiles(float deltaTime);
    void ReportPlayerKilled() { m_lastKillTime = m_totalTime; }
    void ReportPlayerHit() { m_lastHitTime = m_totalTime; }
    void HandleEntityDestruction();

    // Renderers
    void Render();
    void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
   
    void RenderPlaying();
    void Render3D();
    void RenderMap();
    void RenderEntities();
    void RenderPlayers();
    void RenderProjectiles();
    void RenderParticles();
    void RenderHUD();
    void RenderHealthBar();
    void RenderReticle();
    void RenderHitmarker();
    void RenderSingleHitmarker(GLuint texture, float alpha, float scale);

    void RenderStartScreen();
    void RenderShipSelectScreen();
    void RenderModeSelectScreen();
    void RenderPauseScreen();
    void RenderSettingsScreen();
    void RenderGameOverWinScreen(bool win);

private:
    // OpenGL context and window
    GLFWwindow* m_window;
    Player m_player;
    Camera m_camera;
    glm::mat4 m_projection;

    // Reticle and mouse input
    bool m_firstMouse = true;
    double m_lastX = 400.0f;
    double m_lastY = 300.0f;
    float m_mouseSensitivity = 0.25f;

    unsigned int m_reticleVAO, m_reticleVBO;
    double m_mouseX, m_mouseY;  
    glm::vec2 m_currentReticlePos{0.0f};
    glm::vec2 m_targetReticlePos{0.0f};
    float m_steeringSpeed = 5.0f;
    float m_maxReticleOffset = 0.2f;

    // Shader programs
    unsigned int m_shaderProgram;
    unsigned int m_particleShaderProgram;
    unsigned int m_laserShaderProgram;
    unsigned int m_hudShaderProgram;
    unsigned int m_textShader;
    unsigned int m_uiShaderProgram; 

    // Models
    Model* m_spaceshipModel;
    Model* m_mapModel;
    Model* m_sunModel;
    glm::vec3 m_sunPosition;
    Model* m_enemyModel;
    Model* m_bulletModel;

    // Textures
    GLuint LoadTexture(const char* path, 
        GLenum wrapS = GL_CLAMP_TO_EDGE, 
        GLenum wrapT = GL_CLAMP_TO_EDGE,
        GLenum minFilter = GL_LINEAR, 
        GLenum magFilter = GL_LINEAR);

    GLuint m_currentReticleTex;  // reticle1.png (current aim)
    GLuint m_targetReticleTex;   // reticle2.png (desired aim)
    GLuint m_hitmarkerTex;
    GLuint m_deathHitmarkerTex;
    GLuint m_ParticleBaseTex;
    
    float m_lastHitTime = -1.0f;
    bool m_wasKill = false;
    float m_lastKillTime = -1.0f;
    const float HITMARKER_DURATION = 0.5f;
    const float KILLMARKER_DURATION = 1.0f;

    float m_gridCellSize = 0.5f;
    mutable std::mutex m_spatialGridMutex;
    std::unordered_map<std::pair<int, int>, std::vector<Triangle>> m_spatialGrid;

    Particles m_particles;

    FontLoader* m_font;
    GLuint m_textVAO, m_textVBO;

    // UI element creation and rendering
    struct UIElement {
        GLuint vao = 0;
        GLuint texture;
        glm::vec2 position;
        glm::vec2 size;
    };

    struct MenuButton {
        glm::vec2 position;
        glm::vec2 size;
        std::string text;
        std::function<void()> action;
    };
    
    UIElement m_background;
    UIElement m_title;
    std::vector<UIElement> m_menuButtons;

    std::vector<MenuButton> m_mainMenuButtons;

    void RenderUIElement(const UIElement& element, glm::vec4 color, float alpha);
    void CreateUIElement(UIElement& element, const char* texturePath, glm::vec2 pos, glm::vec2 size);

    void DefineButtons(); 
    void DefineMainMenuButtons();
};