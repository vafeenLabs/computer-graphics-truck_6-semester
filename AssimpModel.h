#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "shader_s.h"
#include <soil.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>

// Структура для хранения данных модели
struct Mesh {
    unsigned int VAO;
    unsigned int textureID;
    unsigned int vertexCount;

    Mesh(const std::vector<float>& vertices, unsigned int texture) {
        glGenVertexArrays(1, &VAO);
        unsigned int VBO;
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        // Позиции вершин
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Нормали
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Текстурные координаты
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        vertexCount = vertices.size() / 8;
        textureID = texture;
    }
};

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = SOIL_load_image(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        SOIL_free_image_data(data);
    }
    else {
        std::cout << "Failed to load texture at path: " << path << std::endl;
    }
    return textureID;
}

// Функция загрузки модели
std::vector<Mesh> loadModel(const std::string& path) {
    std::vector<Mesh> meshes;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return meshes;
    }

    auto processMesh = [&](aiMesh* mesh) -> Mesh {
        std::vector<float> vertices;
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            vertices.push_back(mesh->mVertices[i].x);
            vertices.push_back(mesh->mVertices[i].y);
            vertices.push_back(mesh->mVertices[i].z);
            vertices.push_back(mesh->HasNormals() ? mesh->mNormals[i].x : 0.0f);
            vertices.push_back(mesh->HasNormals() ? mesh->mNormals[i].y : 0.0f);
            vertices.push_back(mesh->HasNormals() ? mesh->mNormals[i].z : 0.0f);
            if (mesh->mTextureCoords[0]) {
                vertices.push_back(mesh->mTextureCoords[0][i].x);
                vertices.push_back(mesh->mTextureCoords[0][i].y);
            }
            else {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }

        // Загрузка текстуры
        unsigned int textureID = loadTexture("path/to/your/texture.png");
        return Mesh(vertices, textureID);
        };

    auto processNode = [&](aiNode* node, const aiScene* scene, auto&& processNodeRef) -> void {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNodeRef(node->mChildren[i], scene, processNodeRef);
        }
        };

    processNode(scene->mRootNode, scene, processNode);
    return meshes;
}