#include "Game.hpp"
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

GLFWwindow* CreateWindow() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    GLFWwindow* window = glfwCreateWindow(800, 600, "SpaceNigel", NULL, NULL);

    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowPos(window, (mode->width - 800)/2, (mode->height - 600)/2);
    }

    return window;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game && game->currentState == Game::GameState::PLAYING) {
        game->ProcessPlayingMouseInput(xpos, ypos);
    }
}

int main() {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) return -1;

    GLFWwindow* window = CreateWindow();
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    gladLoadGL();

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GL_TRUE);

    Game game(window);
    glfwSetWindowUserPointer(window, &game);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!game.Initialize()) {
        std::cerr << "Game initialization failed!" << std::endl;
        return -1;
    }

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        game.Update(deltaTime);
        game.Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
