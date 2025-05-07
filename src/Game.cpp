#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include "Game.hpp"
#include "Shaders.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        std::cerr << "GL ERROR: " << message << std::endl;
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    game->ProcessMouseInput(button, action);
}

Game::Game(GLFWwindow* window) : m_window(window) {
    m_projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    m_reticleVAO = 0;
    m_reticleVBO = 0;
    m_textVAO = 0;
    m_textVBO = 0;

    // Callbacks
    glDebugMessageCallback(MessageCallback, 0);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* w, int button, int action, int mods) {
        Game* game = static_cast<Game*>(glfwGetWindowUserPointer(w));
        game->ProcessMouseInput(button, action);
    });

    DefineButtons();
}

void Game::DefineButtons() {
    DefineMainMenuButtons();
}

bool Game::Initialize() {
    glEnable(GL_DEBUG_OUTPUT);
    bool InitSuccess = true;
    if (InitSuccess) InitSuccess = InitializeShaders();
    if (InitSuccess) InitSuccess = LoadModels();
    if (InitSuccess) InitSuccess = LoadTextures();
    if (InitSuccess) InitSuccess = LoadFonts();

    glEnable(GL_DEPTH_TEST);
    glfwSwapInterval(1);

    if (InitSuccess) InitSuccess = LoadPlacements();

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
    CreateUIElement(m_background, "assets/textures/space_bg.jpg", 
                   glm::vec2(0.0f), glm::vec2(2.0f, 2.0f));
    
    CreateUIElement(m_title, "assets/textures/titlecard.png", 
                   glm::vec2(0.0f, 0.6f), glm::vec2(0.8f, 0.3f));
    
    // Menu buttons
    for (const auto& button : m_mainMenuButtons) {
        UIElement elem;
        CreateUIElement(elem, "assets/textures/button_base.png", 
                       button.position, button.size); 
        m_menuButtons.push_back(elem);
    }

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

    unsigned int laserFS = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(laserFS, 1, &laserFragmentShader, NULL);
    glCompileShader(laserFS);

    m_laserShaderProgram = glCreateProgram();
    glAttachShader(m_laserShaderProgram, laserVS);
    glAttachShader(m_laserShaderProgram, laserFS);
    glLinkProgram(m_laserShaderProgram);

    glDeleteShader(laserVS);
    glDeleteShader(laserFS);

    return true;
}

bool Game::LoadModels() {
    try {
        m_mapModel = new Model("assets/maps/map1.obj");
        m_sunModel = new Model("assets/models/sun.obj");
        m_bulletModel = new Model("assets/models/basicprojectile.obj");

        GenerateHeightmap(*m_mapModel);

        SHIP_STATS.at(ShipType::XR9).model = new Model("assets/models/xr9.obj");
        SHIP_STATS.at(ShipType::HellFire).model = new Model("assets/models/hellfire.obj");
    } catch (const std::exception& e) {
        std::cerr << "Model load error: " << e.what() << std::endl;
        return false;
    }

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
            m_sunPosition = glm::vec3(0.0f, 50.0f, 0.0f);
            m_camera.m_position = glm::vec3(10.0f, 15.0f, 20.0f);
            m_camera.m_front = glm::normalize(glm::vec3(0.0f, -5.0f, -20.0f) - m_camera.m_position);
            break;
        }

        case GameState:: PLAYING: {
            m_sunPosition = glm::vec3(0.0f, 50.0f, 0.0f);

            // Main player
            m_players.emplace_back();
            m_mainPlayerIndex = 0;
            m_players[m_mainPlayerIndex].team = 0;
            m_players[m_mainPlayerIndex].isMainPlayer = true;
            m_players[m_mainPlayerIndex].m_shipType = ShipType::XR9;
            m_players[m_mainPlayerIndex].position = glm::vec3(10.0f, 10.0f, 10.0f);

            // AI players
            m_players.emplace_back();
            m_players.back().isAI = true;
            m_players.back().team = 1;
            m_players.back().position = glm::vec3(-50.0f, 6.0f, 30.0f);
            m_players.back().m_shipType = ShipType::XR9;

            m_players.emplace_back();
            m_players.back().isAI = true;
            m_players.back().team = 1;
            m_players.back().position = glm::vec3(1.0f, 3.0f, 20.0f);
            m_players.back().m_shipType = ShipType::XR9;        
        
        }
    }
    return true; 
}

void Game::GenerateHeightmap(const Model& terrainModel) {
    std::lock_guard<std::mutex> lock(m_spatialGridMutex);
    m_spatialGrid.clear();
    const float cellSize = 0.75f;
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

    const float queryRadius = 0.2f; // Account for collision radius
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

void Game::HandleEvents() {
    switch (currentState) {
        case GameState::START_SCREEN: {
            if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                double xpos, ypos;
                glfwGetCursorPos(m_window, &xpos, &ypos);
                int width, height;
                glfwGetWindowSize(m_window, &width, &height);

                // Convert to NDC (-1 to 1)
                float ndcX = (2.0f * xpos / width) - 1.0f;
                float ndcY = 1.0f - (2.0f * ypos / height);

                // Check button collisions
                for (const auto& button : m_mainMenuButtons) {
                    bool xMatch = (ndcX > (button.position.x - button.size.x)) && 
                                  (ndcX < (button.position.x + button.size.x));
                    bool yMatch = (ndcY > (button.position.y - button.size.y)) && 
                                  (ndcY < (button.position.y + button.size.y));
                    
                    if (xMatch && yMatch) {
                        button.action();
                        break;
                    }
                }
            }
            break;
        }

        case GameState::PLAYING: {
            // Add gameplay input handling here
            break;
        }

        case GameState::SETTINGS: {
            // Add settings screen input handling here
            break;
        }

        // Add cases for other states as needed
        default:
            break;
    }
}

void Game::ProcessMouseInput(int button, int action) {

}

void Game::ProcessMouseInput(double xpos, double ypos) {
    if (m_mainPlayerIndex >= m_players.size()) return;

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
    m_players[m_mainPlayerIndex].SetReticleOffset(glm::vec2(
        xoffset * m_mouseSensitivity,
        yoffset * m_mouseSensitivity
    ));
}

void Game::Update(float deltaTime) {
    m_totalTime += deltaTime;
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
        case GameState::PLAYING:
            UpdatePlaying(deltaTime);
            break;
        case GameState::SETTINGS:
            UpdateSettingsScreen(deltaTime);
            break;
        default:
            break;
    }
}

void Game::UpdateStartScreen(float deltaTime) {
    static float orbitAngle = 0.0f; // Degrees
    orbitAngle += 3.0f * deltaTime; // Rotation speed

    // Calculate rotation angles (convert to radians)
    const float pitch = glm::radians(-24.0f); // Original downward angle
    const float yaw = glm::radians(orbitAngle);

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

}

void Game::UpdateModeSelectScreen(float deltaTime) {

}

void Game::UpdatePauseScreen(float deltaTime) {

}

void Game::UpdateSettingsScreen(float deltaTime) {

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
            player.UpdateAI(deltaTime, m_players[m_mainPlayerIndex].position, *this);
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
        case GameState::PLAYING:
            RenderPlaying();
            break;
        case GameState::SETTINGS:
            RenderSettingsScreen();
            break;
        default:
            break;
    }
}

void Game::RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    if (!m_font) return;

    glUseProgram(m_textShader);
    glUniform3f(glGetUniformLocation(m_textShader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_textVAO);

    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
    glUniformMatrix4fv(glGetUniformLocation(m_textShader, "projection"), 1, GL_FALSE, &projection[0][0]);

    for (const char& c : text) {
        auto it = m_font->Characters.find(c);
        if (it == m_font->Characters.end()) continue; // Skip missing characters
        Character ch = it->second;

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

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

        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
    glm::mat4 projection = glm::perspective(glm::radians(50.0f), 800.0f/600.0f, 0.1f, 500.0f);
    
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

        // Calculate text metrics
        float textScale = 0.5f;
        float totalWidthPixels = 0.0f;
        float maxHeightPixels = 0.0f;
        float maxBearingY = 0.0f;

        for (const char& c : button.text) {
            Character ch = m_font->Characters[c];
            totalWidthPixels += (ch.Advance >> 6) * textScale;
            maxHeightPixels = std::max(maxHeightPixels, ch.Size.y * textScale);
            maxBearingY = std::max(maxBearingY, ch.Bearing.y * textScale);
        }

        // Convert button center to screen coordinates
        glm::vec2 screenCenter = {
            (button.position.x + 1.0f) * 0.5f * width,
            (button.position.y + 1.0f) * 0.5f * height
        };

        // Corrected text position calculation
        float textX = screenCenter.x - (totalWidthPixels / 2.0f);
        float textY = screenCenter.y - (maxHeightPixels / 2.0f);

        // Render elements
        // RenderUIElement(m_menuButtons[i], glm::vec4(0.2f, 0.2f, 0.2f, 0.7f), 0.8f);
        RenderText(button.text, textX, textY, textScale, glm::vec3(1.0f));
    }

    glDisable(GL_BLEND);
}

void Game::DefineMainMenuButtons() {
    m_mainMenuButtons = {
        // Start Button
        {   
            .position = {0.0f, 0.1f},
            .size = {0.3f, 0.1f},    
            .text = "START",
            .action = [this]() {
                currentState = GameState::PLAYING;
                LoadPlacements();
                std::cout << "Changing State: Playing" << std::endl; 
            }
        },
        // Settings Button
        {   
            .position = {0.0f, -0.05f},
            .size = {0.3f, 0.1f},
            .text = "SETTINGS",
            .action = [this]() {
                preSettingsState = currentState;
                currentState = GameState::SETTINGS;
                LoadPlacements();
                std::cout << "Changing State: Settings" << std::endl;
            }
        },
        // Quit Button
        {   
            .position = {0.0f, -0.2f},
            .size = {0.3f, 0.1f},
            .text = "QUIT",
            .action = [this]() {
                glfwSetWindowShouldClose(m_window, GL_TRUE);
                std::cout << "Quitting game..." << std::endl; 
            }
        }
    };

    // Initialize menu buttons
    m_menuButtons.clear();
    for (const auto& btn : m_mainMenuButtons) {
        UIElement elem;
        CreateUIElement(elem, "assets/textures/button_base.png", btn.position, btn.size);
        m_menuButtons.push_back(elem);
    }
}

void Game::RenderShipSelectScreen() {

}

void Game::RenderModeSelectScreen() {

}

void Game::RenderPauseScreen() {

}

void Game::RenderSettingsScreen() {

}

void Game::RenderGameOverWinScreen(bool win) {

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
    glUseProgram(m_shaderProgram);

    for (const auto& projectile : m_projectiles) {
        if (projectile.type == ProjectileType::BULLET) {
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
            glm::mat4 viewProj = m_projection * m_camera.GetViewMatrix();
            glUniformMatrix4fv(glGetUniformLocation(m_laserShaderProgram, "uViewProj"), 
                         1, GL_FALSE, &viewProj[0][0]);
        
            glm::vec3 cameraRight = glm::normalize(glm::cross(m_camera.m_front, m_camera.m_up));
            glUniform3fv(glGetUniformLocation(m_laserShaderProgram, "cameraRight"), 
                   1, &cameraRight[0]);
        
            float thickness = 0.6f;
            glUniform1f(glGetUniformLocation(m_laserShaderProgram, "thickness"), thickness);
        
            glUniform3f(glGetUniformLocation(m_laserShaderProgram, "laserColor"), 
                  1.0f, 0.1f, 0.05f);
            glUniform1f(glGetUniformLocation(m_laserShaderProgram, "alphaFalloff"), 2.0f);

            // Calculate end point
            glm::vec3 end = projectile.position + projectile.direction * 50.0f;
        
            // Create quad vertices
            glm::vec3 vertices[] = {
                projectile.position,
                projectile.position,
                end,
                end
            };

            // Setup VAO/VBO
            static GLuint vao = 0, vbo = 0;
            if (vao == 0) {
                glGenVertexArrays(1, &vao);
                glGenBuffers(1, &vbo);
                glBindVertexArray(vao);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
            }
        
            // Draw laser
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);
        
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glBindVertexArray(0);
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
    
    //RenderHealthBar();
    RenderReticle();
    RenderHitmarker();
    
    glUseProgram(currentProgram);
}

void Game::RenderHealthBar() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_hudShaderProgram);
    
    // Health bar background
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "position"), -0.8f, -0.9f);
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "scale"), 1.6f, 0.1f);
    glUniform1f(glGetUniformLocation(m_hudShaderProgram, "alpha"), 0.5f);
    glUniform3f(glGetUniformLocation(m_hudShaderProgram, "color"), 0.2f, 0.2f, 0.2f);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(m_reticleVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Health bar foreground
    float healthWidth = 1.5f * (m_player.health / 100.0f);
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "position"), -0.75f, -0.85f);
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "scale"), healthWidth, 0.05f);
    glUniform1f(glGetUniformLocation(m_hudShaderProgram, "alpha"), 1.0f);
    glUniform3f(glGetUniformLocation(m_hudShaderProgram, "color"), 
        (1.0f - (m_player.health/100.0f)), 
        (m_player.health/100.0f), 
        0.0f
    );
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
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
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "position"), 0.0f, 0.0f);
    glBindTexture(GL_TEXTURE_2D, m_currentReticleTex);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Game::RenderHitmarker() {
    // Use index-based main player reference
    if(m_mainPlayerIndex < m_players.size()) {
        const Player& mainPlayer = m_players[m_mainPlayerIndex];
        if(mainPlayer.lastHitTime > 0 && 
          (m_totalTime - mainPlayer.lastHitTime) < HITMARKER_DURATION) {
            float hitAlpha = 1.0f - (m_totalTime - mainPlayer.lastHitTime)/HITMARKER_DURATION;
            RenderSingleHitmarker(m_hitmarkerTex, hitAlpha, 1.0f);
        }
        if(mainPlayer.lastKillTime > 0 && 
          (m_totalTime - mainPlayer.lastKillTime) < KILLMARKER_DURATION) {
            float killAlpha = 1.0f - (m_totalTime - mainPlayer.lastKillTime)/KILLMARKER_DURATION;
            RenderSingleHitmarker(m_deathHitmarkerTex, killAlpha, 1.5f);
        }
    }
}

void Game::RenderSingleHitmarker(GLuint texture, float alpha, float scale) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_hudShaderProgram);
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "position"), 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(m_hudShaderProgram, "scale"), scale, scale);
    glUniform1f(glGetUniformLocation(m_hudShaderProgram, "alpha"), alpha);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(m_reticleVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}