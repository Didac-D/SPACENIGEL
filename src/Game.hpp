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

class Game {
public:
    // Basic
    float m_totalTime = 0.0f;
    float deltaTime;
    float debugUpdateTimer = 0.0f; 
    float DEBUG_UPDATE_INTERVAL = 3.0f;
    static constexpr float MAP_BOUNDARY = 100.0f;

    // Settings
    float m_mouseSensitivity = 0.25f;
    bool m_xaxisInvert = 0;
    bool m_yaxisInvert = 0;
    bool m_fullscreen = 0;
    int m_reticleType = 0;
    float m_renderDistance = 600.0f;
    bool m_hideHud = 0;
    bool m_nightMode = 0;

    // Gamestates
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
    GameState previousState = GameState::START_SCREEN;

    bool wasMouseRelative = false;
    double m_scrollOffset = 0.0;

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
    bool ReloadAssets();
    bool InitializeShaders();
    bool LoadModels();
    bool LoadTextures();
    bool LoadFonts();
    bool LoadPlacements();

    bool LoadPersistentSettings();
    bool SavePersistentSettings();

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
    void ProcessMouseInput();
    void ProcessPlayingMouseInput(double xpos, double ypos);
    void ProcessKeyboardInput();
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
    void UpdateProjectiles(float deltaTime);
    void ReportPlayerKilled() { m_lastKillTime = m_totalTime; }
    void ReportPlayerHit() { m_lastHitTime = m_totalTime; }
    void HandleEntityDestruction();

    void DebugOutput(float deltaTime);

    // Renderers
    void Render();
    void RenderText(const std::string& text, glm::vec2 position, glm::vec2 size, glm::vec3 color);
    std::vector<std::string> SplitString(const std::string& str, char delimiter);
   
    void RenderPlaying();
    void Render3D();
    void RenderMap();
    void RenderEntities();
    void RenderPlayers();
    void RenderProjectiles();
    void RenderParticles();
    void RenderHUD();
    void RenderReticle();
    void RenderHitmarker();
    void RenderSingleHitmarker(GLuint texture, float alpha, float scale);

    void RenderStartScreen();
    void RenderShipSelectScreen();
    void RenderChooseAbilityPopUp();
    void RenderSelectModeScreen();
    void RenderPauseScreen();
    void RenderSettingsScreen();
    void RenderGameOverWinScreen(bool win);
    void RenderQuitConfirmationPopUp();

private:
    // OpenGL context and window
    GLFWwindow* m_window;
    Player m_player;
    Camera m_camera;
    glm::mat4 m_projection;

    float m_width = 800, m_heigth = 600;

    int m_windowedPosX = 100, m_windowedPosY = 100;
    int m_windowedWidth = 800, m_windowedHeight = 600;

    // Reticle and mouse input
    bool m_firstMouse = true;
    double m_lastX = 400.0f;
    double m_lastY = 300.0f;

    unsigned int m_reticleVAO = 0, m_reticleVBO = 0;
    double m_mouseX, m_mouseY;  
    glm::vec2 m_currentReticlePos{0.0f, -0.0325f};
    glm::vec2 m_targetReticlePos{0.0f, -0.0325f};
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
    Model* m_explosiveRoundModel;

    FontLoader* m_font;

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

    GLuint m_ability1Tex;
    GLuint m_ability2Tex;
    
    float m_lastHitTime = -1.0f;
    bool m_wasKill = false;
    float m_lastKillTime = -1.0f;
    const float HITMARKER_DURATION = 0.5f;
    const float KILLMARKER_DURATION = 1.0f;

    float m_gridCellSize = 0.5f;
    mutable std::mutex m_spatialGridMutex;
    std::unordered_map<std::pair<int, int>, std::vector<Triangle>> m_spatialGrid;

    Particles m_particles;

    GLuint dummyVAO = 0, dummyVBO = 0;
    GLuint m_textVAO = 0, m_textVBO = 0;
    GLuint m_laserVAO = 0, m_laserVBO = 0;

    // Laser shader uniform locations
    GLint m_locLaserViewProj = -1;
    GLint m_locLaserCameraRight = -1;
    GLint m_locLaserColor = -1;
    GLint m_locLaserThickness = -1;
    GLint m_locLaserAlphaFalloff = -1;

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
        bool hovered = false;
    };

    struct KeyStates {
        bool up = false,        upJustPressed = false;
        bool down = false,      downJustPressed = false;
        bool left = false,      leftJustPressed = false;
        bool right = false,     rightJustPressed = false;
        bool confirm = false,   confirmJustPressed = false;
        bool quit = false,      quitJustPressed = false;

        bool mouseLeft = false, mouseLeftJustPressed = false;
        double scrollOffset = 0.0f; bool scrollJustOffset = false;
    };

    void RenderUIElement(const UIElement& element, glm::vec4 color, float alpha);
    void CreateUIElement(UIElement& element, const char* texturePath, glm::vec2 pos, glm::vec2 size);

    // ========= MENU ELEMENTS ============ //
    KeyStates m_currentKeyStates;
    KeyStates m_prevKeyStates;
    glm::vec2 m_mousePosNDC{0.0f, 0.0f};
    int selectedButtonIndex = 0;

    // Main menu
    std::vector<MenuButton> m_mainMenuButtons;
    std::vector<UIElement> m_mainMenuButtonsBackground;
    UIElement m_title;
    float m_cameraOrbitAngle = 0.0f;

    // Ship Select
    std::vector<MenuButton> m_shipSelectButtons;
    std::vector<UIElement> m_shipSelectButtonsBackground;
    UIElement m_leftArrow;
    UIElement m_rightArrow;
    UIElement m_leftArrowHover;
    UIElement m_rightArrowHover;
    bool chooseAbilityState = false;
    std::vector<MenuButton> m_ChooseAbilityPopUpButtons;
    std::vector<UIElement> m_ChooseAbilityPopUpButtonsBackground;
    int m_selectedAbility = 0;
    int selectedAbilityIndex;
    std::pair<int,int> m_chosenAbilitiesIndex;
    std::pair<AbilityType, AbilityType> m_chosenAbilities = {AbilityType::BOMB, AbilityType::TURBO};
    int shipSelectRow = 2;
    int shipSelectAbilityCol = 0;
    int shipSelectBottomCol = 1;

    // Pause
    std::vector<MenuButton> m_pauseButtons;
    std::vector<UIElement> m_pauseButtonsBackground;

    // Settings
    std::vector<MenuButton> m_settingsButtons;
    std::vector<UIElement> m_settingsButtonsBackground;
    int selectedSection = 0;

    // Quit confirmation
    std::vector<MenuButton> m_QuitConfirmationPopUpButtons;
    std::vector<UIElement> m_QuitConfirmationPopUpButtonsBackground;
    bool confirmQuitState = false;
    int selectedQuitButtonIndex = 0;
    GLuint m_blankTex;

    GLuint m_leftArrowTex;
    GLuint m_rightArrowTex;
    int m_selectedShipIndex = 0;
    float m_shipRotationAngle = 0.0f;

    void DefineButtons(); 
    void DefineMainMenuButtons();
    void DefineShipSelectButtons();
    void DefineChooseAbilityPopUpButtons();
    void DefinePauseButtons();
    void DefineSettingsButtons();
    void DefineQuitConfirmationPopUpButtons();
};