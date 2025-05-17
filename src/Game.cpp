#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include "Game.hpp"
#include "Player.hpp"
#include "Shaders.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        std::cerr << "GL ERROR: " << message << std::endl;
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
}

Game::Game(GLFWwindow* window) : m_window(window) {
    m_projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 600.0f);
    
    // Callbacks
    glDebugMessageCallback(MessageCallback, 0);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* w, int button, int action, int mods) {
        Game* game = static_cast<Game*>(glfwGetWindowUserPointer(w));
    });
}

void Game::DefineButtons() {
    DefineMainMenuButtons();
    DefineShipSelectButtons();
    DefineChooseAbilityPopUpButtons();
    DefinePauseButtons();
    DefineSettingsButtons();
    DefineQuitConfirmationPopUpButtons();
}

bool Game::Initialize() {
    glEnable(GL_DEBUG_OUTPUT);
    bool InitSuccess = true;
    if (InitSuccess) InitSuccess = InitializeShaders();
    if (InitSuccess) InitSuccess = LoadModels();
    if (InitSuccess) InitSuccess = LoadTextures();
    if (InitSuccess) InitSuccess = LoadFonts();
    if (InitSuccess) InitSuccess = LoadPlacements();
    
    DefineButtons();

    glEnable(GL_DEPTH_TEST);
    glfwSwapInterval(1);

    return InitSuccess;
}

bool Game::InitializeShaders() {
    // Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader error:\n" << infoLog << std::endl;
        return false;
    }

    // Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader error:\n" << infoLog << std::endl;
        return false;
    }

    // Shader Program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program error:\n" << infoLog << std::endl;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // UI Shader Program
    unsigned int uiVS = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(uiVS, 1, &uiVertexShader, NULL);
    glCompileShader(uiVS);
    
    unsigned int uiFS = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(uiFS, 1, &uiFragmentShader, NULL);
    glCompileShader(uiFS);
    
    m_uiShaderProgram = glCreateProgram();
    glAttachShader(m_uiShaderProgram, uiVS);
    glAttachShader(m_uiShaderProgram, uiFS);
    glLinkProgram(m_uiShaderProgram);
    
    // Create UI elements
    CreateUIElement(m_title, "assets/textures/titlecard.png", 
                   glm::vec2(0.0f, 0.6f), glm::vec2(0.8f, 0.3f));

    // HUD Shader Program
    unsigned int hudVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(hudVertexShader, 1, &hudVertexShaderSource, NULL);
    glCompileShader(hudVertexShader);

    unsigned int hudFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(hudFragmentShader, 1, &hudFragmentShaderSource, NULL);
    glCompileShader(hudFragmentShader);

    m_hudShaderProgram = glCreateProgram();
    glAttachShader(m_hudShaderProgram, hudVertexShader);
    glAttachShader(m_hudShaderProgram, hudFragmentShader);
    glLinkProgram(m_hudShaderProgram);

    glDeleteShader(hudVertexShader);
    glDeleteShader(hudFragmentShader);

    // Particle Shader Program 
    unsigned int particleVS = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(particleVS, 1, &particleVertexShader, NULL);
    glCompileShader(particleVS);

    unsigned int particleFS = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(particleFS, 1, &particleFragmentShader, NULL);
    glCompileShader(particleFS);

    m_particleShaderProgram = glCreateProgram();
    glAttachShader(m_particleShaderProgram, particleVS);
    glAttachShader(m_particleShaderProgram, particleFS);
    glLinkProgram(m_particleShaderProgram);

    glDeleteShader(particleVS);
    glDeleteShader(particleFS);

    // Laser Shader Program
    unsigned int laserVS = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(laserVS, 1, &laserVertexShader, NULL);
    glCompileShader(laserVS);

    glGetShaderiv(laserVS, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(laserVS, 512, NULL, infoLog);
        std::cerr << "Laser vertex shader error:\n" << infoLog << std::endl;
        return false;
    }

    unsigned int laserFS = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(laserFS, 1, &laserFragmentShader, NULL);
    glCompileShader(laserFS);

    glGetShaderiv(laserFS, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(laserFS, 512, NULL, infoLog);
        std::cerr << "Laser fragment shader error:\n" << infoLog << std::endl;
        return false;
    }

    m_laserShaderProgram = glCreateProgram();
    glAttachShader(m_laserShaderProgram, laserVS);
    glAttachShader(m_laserShaderProgram, laserFS);
    glLinkProgram(m_laserShaderProgram);

    glGetProgramiv(m_laserShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_laserShaderProgram, 512, NULL, infoLog);
        std::cerr << "Laser shader program link error:\n" << infoLog << std::endl;
        return false;
    }

    // Dedicated uniform locations for laser shader
    m_locLaserViewProj =        glGetUniformLocation(m_laserShaderProgram, "uViewProj");
    m_locLaserCameraRight =     glGetUniformLocation(m_laserShaderProgram, "cameraRight");
    m_locLaserColor =           glGetUniformLocation(m_laserShaderProgram, "laserColor");
    m_locLaserThickness =       glGetUniformLocation(m_laserShaderProgram, "thickness");
    m_locLaserAlphaFalloff =    glGetUniformLocation(m_laserShaderProgram, "alphaFalloff");

    if (m_locLaserViewProj == -1 || m_locLaserCameraRight == -1 || m_locLaserColor == -1 
        || m_locLaserThickness == -1 || m_locLaserAlphaFalloff == -1) {
        std::cerr << "Laser shader uniform location error!" << std::endl;
        return false;
    }

    glDeleteShader(laserVS);
    glDeleteShader(laserFS);

    // Dummy VAO
    glGenVertexArrays(1, &dummyVAO);
    glGenBuffers(1, &dummyVBO);

    glBindVertexArray(dummyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dummyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 4, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    return true;
}

bool Game::LoadModels() {
    try {
        m_mapModel =    new Model("assets/maps/map1.obj");
        m_sunModel =    new Model("assets/models/sun.obj");
        m_bulletModel = new Model("assets/models/basicprojectile.obj");
        m_explosiveRoundModel = new Model("assets/models/basicprojectile.obj");

        SHIP_STATS.at(ShipType::XR9).model =        new Model("assets/models/xr9.obj");
        SHIP_STATS.at(ShipType::HellFire).model =   new Model("assets/models/hellfire.obj");
        SHIP_STATS.at(ShipType::HYDRA).model =      new Model("assets/models/hydra.obj");
        SHIP_STATS.at(ShipType::SPEAR).model =      new Model("assets/models/spear.obj");

    } catch (const std::exception& e) {
        std::cerr << "Model load error: " << e.what() << std::endl;
        return false;
    }

    GenerateHeightmap(*m_mapModel);

    return true;
}

bool Game::LoadTextures() {
    m_targetReticleTex = LoadTexture("assets/textures/reticle2.png",
            GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
    m_currentReticleTex = LoadTexture("assets/textures/reticle1.png",
            GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
    m_hitmarkerTex = LoadTexture("assets/textures/hitmarker.png",
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);        
    m_deathHitmarkerTex = LoadTexture("assets/textures/deathhitmarker.png",
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    m_ParticleBaseTex = LoadTexture("assets/textures/particlebase.png",
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    m_leftArrowTex = LoadTexture("assets/textures/sideways_arrow.png",
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    m_rightArrowTex = LoadTexture("assets/textures/sideways_arrow.png",
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    m_blankTex = LoadTexture("assets/textures/blank.png",
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    

    // Reticle VAO and VBO
    float reticleVertices[] = {
        // Positions   // UVs
        -0.05f,  0.05f, 0.0f, 0.0f,
         0.05f,  0.05f, 1.0f, 0.0f,
        -0.05f, -0.05f, 0.0f, 1.0f,
         0.05f, -0.05f, 1.0f, 1.0f
    };
    glGenVertexArrays(1, &m_reticleVAO);
    glGenBuffers(1, &m_reticleVBO);
    glBindVertexArray(m_reticleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_reticleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(reticleVertices), reticleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    return true;
}

GLuint Game::LoadTexture(const char* path, 
                        GLenum wrapS, GLenum wrapT,
                        GLenum minFilter, GLenum magFilter) {
    // Generate texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    // Load image data
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = GL_RGB;
        GLint internalFormat = GL_RGB;
        if (nrChannels == 4) {
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
        } else if (nrChannels == 1) {
            format = GL_RED;
            internalFormat = GL_R8;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    stbi_image_free(data);
    return textureID;
}

bool Game::LoadFonts() {
    try {
        m_font = new FontLoader("assets/fonts/font.ttf", 48);
        
        glGenVertexArrays(1, &m_textVAO);
        glGenBuffers(1, &m_textVBO);
        glBindVertexArray(m_textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Compile text shader
        unsigned int textVS = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(textVS, 1, &textVertexShader, NULL);
        glCompileShader(textVS);
        
        unsigned int textFS = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(textFS, 1, &textFragmentShader, NULL);
        glCompileShader(textFS);
        
        m_textShader = glCreateProgram();
        glAttachShader(m_textShader, textVS);
        glAttachShader(m_textShader, textFS);
        glLinkProgram(m_textShader);
        
        glDeleteShader(textVS);
        glDeleteShader(textFS);

        if (m_font->Characters.empty()) {
            throw std::runtime_error("Font loaded but no characters found");
        }
        return true;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Font load error: " << e.what() << std::endl;
        delete m_font;
        m_font = nullptr;
        return false;
    }
}

bool Game::LoadPlacements() {
    m_players.clear();
    m_projectiles.clear();
    m_particles.Clear();

    switch (currentState) {
        case GameState::START_SCREEN:{
            m_sunPosition = glm::vec3(0.0f, 100.0f, 0.0f);
            m_camera.m_position = glm::vec3(10.0f, 15.0f, 20.0f);
            m_camera.m_front = glm::normalize(glm::vec3(0.0f, -5.0f, -20.0f) - m_camera.m_position);
        }

        case GameState::SHIP_SELECT:{
            m_sunPosition = glm::vec3(-10.0f, 10.0f, -40.0f);
        }

        case GameState:: PLAYING: {
            m_sunPosition = glm::vec3(0.0f, 100.0f, 0.0f);

            // Main player
            m_players.emplace_back();
            m_mainPlayerIndex = 0;
            m_players[m_mainPlayerIndex].team = 0;
            m_players[m_mainPlayerIndex].isMainPlayer = true;
            m_players[m_mainPlayerIndex].position = glm::vec3(10.0f, 10.0f, 10.0f);

            m_players[m_mainPlayerIndex].m_shipType = static_cast<ShipType>(m_selectedShipIndex);
            m_players[m_mainPlayerIndex].m_ability1 = static_cast<AbilityType>(m_chosenAbilities.first);
            m_players[m_mainPlayerIndex].m_ability2 = static_cast<AbilityType>(m_chosenAbilities.second);

            // AI players
            m_players.emplace_back();
            m_players.back().isAI = true;
            m_players.back().team = 1;
            m_players.back().position = glm::vec3(1.0f, 3.0f, 20.0f);
            m_players.back().m_shipType = ShipType::SPEAR; 

            m_players.emplace_back();
            m_players.back().isAI = true;
            m_players.back().team = 1;
            m_players.back().position = glm::vec3(-50.0f, 6.0f, 30.0f);
            m_players.back().m_shipType = ShipType::HellFire;
            
            m_players.emplace_back();
            m_players.back().isAI = true;
            m_players.back().team = 0;
            m_players.back().position = glm::vec3(-6.0f, 10.0f, -41.0f);
            m_players.back().m_shipType = ShipType::HYDRA; 
        
            for (Player &player : m_players) {
                player.InitializeStats();
            }
        }
    }
    return true; 
}

void Game::GenerateHeightmap(const Model& terrainModel) {
    std::lock_guard<std::mutex> lock(m_spatialGridMutex);
    m_spatialGrid.clear();
    const float cellSize = 0.8f;
    m_gridCellSize = cellSize; 

    for (const auto& mesh : terrainModel.Getmeshes()) {
        const auto& vertices = terrainModel.GetVertices();
        const auto& indices = mesh.indices;

        // Safety check for valid triangle data
        if (indices.size() % 3 != 0) {
            std::cerr << "Warning: Mesh has incomplete triangle data ("
                      << indices.size() << " indices)" << std::endl;
            continue;
        }

        for (size_t i = 0; i < indices.size(); i += 3) {
            // Validate index range
            if (indices[i] >= vertices.size() ||
                indices[i+1] >= vertices.size() ||
                indices[i+2] >= vertices.size()) 
            {
                std::cerr << "Warning: Invalid vertex index in triangle "
                          << i/3 << ", skipping" << std::endl;
                continue;
            }

            Triangle tri;
            tri.v0 = vertices[indices[i]];
            tri.v1 = vertices[indices[i+1]];
            tri.v2 = vertices[indices[i+2]];

            // Calculate triangle normal
            glm::vec3 edge1 = tri.v1 - tri.v0;
            glm::vec3 edge2 = tri.v2 - tri.v0;
            tri.normal = glm::normalize(glm::cross(edge1, edge2));

            // Calculate tight AABB
            glm::vec3 minBounds = glm::min(tri.v0, glm::min(tri.v1, tri.v2));
            glm::vec3 maxBounds = glm::max(tri.v0, glm::max(tri.v1, tri.v2));

            // Convert bounds to grid coordinates
            int startX = static_cast<int>(std::floor(minBounds.x / cellSize));
            int endX = static_cast<int>(std::ceil(maxBounds.x / cellSize));
            int startZ = static_cast<int>(std::floor(minBounds.z / cellSize));
            int endZ = static_cast<int>(std::ceil(maxBounds.z / cellSize));

            // Add triangle to all covered cells
            for (int x = startX; x <= endX; x++) {
                for (int z = startZ; z <= endZ; z++) {
                    m_spatialGrid[{x, z}].push_back(tri);
                }
            }
        }
    }
}

float Game::GetTerrainHeight(float x, float z) const {
    std::lock_guard<std::mutex> lock(m_spatialGridMutex);

    const float queryRadius = 0.8f; // Account for collision radius
    float minX = x - queryRadius;
    float maxX = x + queryRadius;
    float minZ = z - queryRadius;
    float maxZ = z + queryRadius;

    float maxHeight = -FLT_MAX;
    
    // Check all grid cells in the search area
    for (int gridX = static_cast<int>(std::floor(minX / m_gridCellSize)); 
         gridX <= static_cast<int>(std::ceil(maxX / m_gridCellSize)); 
         gridX++) {
        for (int gridZ = static_cast<int>(std::floor(minZ / m_gridCellSize));
             gridZ <= static_cast<int>(std::ceil(maxZ / m_gridCellSize));
             gridZ++) {
            if (!m_spatialGrid.count({gridX, gridZ})) continue;
            
            for (const Triangle& tri : m_spatialGrid.at({gridX, gridZ})) {
                // Perform precise ray-triangle intersection
                glm::vec3 origin(x, 1000.0f, z);
                glm::vec3 dir(0.0f, -1.0f, 0.0f);
                
                float t = 0.0f;
                glm::vec2 baryPosition; // Variable to store barycentric coordinates
                if (glm::intersectRayTriangle(origin, dir, tri.v0, tri.v1, tri.v2, baryPosition, t)) {
                    float yHeight = origin.y + dir.y * t;
                    maxHeight = std::max(maxHeight, yHeight);
                }
            }
        }
    }
    
    return maxHeight > -FLT_MAX ? maxHeight : -1000.0f;
}

glm::vec3 Game::GetTerrainNormal(float x, float z) const {
    const float queryRadius = 0.8f; // Match collision check radius
    float minX = x - queryRadius;
    float maxX = x + queryRadius;
    float minZ = z - queryRadius;
    float maxZ = z + queryRadius;

    // Check all grid cells in the search area
    for (int gridX = static_cast<int>(std::floor(minX / m_gridCellSize)); 
         gridX <= static_cast<int>(std::ceil(maxX / m_gridCellSize)); 
         gridX++) {
        for (int gridZ = static_cast<int>(std::floor(minZ / m_gridCellSize));
             gridZ <= static_cast<int>(std::ceil(maxZ / m_gridCellSize));
             gridZ++) {
            const auto cellKey = std::make_pair(gridX, gridZ);
            if (!m_spatialGrid.count(cellKey)) continue;
            
            for (const Triangle& tri : m_spatialGrid.at(cellKey)) {
                if (IsPointInTriangleXZ(glm::vec2(x, z), tri)) {
                    return tri.normal;
                }
            }
        }
    }
    return glm::vec3(0.0f, 1.0f, 0.0f); // Fallback
}

bool Game::IsPointInTriangleXZ(const glm::vec2& point, const Triangle& tri) const {
    const float epsilon = 0.001f;
    glm::vec2 a(tri.v0.x, tri.v0.z);
    glm::vec2 b(tri.v1.x, tri.v1.z);
    glm::vec2 c(tri.v2.x, tri.v2.z);

    glm::vec2 v0 = b - a;
    glm::vec2 v1 = c - a;
    glm::vec2 v2 = point - a;

    float den = v0.x * v1.y - v1.x * v0.y;
    if (std::abs(den) < epsilon) return false;
    
    float u = (v2.x * v1.y - v2.y * v1.x) / den;
    float v = (v2.y * v0.x - v2.x * v0.y) / den;
    
    return (u >= -epsilon) && (v >= -epsilon) && (u + v <= 1.0f + epsilon);
}

void Game::CreateUIElement(UIElement& element, const char* texturePath, glm::vec2 pos, glm::vec2 size) {
    // Geometry setup
    float vertices[] = {
        -0.5f,  0.5f, 0.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &element.vao);
    GLuint vbo;
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(element.vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Load texture
    element.texture = LoadTexture(texturePath, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    element.position = pos;
    element.size = size;
}

void Game::ProcessPlayingMouseInput(double xpos, double ypos) {
    if (currentState == GameState::PLAYING && m_mainPlayerIndex < m_players.size()) {
        if (m_firstMouse) {
            m_lastX = xpos;
            m_lastY = ypos;
            m_firstMouse = false;
        }

        // Calculate raw offsets
        float xoffset = (m_lastX - xpos);
        float yoffset = (m_lastY - ypos);
        m_lastX = xpos;
        m_lastY = ypos;

        // Apply sensitivity and send to player
        glm::vec2 reticleOffset(
            xoffset * m_mouseSensitivity,
            yoffset * m_mouseSensitivity
        );
        m_players[m_mainPlayerIndex].SetReticleOffset(reticleOffset);

        // Delegate to player's mouse input processing
        m_players[m_mainPlayerIndex].ProcessMouseInput(m_window, 0.0f, *this);
    }
}

void Game::ProcessMouseInput() {
    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);

    // Convert to NDC
    m_mousePosNDC.x = static_cast<float>((2.0 * xpos) / width - 1.0);
    m_mousePosNDC.y = static_cast<float>(1.0 - (2.0 * ypos) / height);

    // Current state
    m_currentKeyStates.mouseLeft = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    
    // Just pressed calculation
    m_currentKeyStates.mouseLeftJustPressed = m_currentKeyStates.mouseLeft && !m_prevKeyStates.mouseLeft;

    m_prevKeyStates = m_currentKeyStates;
}

void Game::ProcessKeyboardInput() {
    // Current states
    m_currentKeyStates.up =     (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) || 
                                (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS);
    m_currentKeyStates.down =   (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) || 
                                (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS);
    m_currentKeyStates.left =   (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) || 
                                (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS);
    m_currentKeyStates.right =  (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) || 
                                (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS);
    m_currentKeyStates.confirm =(glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS) || 
                                (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS);
    m_currentKeyStates.quit =   (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS);

    // Just pressed calculations
    m_currentKeyStates.upJustPressed =      m_currentKeyStates.up && !m_prevKeyStates.up;
    m_currentKeyStates.downJustPressed =    m_currentKeyStates.down && !m_prevKeyStates.down;
    m_currentKeyStates.leftJustPressed =    m_currentKeyStates.left && !m_prevKeyStates.left;
    m_currentKeyStates.rightJustPressed =   m_currentKeyStates.right && !m_prevKeyStates.right;
    m_currentKeyStates.confirmJustPressed = m_currentKeyStates.confirm && !m_prevKeyStates.confirm;
    m_currentKeyStates.quitJustPressed =    m_currentKeyStates.quit && !m_prevKeyStates.quit;

    m_prevKeyStates = m_currentKeyStates;
}

void Game::HandleEvents() {
    auto UpdateButtonHoverStates = [&](auto& buttons, int& selectedIndex) {
        bool anyHovered = false;

        // Update hover states based on mouse position
        for (size_t i = 0; i < buttons.size(); ++i) {
            auto& button = buttons[i];
            const bool xMatch = (m_mousePosNDC.x > (button.position.x - button.size.x)) &&
                                (m_mousePosNDC.x < (button.position.x + button.size.x));
            const bool yMatch = (m_mousePosNDC.y > (button.position.y - button.size.y)) &&
                                (m_mousePosNDC.y < (button.position.y + button.size.y));
            button.hovered = xMatch && yMatch;

            if (button.hovered) {
                anyHovered = true;
                selectedIndex = static_cast<int>(i); // Update selected index to match hovered button
            }
        }

        // If no button is hovered, highlight the button selected by the keyboard
        if (!anyHovered) {
            for (size_t i = 0; i < buttons.size(); ++i) {
                buttons[i].hovered = (static_cast<int>(i) == selectedIndex);
            }
        }
    };

    if (confirmQuitState) {
        UpdateButtonHoverStates(m_QuitConfirmationPopUpButtons, selectedQuitButtonIndex);
        
        if (m_currentKeyStates.quitJustPressed) {
            confirmQuitState = false;
        }

        if (m_currentKeyStates.leftJustPressed || m_currentKeyStates.rightJustPressed) {
            selectedQuitButtonIndex = (selectedQuitButtonIndex +
                                       (m_currentKeyStates.leftJustPressed ? -1 : 1) +
                                        m_QuitConfirmationPopUpButtons.size()) %
                                        m_QuitConfirmationPopUpButtons.size();
        }

        if (m_currentKeyStates.confirmJustPressed) {
            m_QuitConfirmationPopUpButtons[selectedQuitButtonIndex].action();
        }

        if (m_currentKeyStates.mouseLeftJustPressed) {
            for (auto& button : m_QuitConfirmationPopUpButtons) {
                if (button.hovered) {
                    button.action();
                    break;
                }
            }
        }
    } 

    else if (chooseAbilityState) {
        UpdateButtonHoverStates(m_ChooseAbilityPopUpButtons, selectedAbilityIndex);

        // ADD THE CONDITIONS LATER, WHEN THERES AN APPROPIATE NUMBER OF ABILITIES
        if (m_currentKeyStates.leftJustPressed) {
            //if (selectedAbilityIndex != 0 && selectedAbilityIndex != 2)
                selectedAbilityIndex = (selectedAbilityIndex - 1) % m_ChooseAbilityPopUpButtons.size();
        }
        if (m_currentKeyStates.rightJustPressed) {
            //if (selectedAbilityIndex != 1 && selectedAbilityIndex != 4)
                selectedAbilityIndex = (selectedAbilityIndex + 1) % m_ChooseAbilityPopUpButtons.size();
        }
        if (m_currentKeyStates.upJustPressed) {
            //if (selectedAbilityIndex != 1 && selectedAbilityIndex != 2)
                selectedAbilityIndex = (selectedAbilityIndex + 3) % m_ChooseAbilityPopUpButtons.size();
        }
        if (m_currentKeyStates.downJustPressed) {
            //if (selectedAbilityIndex != m_ChooseAbilityPopUpButtons.size() && 
                                //selectedAbilityIndex != m_ChooseAbilityPopUpButtons.size()-1 && 
                                //selectedAbilityIndex != m_ChooseAbilityPopUpButtons.size()-2)
                selectedAbilityIndex = (selectedAbilityIndex - 3) % m_ChooseAbilityPopUpButtons.size();
        }

        if (m_currentKeyStates.confirmJustPressed) {
            m_ChooseAbilityPopUpButtons[selectedAbilityIndex].action();
        }

        // Mouse click
        if (m_currentKeyStates.mouseLeftJustPressed) {
            for (auto& button : m_ChooseAbilityPopUpButtons) {
                if (button.hovered) {
                    button.action();
                    break;
                }
            }
        }

        if (m_currentKeyStates.quitJustPressed) {
            chooseAbilityState = false;
        }
    }
    
    else {
        switch (currentState) {
            case GameState::START_SCREEN: {
                UpdateButtonHoverStates(m_mainMenuButtons, selectedButtonIndex);

                // Keyboard navigation
                if (m_currentKeyStates.upJustPressed) {
                    selectedButtonIndex = (selectedButtonIndex - 1 + m_mainMenuButtons.size()) %
                                          m_mainMenuButtons.size();
                }
                if (m_currentKeyStates.downJustPressed) {
                    selectedButtonIndex = (selectedButtonIndex + 1) % m_mainMenuButtons.size();
                }

                // Confirm action
                if (m_currentKeyStates.confirmJustPressed) {
                    m_mainMenuButtons[selectedButtonIndex].action();
                }

                // Mouse click
                if (m_currentKeyStates.mouseLeftJustPressed) {
                    for (auto& button : m_mainMenuButtons) {
                        if (button.hovered) {
                            button.action();
                            break;
                        }
                    }
                }

                if (m_currentKeyStates.quitJustPressed) {
                    confirmQuitState = true;
                    std::cout << "Quit Confirmation" << std::endl;
                }
                break;
            }

            case GameState::SHIP_SELECT: {
                UpdateButtonHoverStates(m_shipSelectButtons, selectedButtonIndex);

                // UP/DOWN navigation between rows (0=abilities, 1=ship, 2=bottom)
                if (m_currentKeyStates.upJustPressed) {
                    shipSelectRow = (shipSelectRow - 1 + 3) % 3;
                }
                if (m_currentKeyStates.downJustPressed) {
                    shipSelectRow = (shipSelectRow + 1) % 3;
                }

                // LEFT/RIGHT navigation within row
                if (shipSelectRow == 0) {
                    // Ability selection row
                    if (m_currentKeyStates.leftJustPressed || m_currentKeyStates.rightJustPressed) {
                        shipSelectAbilityCol = 1 - shipSelectAbilityCol; 
                    }
                    selectedButtonIndex = 4 + shipSelectAbilityCol;
                    // Confirm opens ability popup
                    if (m_currentKeyStates.confirmJustPressed) {
                        m_shipSelectButtons[selectedButtonIndex].action();
                    }
                } else if (shipSelectRow == 1) {
                    // Ship selection row (middle)
                    if (m_currentKeyStates.leftJustPressed) {
                        m_selectedShipIndex = (m_selectedShipIndex - 1 + SHIP_STATS.size()) % SHIP_STATS.size();
                    }
                    if (m_currentKeyStates.rightJustPressed) {
                        m_selectedShipIndex = (m_selectedShipIndex + 1) % SHIP_STATS.size();
                    }
                    selectedButtonIndex = 0;
                } else if (shipSelectRow == 2) {
                    // Bottom buttons row
                    if (m_currentKeyStates.leftJustPressed || m_currentKeyStates.rightJustPressed) {
                        shipSelectBottomCol = 1 - shipSelectBottomCol;
                    }
                    selectedButtonIndex = 2 + shipSelectBottomCol;
                    // Confirm activates button
                    if (m_currentKeyStates.confirmJustPressed) {
                        m_shipSelectButtons[selectedButtonIndex].action();
                    }
                }

                // Mouse click
                if (m_currentKeyStates.mouseLeftJustPressed) {
                    for (auto& button : m_shipSelectButtons) {
                        if (button.hovered) {
                            button.action();
                            break;
                        }
                    }
                }

                // Return to main menu
                if (m_currentKeyStates.quitJustPressed) {
                    currentState = GameState::START_SCREEN;
                    std::cout << "Changing State: Start Screen" << std::endl; 
                }
                break;
            }

            case GameState::PLAYING: {
                if (m_currentKeyStates.quitJustPressed) {
                    currentState = GameState::PAUSED;
                    std::cout << "Changing State: Paused" << std::endl; 
                }
                break;
            }

            case GameState::PAUSED: {
                UpdateButtonHoverStates(m_pauseButtons, selectedButtonIndex);

                // Keyboard navigation
                if (m_currentKeyStates.upJustPressed) {
                    selectedButtonIndex = (selectedButtonIndex - 1 + m_pauseButtons.size()) %
                                          m_pauseButtons.size();
                }
                if (m_currentKeyStates.downJustPressed) {
                    selectedButtonIndex = (selectedButtonIndex + 1) % m_pauseButtons.size();
                }

                // Confirm action
                if (m_currentKeyStates.confirmJustPressed) {
                    m_pauseButtons[selectedButtonIndex].action();
                }

                // Mouse click
                if (m_currentKeyStates.mouseLeftJustPressed) {
                    for (auto& button : m_pauseButtons) {
                        if (button.hovered) {
                            button.action();
                            break;
                        }
                    }
                }

                if (m_currentKeyStates.quitJustPressed) {
                    currentState = GameState::PLAYING;
                    std::cout << "Changing State: Playing" << std::endl; 
                }

                break;
            }

            case GameState::SETTINGS: {
                UpdateButtonHoverStates(m_settingsButtons, selectedButtonIndex);

                if (m_currentKeyStates.mouseLeftJustPressed) {
                    for (auto& button : m_settingsButtons) {
                        if (button.hovered) {
                            button.action();
                            break;
                        }
                    }
                }

                if (m_currentKeyStates.quitJustPressed) {
                    currentState = previousState;
                    std::cout << "Changing State: Previous" << std::endl; 
                }

                break;
            }
        }
    }
}

void Game::Update(float deltaTime) {
    m_totalTime += deltaTime;

    ProcessMouseInput();
    ProcessKeyboardInput();
    HandleEvents();

    if (currentState == GameState::PLAYING) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    switch (currentState) {
        case GameState::START_SCREEN:
            UpdateStartScreen(deltaTime);
            break;
        case GameState::SHIP_SELECT:
            UpdateShipSelectScreen(deltaTime);
            break;
        case GameState::PLAYING:
            UpdatePlaying(deltaTime);
            break;
        case GameState::PAUSED:
            UpdatePauseScreen(deltaTime);
            break;
        case GameState::SETTINGS:
            UpdateSettingsScreen(deltaTime);
            break;
        default:
            break;
    }
}

void Game::UpdateStartScreen(float deltaTime) {
    m_cameraOrbitAngle += 3.0f * deltaTime; // Rotation speed

    // Calculate rotation angles (convert to radians)
    const float pitch = glm::radians(-24.0f); // Original downward angle
    const float yaw = glm::radians(m_cameraOrbitAngle);

    // Calculate new front vector
    glm::vec3 front;
    front.x = cos(yaw) * cos(pitch);
    front.y = sin(pitch);
    front.z = sin(yaw) * cos(pitch);

    m_camera.m_front = glm::normalize(front);

    glUseProgram(m_shaderProgram);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPos"), 1, &m_sunPosition[0]);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightColor"), 1.0f, 0.95f, 0.9f);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &m_camera.GetPosition()[0]);
}

void Game::UpdateShipSelectScreen(float deltaTime) {
    // Keep rotating the ship model around X-axis
    m_shipRotationAngle += 25.0f * deltaTime;
    if(m_shipRotationAngle >= 360.0f) m_shipRotationAngle -= 360.0f;

    // Set fixed camera position and look direction
    m_camera.m_position = glm::vec3(2.0f, 2.0f, -44.0f);
    glm::vec3 shipPos(-9.0f, 2.0f, -40.0f);
    glm::vec3 lookDir = shipPos - m_camera.m_position;
    m_camera.m_front = glm::normalize(lookDir);

    // Maintain lighting uniforms
    glUseProgram(m_shaderProgram);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPos"), 1, &m_sunPosition[0]);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightColor"), 0.7f, 0.665f, 0.63f);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &m_camera.GetPosition()[0]);

    // Handle ship selection bounds
    if(m_selectedShipIndex < 0) m_selectedShipIndex = SHIP_STATS.size() - 1;
    if(m_selectedShipIndex >= SHIP_STATS.size()) m_selectedShipIndex = 0;
}

void Game::UpdateModeSelectScreen(float deltaTime) {

}

void Game::UpdatePauseScreen(float deltaTime) {
    // Maintain lighting uniforms
    glUseProgram(m_shaderProgram);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPos"), 1, &m_sunPosition[0]);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightColor"), 1.0f, 0.95f, 0.9f);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &m_camera.GetPosition()[0]);
}

void Game::UpdateSettingsScreen(float deltaTime) {
    // Maintain lighting uniforms
    glUseProgram(m_shaderProgram);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPos"), 1, &m_sunPosition[0]);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightColor"), 1.0f, 0.95f, 0.9f);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &m_camera.GetPosition()[0]);
}

void Game::UpdateGameOverWinScreen(float deltaTime) {

}

void Game::UpdatePlaying(float deltaTime) {
    UpdateAllPlayers(deltaTime);
    if (m_mainPlayerIndex < m_players.size() && m_players[m_mainPlayerIndex].IsAlive()) {
        m_camera.Update(
            *this, 
            m_players[m_mainPlayerIndex].position, 
            m_players[m_mainPlayerIndex].GetForward(), 
            deltaTime, 
            m_players[m_mainPlayerIndex]
        );
    }
    m_particles.Update(deltaTime);

    UpdateProjectiles(deltaTime);
    HandleEntityDestruction();

    // Update lighting uniforms
    glUseProgram(m_shaderProgram);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPos"), 1, &m_sunPosition[0]);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightColor"), 1.0f, 0.95f, 0.9f);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &m_camera.GetPosition()[0]);
    
    // Set default glow parameters
    glUseProgram(m_shaderProgram);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "outlineThickness"), 0.05f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "outlineColor"), 1.0f, 0.0f, 0.0f);

    //DebugOutput(deltaTime);
}

void Game::DebugOutput(float deltaTime) {
    // Throttled debug output
    debugUpdateTimer += deltaTime;
    if(debugUpdateTimer >= DEBUG_UPDATE_INTERVAL) {
        std::cout << "\n--- DEBUG INFO ---\n";
        std::cout << "FPS: " << (1.0f / deltaTime) << "\n";
        std::cout << "Player Alive: " << m_players[m_mainPlayerIndex].IsAlive() << "\n";
        std::cout << "Player Position: " << m_players[m_mainPlayerIndex].position.x << ", "
                  << m_players[m_mainPlayerIndex].position.y << ", "
                  << m_players[m_mainPlayerIndex].position.z << "\n";
        
        debugUpdateTimer = 0.0f;
    }
}

void Game::UpdateAllPlayers(float deltaTime) {
    for(auto& player : m_players) {
        if (!player.IsAlive()) continue;
        if(player.isAI) {
            //player.UpdateAI(deltaTime, m_players[m_mainPlayerIndex].position, *this);
        }
        else {
            player.Update(m_window, deltaTime, *this);
        }
        player.TransferProjectiles(m_projectiles);
    }
}

void Game::UpdateProjectiles(float deltaTime) {
    for(auto& projectile : m_projectiles) {
        projectile.Update(deltaTime, m_players, *this);
    }
}

void Game::HandleEntityDestruction() {
    // Projectiles
    auto projectileEnd = std::remove_if(
        m_projectiles.begin(), m_projectiles.end(),
        [](const Projectile& p) { return p.ShouldDestroy(); }
    );
    m_projectiles.erase(projectileEnd, m_projectiles.end());
}

void Game::Render() {
    glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    switch (currentState) {
        case GameState::START_SCREEN:
            RenderStartScreen();
            break;
        case GameState::SHIP_SELECT:
            RenderShipSelectScreen();
            break;
        case GameState::PLAYING:
            RenderPlaying();
            break;
        case GameState::PAUSED:
            RenderPauseScreen();
            break;
        case GameState::SETTINGS:
            RenderSettingsScreen();
            break;
        default:
            break;
    }

    if (confirmQuitState == true) {
        RenderQuitConfirmationPopUp();
    }
    if (chooseAbilityState == true) {
        RenderChooseAbilityPopUp();
    }
}

void Game::RenderText(const std::string& text, glm::vec2 position, glm::vec2 size, glm::vec3 color) {
    if (!m_font) return;

    glUseProgram(m_textShader);
    glUniform3f(glGetUniformLocation(m_textShader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_textVAO);

    // Use NDC-based orthographic projection
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(m_textShader, "projection"), 1, GL_FALSE, &projection[0][0]);

    // Adjust scale to fit text height within the button
    float scale = size.y * 0.02f;

    // Calculate total width of the text in NDC
    float totalWidth = 0.0f;
    for (const char& c : text) {
        auto it = m_font->Characters.find(c);
        if (it == m_font->Characters.end()) continue;
        Character ch = it->second;
        totalWidth += (ch.Advance >> 6) * scale; // Advance width
    }

    // Adjust starting position to center the text within the button
    float startX = position.x - (totalWidth / 2.0f);
    float startY = position.y - (size.y / 2.0f) + (scale / 2.0f); // Center vertically

    // Render each character
    for (const char& c : text) {
        auto it = m_font->Characters.find(c);
        if (it == m_font->Characters.end()) continue;
        Character ch = it->second;

        // Calculate character position and size in NDC
        float xpos = startX + ch.Bearing.x * scale;
        float ypos = startY - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance to the next character
        startX += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

std::vector<std::string> Game::SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void Game::RenderUIElement(const UIElement& element, glm::vec4 color, float alpha) {
    if (element.texture == 0 || element.vao == 0 || !glIsVertexArray(element.vao)) {
        return; // Skip invalid elements
    }
    glUseProgram(m_uiShaderProgram);
    
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(m_uiShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    glUniform2fv(glGetUniformLocation(m_uiShaderProgram, "position"), 1, &element.position[0]);
    glUniform2fv(glGetUniformLocation(m_uiShaderProgram, "size"), 1, &element.size[0]);
    glUniform4fv(glGetUniformLocation(m_uiShaderProgram, "color"), 1, &color[0]);
    glUniform1f(glGetUniformLocation(m_uiShaderProgram, "alpha"), alpha);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, element.texture);
    glBindVertexArray(element.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Game::RenderStartScreen() {
    // Render 3D map background
    glEnable(GL_DEPTH_TEST);
    glUseProgram(m_shaderProgram);
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(50.0f), 800.0f/600.0f, 0.1f, 600.0f);
    
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    RenderMap();

    // Render UI elements
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render title
    RenderUIElement(m_title, glm::vec4(1.0f), 1.0f);

    for (int i = m_mainMenuButtons.size() - 1; i >= 0; --i) {
        auto& button = m_mainMenuButtons[i];
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);

        // Render elements
        //RenderUIElement(m_mainMenuButtonsBackground[i], glm::vec4(0.2f, 0.2f, 0.2f, 0.7f), 0.8f);

        glm::vec3 textColor = button.hovered ? 
        glm::vec3(1.0f, 1.0f, 0.0f) :   // Yellow when hovered
        glm::vec3(1.0f);                // White normally
        RenderText(button.text, button.position, button.size, textColor);
    }

    glDisable(GL_BLEND);
}

void Game::DefineMainMenuButtons() {
    m_mainMenuButtons = {
        // Start Button
        {   
            .position = {0.0f, 0.05f},
            .size = {0.3f, 0.1f},    
            .text = "START",
            .action = [this]() {
                currentState = GameState::SHIP_SELECT;
                LoadPlacements();
                std::cout << "Changing State: Ship Select" << std::endl; 
            }
        },
        // Settings Button
        {   
            .position = {0.0f, -0.15f},
            .size = {0.3f, 0.1f},
            .text = "SETTINGS",
            .action = [this]() {
                previousState = currentState;
                currentState = GameState::SETTINGS;
                LoadPlacements();
                std::cout << "Changing State: Settings" << std::endl;
            }
        },
        // Quit Button
        {   
            .position = {0.0f, -0.35f},
            .size = {0.3f, 0.1f},
            .text = "QUIT",
            .action = [this]() {
                confirmQuitState = true;
                std::cout << "Quit Confirmation" << std::endl; 
            }
        },
    };

    // Initialize menu buttons
    m_mainMenuButtonsBackground.clear();
    for (const auto& btn : m_mainMenuButtons) {
        UIElement elem;
        CreateUIElement(elem, "assets/textures/button_base.png", btn.position, btn.size);
        m_mainMenuButtonsBackground.push_back(elem);
    }
}

void Game::RenderShipSelectScreen() {
    // 1. Render darkened background
    glEnable(GL_DEPTH_TEST);
    glUseProgram(m_shaderProgram);
    
    // Use original start screen camera setup
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(50.0f), 800.0f/600.0f, 0.1f, 500.0f);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Darken the scene by reducing light intensity
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightColor"), 0.6f, 0.57f, 0.54f);
    RenderMap();

    // 2. Render rotating ship model
    ShipType selectedType = SHIP_ORDER[m_selectedShipIndex];
    Model* shipModel = SHIP_STATS.at(selectedType).model;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-6.0f, 1.0f, -41.0f)); // Fixed position
    model = glm::rotate(model, glm::radians(m_shipRotationAngle), glm::vec3(0.0f, 0.5f, 0.0f)); // X-axis rotation
    model = glm::scale(model, glm::vec3(0.25f));

    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "objectColor"), 1.0f, 1.0f, 1.0f);
    shipModel->Draw(m_shaderProgram);
    
    // 3. UI Elements
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Plane Name
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);

    std::string planeName = SHIP_STATS.at(selectedType).name;
    float ndcX = 0.0f;
    float ndcY = 0.1f;
    RenderText(planeName, glm::vec2(0.0f, 0.1f), glm::vec2(0.5f, 0.1f), glm::vec3(1.0f));

    // Define stat names and values
    const ShipStats& stats = SHIP_STATS.at(selectedType);

    ProjectileType selectedProjectileType = SHIP_STATS.at(selectedType).projectileType;
    std::ostringstream fireRateStream;
    fireRateStream << std::fixed << std::setprecision(1) << (1 / stats.fireRate);

    std::vector<std::pair<std::string, std::string>> statLines = {
        {"Max Health:   ", std::to_string(static_cast<int>(stats.maxHealth))},
        {"", ""},
        {"Projectile:   ", (selectedProjectileType == ProjectileType::NONE) ? "NONE" : PROJECTILE_STATS.at(selectedProjectileType).name},
        {"Fire Rate:    ", (selectedProjectileType == ProjectileType::NONE) ? "NONE" : fireRateStream.str()},
        {"", ""},
        {"Max Speed:    ", std::to_string(static_cast<int>(stats.maxSpeed))},
        {"Acceleration: ", std::to_string(static_cast<int>(stats.acceleration))}
    };

    // Abilities

    // Calculate alignment offsets
    float statsY = 0.9f;
    float lineHeight = 0.075f;
    float nameColumnX = -0.45f; // X position for stat names
    float valueColumnX = 0.065f; // X position for stat values

    for (const auto& [name, value] : statLines) {
        // Render stat name (left-aligned)
        RenderText(name, glm::vec2(nameColumnX, statsY), glm::vec2(0.8f, 0.065f), glm::vec3(1.0f));

        // Render stat value (right-aligned)
        RenderText(value, glm::vec2(valueColumnX, statsY), glm::vec2(0.8f, 0.065f), glm::vec3(1.0f));

        statsY -= lineHeight;
    }


    // Navigation arrows
    RenderUIElement(m_leftArrow, glm::vec4(1.0f), 1.0f);
    RenderUIElement(m_rightArrow, glm::vec4(1.0f), 1.0f);

    for (const auto& button : m_shipSelectButtons) {
        if (button.text.empty() && button.hovered) {
            if (button.position.x < 0) {
                RenderUIElement(m_leftArrow, glm::vec4(1.0f, 1.0f, 0.0f, 0.3f), 1.0f);
            } else {
                RenderUIElement(m_rightArrow, glm::vec4(1.0f, 1.0f, 0.0f, 0.3f), 1.0f);
            }
        }
    }

    // Control buttons
    for (auto& button : m_shipSelectButtons) {
        if (!button.text.empty()) { // Skip arrow buttons
            glm::vec2 screenPos = button.position;

            glm::vec3 textColor = button.hovered ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f);
            RenderText(button.text, button.position, button.size, textColor);
        }
    }
    glDisable(GL_BLEND);
}

void Game::DefineShipSelectButtons() {
    m_shipSelectButtons = {
        {   // Left arrow button
            .position = {-0.8f, 0.0f},
            .size = {0.2f, 0.3f},
            .text = "", // No text
            .action = [this]() {
                m_selectedShipIndex = (m_selectedShipIndex - 1 + SHIP_STATS.size()) % SHIP_STATS.size();
            }
        },
        {   // Right arrow button
            .position = {0.8f, 0.0f},
            .size = {0.2f, 0.3f},
            .text = "", // No text
            .action = [this]() {
                m_selectedShipIndex = (m_selectedShipIndex + 1) % SHIP_STATS.size();
            }
        },
        {   // Back button
            .position = {-0.7f, -0.8f},
            .size = {0.2f, 0.1f},
            .text = "BACK",
            .action = [this]() { 
                currentState = GameState::START_SCREEN;
                LoadPlacements();
            }
        },
        {   // Play button
            .position = {0.6f, -0.8f},
            .size = {0.2f, 0.1f},
            .text = "PLAY",
            .action = [this]() {
                ShipType selectedType = SHIP_ORDER[m_selectedShipIndex];
                m_players[m_mainPlayerIndex].m_shipType = selectedType;
                currentState = GameState::PLAYING;
                LoadPlacements();
            }
        },
        {   // Ability 1
            .position = {0.6f, 0.8f},
            .size = {0.1f, 0.1f},
            .text = ABILITY_PARAMS.at(m_chosenAbilities.first).name,
            .action = [this]() {
                chooseAbilityState = true;
                m_selectedAbility = 1; 
            }
        },
        {   // Ability 2
            .position = {0.6f, 0.6f},
            .size = {0.1f, 0.1f},
            .text = ABILITY_PARAMS.at(m_chosenAbilities.second).name,
            .action = [this]() {
                chooseAbilityState = true;
                m_selectedAbility = 2; 
            }
        }
    };

    CreateUIElement(m_leftArrow, "assets/textures/sideways_arrow.png", 
                   glm::vec2(-0.8f, 0.0f), glm::vec2(0.2f, 0.3f));
    CreateUIElement(m_rightArrow, "assets/textures/sideways_arrow.png", 
                   glm::vec2(0.8f, 0.0f), glm::vec2(-0.2f, 0.3f));

    m_shipSelectButtonsBackground.clear();
    for (const auto& btn : m_shipSelectButtons) {
        UIElement elem;
        CreateUIElement(elem, "assets/textures/button_base.png", btn.position, btn.size);
        m_shipSelectButtonsBackground.push_back(elem);
    }
}

void Game::RenderChooseAbilityPopUp() {
    if (!chooseAbilityState) return;

    // Dark Overlay
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    UIElement darkOverlay;
    CreateUIElement(darkOverlay, "assets/textures/blank.png", glm::vec2(0.0f), glm::vec2(2.0f, 2.0f));
    RenderUIElement(darkOverlay, glm::vec4(0.0f, 0.0f, 0.0f, 0.8f), 1.0f);

    // Popup Panel
    UIElement popupPanel;
    CreateUIElement(popupPanel, "assets/textures/blank.png", glm::vec2(0.0f, 0.0f), glm::vec2(1.4f, 1.4f));
    RenderUIElement(popupPanel, glm::vec4(0.4f, 0.4f, 0.4f, 1.0f), 1.0f);

    
    for (int i = 0; i <  m_ChooseAbilityPopUpButtons.size(); i++) {
        RenderUIElement(m_ChooseAbilityPopUpButtonsBackground[i], glm::vec4(0.2f, 0.2f, 0.2f, 0.7f), 0.8f);

        glm::vec2 screenPos = m_ChooseAbilityPopUpButtons[i].position;
        glm::vec3 textColor = m_ChooseAbilityPopUpButtons[i].hovered ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f);
        if (i == 0 && m_selectedAbility == 1 || i == 1 && m_selectedAbility == 2) 
            textColor =  glm::vec3(1.0f, 1.0f, 0.0f);

        RenderText(m_ChooseAbilityPopUpButtons[i].text, 
            m_ChooseAbilityPopUpButtons[i].position, 
            m_ChooseAbilityPopUpButtons[i].size, 
            textColor);
    }

    glDisable(GL_BLEND);
}

void Game::DefineChooseAbilityPopUpButtons() {
    m_ChooseAbilityPopUpButtons = {
        {   // CURRENT ABILITY 1
            .position = {-0.3f, 0.6f},
            .size = {0.1f, 0.1f},
            .text = ABILITY_PARAMS.at(m_chosenAbilities.first).name,
            .action = [this]() {
                m_selectedAbility = 1; 
            }
        },
        {   // CURRENT ABILITY 2
            .position = {0.3f, 0.6f},
            .size = {0.1f, 0.1f},
            .text = ABILITY_PARAMS.at(m_chosenAbilities.second).name,
            .action = [this]() {
                m_selectedAbility = 2;
            }
        },
        {   // BOMB
            .position = {-0.5f, 0.3f},
            .size = {0.075f, 0.075f},
            .text = "BOMB",
            .action = [this]() {
                if (m_selectedAbility == 1)
                    m_chosenAbilities.first = AbilityType::BOMB;
                else 
                    m_chosenAbilities.second = AbilityType::BOMB;

                DefineChooseAbilityPopUpButtons();
                DefineShipSelectButtons();
            }
        },
        {   // TURBO
            .position = {0.0f, 0.3f},
            .size = {0.075f, 0.075f},
            .text = "TURBO",
            .action = [this]() {
                if (m_selectedAbility == 1) 
                    m_chosenAbilities.first = AbilityType::TURBO;
                else 
                    m_chosenAbilities.second = AbilityType::TURBO;
                    
                DefineChooseAbilityPopUpButtons();
                DefineShipSelectButtons();
            }
        }
    }; 

    m_ChooseAbilityPopUpButtonsBackground.clear();
    for (const auto& btn : m_ChooseAbilityPopUpButtons) {
        UIElement elem;
        CreateUIElement(elem, "assets/textures/button_base.png", btn.position, btn.size);
        m_ChooseAbilityPopUpButtonsBackground.push_back(elem);
    }
}

void Game::RenderSelectModeScreen() {

}

void Game::RenderPauseScreen() {
    // Render the current 3D scene
    Render3D();

    // Apply a dark filter
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    UIElement darkOverlay;
    CreateUIElement(darkOverlay, "assets/textures/blank.png", glm::vec2(0.0f), glm::vec2(2.0f, 2.0f));
    RenderUIElement(darkOverlay, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f), 1.0f);

    // Render buttons
    for (int i = m_pauseButtons.size() - 1; i >= 0; --i) {
        auto& button = m_pauseButtons[i];
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);

        // Render elements
        //RenderUIElement(m_pauseButtonsBackground[i], glm::vec4(0.2f, 0.2f, 0.2f, 0.7f), 0.8f);

        glm::vec3 textColor = button.hovered ? 
        glm::vec3(1.0f, 1.0f, 0.0f) :   // Yellow when hovered
        glm::vec3(1.0f);                // White normally
        RenderText(button.text, button.position, button.size, textColor);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Game::DefinePauseButtons() {
    m_pauseButtons = {
        {   // Resume Button
            .position = {0.0f, 0.2f},
            .size = {0.3f, 0.1f},
            .text = "RESUME",
            .action = [this]() {
                currentState = GameState::PLAYING; // Directly set to PLAYING
                std::cout << "Changing State: Playing" << std::endl;
            }
        },
        {   // Settings Button
            .position = {0.0f, 0.0f},
            .size = {0.3f, 0.1f},
            .text = "SETTINGS",
            .action = [this]() {
                previousState = currentState; // Save the current state
                currentState = GameState::SETTINGS;
                std::cout << "Changing State: Settings" << std::endl;
            }
        },
        {   // Quit Button
            .position = {0.0f, -0.2f},
            .size = {0.3f, 0.1f},
            .text = "QUIT",
            .action = [this]() {
                confirmQuitState = true;
                std::cout << "Quit Confirmation" << std::endl;
            }
        }
    };

    m_pauseButtonsBackground.clear();
    for (const auto& btn : m_pauseButtons) {
        UIElement elem;
        CreateUIElement(elem, "assets/textures/button_base.png", btn.position, btn.size);
        m_pauseButtonsBackground.push_back(elem);
    }
}

void Game::RenderSettingsScreen() {
    Render3D();

    // Apply a dark filter
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    UIElement darkOverlay;
    CreateUIElement(darkOverlay, "assets/textures/blank.png", glm::vec2(0.0f), glm::vec2(2.0f, 2.0f));
    RenderUIElement(darkOverlay, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f), 1.0f);

    for (int i = m_settingsButtons.size() - 1; i >= 0; i--) {
        if (i != 0 && i != 5 && i != 8 && i != 10)
            switch (selectedSection) {
                case 0:
                    if (i > 4) continue;
                    break;
                case 1:
                    if (i < 5 || i > 7) continue;
                    break;
                case 2:
                    if (i < 8) continue;
                    break;
            }

        auto& button = m_settingsButtons[i];
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);

        // Render elements
        //RenderUIElement(m_settingsButtonsBackground[i], glm::vec4(0.2f, 0.2f, 0.2f, 0.7f), 0.8f);

        glm::vec3 textColor = button.hovered ? 
        glm::vec3(1.0f, 1.0f, 0.0f) :   // Yellow when hovered
        glm::vec3(1.0f);                // White normally
        RenderText(button.text, button.position, button.size, textColor);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Game::DefineSettingsButtons() {
    m_settingsButtons = {
        {   // GAMEPLAY SECTION HEAD
            .position = {-0.6f, 0.8f},
            .size = {0.05f, 0.05f},
            .text = "GAMEPLAY",
            .action = [this]() {
                selectedSection = 0;
            }
        },
        {   //  Sensitivity
            .position = {0.0f, 0.4f},
            .size = {0.035f, 0.035f},
            .text = "Sensitivity",
            .action = [this]() {

            }
        },
        {   // X-Axis invert
            .position = {0.0f, 0.2f},
            .size = {0.035f, 0.035f},
            .text = "X-Axis invert",
            .action = [this]() {

            }
        },
        {   // Y-Axis invert
            .position = {0.0f, 0.0f},
            .size = {0.035f, 0.035f},
            .text = "Y-Axis invert",
            .action = [this]() {

            }
        },
        {   // Fullscreen
            .position = {0.0f, -0.2f},
            .size = {0.035f, 0.035f},
            .text = "Fullscreen",
            .action = [this]() {

            }
        },

        {   // GRAPHICS SECTION HEAD
            .position = {0.0f, 0.8f},
            .size = {0.05f, 0.05f},
            .text = "GRAPHICS",
            .action = [this]() {
                selectedSection = 1;
            }
        },
        {   // ReticleSomethingSomething
            .position = {0.0f, 0.4f},
            .size = {0.035f, 0.035f},
            .text = "ReticleSomethingSomething",
            .action = [this]() {

            }
        },
        {   // Render Distance
            .position = {0.0f, 0.2f},
            .size = {0.035f, 0.035f},
            .text = "Render Distance",
            .action = [this]() {

            }
        },

        {   // MISC SECTION HEAD
            .position = {0.6f, 0.8f},
            .size = {0.05f, 0.05f},
            .text = "MISCELLANIA",
            .action = [this]() {
                selectedSection = 2;
            }
        },
        {   // NIGHT MODE
            .position = {0.0f, 0.4f},
            .size = {0.035f, 0.035f},
            .text = "NIGHT MODE",
            .action = [this]() {

            }
        },

        {   // BACK
            .position = {0.0f, -0.6f},
            .size = {0.05f, 0.05f},
            .text = "BACK",
            .action = [this]() {
                currentState = previousState;
                std::cout << "Changing state to previous" << std::endl;
            }   
        }
    };
}

void Game::RenderGameOverWinScreen(bool win) {

}

void Game::RenderQuitConfirmationPopUp() {
    if (!confirmQuitState) return;

    // 1. Dark overlay
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Fullscreen dark overlay using dedicated blank texture
    UIElement darkOverlay;
    CreateUIElement(darkOverlay, "assets/textures/blank.png", glm::vec2(0.0f), glm::vec2(2.0f, 2.0f));
    RenderUIElement(darkOverlay, glm::vec4(0.0f, 0.0f, 0.0f, 0.8f), 1.0f);

    // 2. Popup panel
    UIElement popupPanel;
    CreateUIElement(popupPanel, "assets/textures/blank.png", glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.4f));
    RenderUIElement(popupPanel, glm::vec4(0.4f, 0.4f, 0.4f, 1.0f), 1.0f);

    // 3. Centered question text with proper scaling
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    std::string question = "Are you sure?";
    
    // Calculate text width
    float textWidth = 0.0f;
    for (const char& c : question) {
        Character ch = m_font->Characters[c];
        textWidth += (ch.Advance >> 6) * 0.5f;
    }
    
    float ndcX = 0.0f; // Centered horizontally in NDC
    float ndcY = 0.0f; // Centered vertically in NDC

    RenderText(question, glm::vec2(0.0f, 0.075f), glm::vec2(0.4f, 0.07f), glm::vec3(1.0f));

    // 4. Buttons
    for (size_t i = 0; i < m_QuitConfirmationPopUpButtons.size(); ++i) {
        const auto& button = m_QuitConfirmationPopUpButtons[i];
        
        // Button background
        UIElement btnBg;
        CreateUIElement(btnBg, "assets/textures/button_base.png", 
            button.position, glm::vec2(0.2f, 0.1f));
        // RenderUIElement(btnBg, button.hovered ? glm::vec4(0.4f, 0.4f, 0.4f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 1.0f), 1.0f);

        // Centered button text
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        
        float screenX = button.position.x; 
        float screenY = button.position.y; 
        
        // Calculate text width for centering
        float btnTextWidth = 0.0f;
        for (const char& c : button.text) {
            Character ch = m_font->Characters[c];
            btnTextWidth += (ch.Advance >> 6) * 0.4f;
        }
        
        float textX = button.position.x - (btnTextWidth / 2.0f) / width;
        float textY = button.position.y - 0.01f;

        glm::vec3 textColor = button.hovered ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f);
        RenderText(button.text, button.position, button.size, textColor);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Game::DefineQuitConfirmationPopUpButtons() {
    m_QuitConfirmationPopUpButtons =   {
        // Confirm Quit
        {
            .position = {-0.15f, -0.08f},
            .size = {0.1f, 0.05f},
            .text = "YES",
            .action = [this]() {
                switch (currentState) {
                    case GameState::START_SCREEN:
                        glfwSetWindowShouldClose(m_window, GL_TRUE);
                        std::cout << "Quitting game..." << std::endl; 
                    
                    case GameState::PAUSED:
                        currentState = GameState::START_SCREEN;
                        confirmQuitState = false;
                        std::cout << "Changing State: Start Screen" << std::endl;
                }

            }
        },
        // Cancel Quit
        {
            .position = {0.15f, -0.08f},
            .size = {0.1f, 0.05f},
            .text = "NO",
            .action = [this]() {
                confirmQuitState = false;
                std::cout << "Quitting canceled" << std::endl; 
            }
        }
    };

    m_QuitConfirmationPopUpButtonsBackground.clear();
    for (const auto& btn : m_QuitConfirmationPopUpButtons) {
        UIElement elem;
        CreateUIElement(elem, "assets/textures/button_base.png", btn.position, btn.size);
        m_QuitConfirmationPopUpButtonsBackground.push_back(elem);
    }
}

void Game::RenderPlaying() {
    Render3D();
    RenderHUD();
}

void Game::Render3D() {
    glUseProgram(m_shaderProgram);
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::mat4 proj = m_projection;
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, &m_camera.GetViewMatrix()[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, &m_projection[0][0]);

    RenderMap();
    RenderEntities();
    RenderParticles();
}

void Game::RenderMap() {
    if (!m_mapModel) return;
    // Set up model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -10.0f, 0.0f)); // Lower position
    model = glm::scale(model, glm::vec3(1.0f)); // Start with scale 1.0

    // Set shader uniforms
    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "objectColor"), 0.8f, 0.8f, 0.8f);
    
    // Debug: Force depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Draw map
    m_mapModel->Draw(m_shaderProgram);
}

void Game::RenderEntities() {
    RenderPlayers();
    RenderProjectiles();
}

void Game::RenderPlayers() {
    glUseProgram(m_shaderProgram);

    for(auto& player : m_players) {
        if (!player.IsAlive()) continue;

        const auto& ShipModel = SHIP_STATS.at(player.m_shipType).model;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, player.position);
        model *= glm::mat4_cast(player.rotation);
        model = glm::scale(model, glm::vec3(0.1f));

        if(!player.isMainPlayer) {
            // Outline Pass (Backfaces)
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            
            glm::mat4 outlineModel = model;
            outlineModel = glm::scale(outlineModel, glm::vec3(1.1f));
            
            glUniform1i(glGetUniformLocation(m_shaderProgram, "isOutline"), GL_TRUE);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "outlineColor"), 
                       player.team == 0 ? 0.0f : 1.0f,  // R
                       player.team == 0 ? 0.0f : 0.0f,  // G
                       player.team == 0 ? 1.0f : 0.0f); // B
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, &outlineModel[0][0]);
            ShipModel->Draw(m_shaderProgram);

            // Reset to normal rendering
            glCullFace(GL_BACK);
            glDisable(GL_CULL_FACE);
            glUniform1i(glGetUniformLocation(m_shaderProgram, "isOutline"), GL_FALSE);
        }

        // Main Model Pass
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        ShipModel->Draw(m_shaderProgram);
    }
}

void Game::RenderProjectiles() {
    for (const auto& projectile : m_projectiles) {
        if (projectile.type == ProjectileType::BULLET) {
            glUseProgram(m_shaderProgram);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), projectile.position);
            model = glm::scale(model, glm::vec3(0.07f));
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 
                1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "objectColor"), 
                1.0f, 0.2f, 0.2f);
            m_bulletModel->Draw(m_shaderProgram);
        } 

        else if (projectile.type == ProjectileType::LASER) {
            glUseProgram(m_laserShaderProgram);

            // Bind VAO and VBO
            glBindVertexArray(dummyVAO);
            glBindBuffer(GL_ARRAY_BUFFER, dummyVBO);

            // Update vertex data
            float laserLength = projectile.speed * projectile.lifetime;
            glm::vec3 vertices[] = {
                projectile.position,
                projectile.position,
                projectile.position + projectile.direction * laserLength,
                projectile.position + projectile.direction * laserLength
            };
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

            // Set uniforms using dedicated locations
            glm::mat4 viewProj = m_projection * m_camera.GetViewMatrix();
            glm::vec3 cameraRight = glm::normalize(glm::cross(m_camera.m_front, m_camera.m_up));
            glUniformMatrix4fv(m_locLaserViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
            glUniform3fv(m_locLaserCameraRight, 1, glm::value_ptr(cameraRight));
            glUniform3f(m_locLaserColor, 1.0f, 0.1f, 0.05f);
            glUniform1f(m_locLaserThickness, 0.5f);
            glUniform1f(m_locLaserAlphaFalloff, 1.0f);
            
            // Draw
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Cleanup
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        if (projectile.type == ProjectileType::EXPLOSIVE) {
            glUseProgram(m_shaderProgram);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), projectile.position);
            model = glm::scale(model, glm::vec3(0.14f));
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 
                1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(m_shaderProgram, "objectColor"), 
                0.8f, 0.0f, 0.8f);
            m_explosiveRoundModel->Draw(m_shaderProgram);
        } 
    }
}

void Game::RenderParticles() {
    glUseProgram(m_particleShaderProgram);
    glm::mat4 viewProj = m_projection * m_camera.GetViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_particleShaderProgram, "uViewProj"), 1, GL_FALSE, &viewProj[0][0]);
    glUniform1f(glGetUniformLocation(m_particleShaderProgram, "pointSize"), 8.0f);
    
    m_particles.Render(viewProj, m_ParticleBaseTex);
}

void Game::RenderHUD() {
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    
    RenderReticle();
    RenderHitmarker();
    
    glUseProgram(currentProgram);
}

void Game::RenderReticle() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_hudShaderProgram);

    // Reset critical uniforms
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "scale"), 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(m_hudShaderProgram, "alpha"), 1.0f);

    // Draw target reticle
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "position"), 
               m_targetReticlePos.x, m_targetReticlePos.y);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_targetReticleTex);
    glBindVertexArray(m_reticleVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Draw current reticle (center)
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "position"), 
               m_currentReticlePos.x, m_currentReticlePos.y);
    glBindTexture(GL_TEXTURE_2D, m_currentReticleTex);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Game::RenderHitmarker() {
    const Player& mainPlayer = m_players[m_mainPlayerIndex];

    // Only use the main player's hit/kill times
    if (mainPlayer.lastHitTime > 0 && 
        (m_totalTime - mainPlayer.lastHitTime) < HITMARKER_DURATION) {
        float hitAlpha = 1.0f - (m_totalTime - mainPlayer.lastHitTime) / HITMARKER_DURATION;
        RenderSingleHitmarker(m_hitmarkerTex, hitAlpha, 1.0f);
    }
    if (mainPlayer.lastKillTime > 0 && 
        (m_totalTime - mainPlayer.lastKillTime) < KILLMARKER_DURATION) {
        float killAlpha = 1.0f - (m_totalTime - mainPlayer.lastKillTime) / KILLMARKER_DURATION;
        RenderSingleHitmarker(m_deathHitmarkerTex, killAlpha, 1.5f);
    }
}

void Game::RenderSingleHitmarker(GLuint texture, float alpha, float scale) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_hudShaderProgram);
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "position"), 
                m_targetReticlePos.x, m_targetReticlePos.y);
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "scale"), scale, scale);
    glUniform1f(glGetUniformLocation(m_hudShaderProgram, "alpha"), alpha);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(m_reticleVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}