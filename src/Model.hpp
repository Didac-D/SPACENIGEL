#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/scene.h>

class Model {
public:
    Model(const std::string& path);
    void Draw(unsigned int shaderProgram);

    struct Mesh {
        unsigned int VAO, VBO, EBO;
        unsigned int indexCount;
        std::vector<unsigned int> indices;
    };

    size_t GetMeshCount() const;
    const std::vector<glm::vec3>& GetVertices() const { return vertices;}
    const std::vector<Mesh>& Getmeshes() const { return meshes; }
    
private:
    std::vector<Mesh> meshes;
    std::vector<glm::vec3> vertices;
    std::string directory;

    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};