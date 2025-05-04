#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <glm/glm.hpp>

struct Character {
    GLuint     TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    FT_Pos     Advance;
};

class FontLoader {
public:
    std::map<FT_Char, Character> Characters;
    
    FontLoader(const char* fontPath, FT_UInt fontSize);
    ~FontLoader();
    
private:
    FT_Library ft;
    FT_Face face;
};