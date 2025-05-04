#include "Model.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <iostream>

Model::Model(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs |
        aiProcess_GenNormals); 

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    
    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
}

void Model::Draw(unsigned int shaderProgram) {
    for(const auto& mesh : meshes) {
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // Process all meshes in node
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    
    // Process children nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

size_t Model::GetMeshCount() const {
    return meshes.size();
}

Model::Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    // Process vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Positions
        vertices.push_back({
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        });
        
        // Normals
        if(mesh->HasNormals()) {
            normals.push_back({
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            });
        } else {
            // Default normal (up vector) if missing
            normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
        // Default color (will be overridden in Game.cpp)
        colors.push_back(glm::vec3(0.7f));
    }
    this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());

    // Process indices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Setup OpenGL buffers
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    // Combine data into one buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 
                vertices.size() * sizeof(glm::vec3) + 
                colors.size() * sizeof(glm::vec3) +
                normals.size() * sizeof(glm::vec3),
                nullptr, GL_STATIC_DRAW);

    // Positions
    glBufferSubData(GL_ARRAY_BUFFER, 0, 
                   vertices.size() * sizeof(glm::vec3), vertices.data());
    
    // Colors
    glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
                   colors.size() * sizeof(glm::vec3), colors.data());
    
    // Normals
    glBufferSubData(GL_ARRAY_BUFFER, 
                   (vertices.size() + colors.size()) * sizeof(glm::vec3),
                   normals.size() * sizeof(glm::vec3), normals.data());

    // Element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                indices.size() * sizeof(unsigned int), 
                indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                        (void*)(vertices.size() * sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                        (void*)((vertices.size() + colors.size()) * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return {VAO, VBO, EBO, static_cast<unsigned int>(indices.size()), indices};
}