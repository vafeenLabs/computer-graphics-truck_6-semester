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

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Переменные для управления моделью
glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, 0.0f);
float rotationX = 0.0f;
float rotationY = 0.0f;
float rotationZ = 0.0f;

// Определение структуры Mesh
struct Mesh {
    unsigned int VAO, VBO, EBO;
    unsigned int indexCount;
    unsigned int textureID;
};

// Функция загрузки текстуры
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

// Функция обработки ввода с клавиатуры
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Управление вращением модели
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        rotationY += 0.5f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        rotationY -= 0.5f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        rotationX += 0.5f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        rotationX -= 0.5f;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        rotationZ += 0.5f;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        rotationZ -= 0.5f;
}

// Функция загрузки модели
std::vector<Mesh> loadModel(const std::string& path) {
    std::vector<Mesh> meshes;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return meshes;
    }

    // Предполагаем, что текстуры находятся в той же папке, что и модель
    std::string directory = path.substr(0, path.find_last_of('/')) + "/";

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];

        // Получаем данные вершин
        std::vector<float> vertices;
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            // Позиции
            vertices.push_back(mesh->mVertices[j].x);
            vertices.push_back(mesh->mVertices[j].y);
            vertices.push_back(mesh->mVertices[j].z);

            // Нормали
            vertices.push_back(mesh->mNormals[j].x);
            vertices.push_back(mesh->mNormals[j].y);
            vertices.push_back(mesh->mNormals[j].z);

            // Текстурные координаты
            if (mesh->mTextureCoords[0]) {
                vertices.push_back(mesh->mTextureCoords[0][j].x);
                vertices.push_back(mesh->mTextureCoords[0][j].y);
            }
            else {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }

        // Получаем индексы
        std::vector<unsigned int> indices;
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                indices.push_back(face.mIndices[k]);
            }
        }

        // Создаем VAO, VBO и EBO
        Mesh m;
        m.indexCount = indices.size();

        glGenVertexArrays(1, &m.VAO);
        glGenBuffers(1, &m.VBO);
        glGenBuffers(1, &m.EBO);

        glBindVertexArray(m.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Устанавливаем атрибуты вершин
        // Позиции
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Нормали
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // Текстурные координаты
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Загружаем текстуры материала
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // Загружаем диффузную текстуру
            aiString str;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &str) == AI_SUCCESS) {
                std::string texturePath = directory + str.C_Str();
                m.textureID = loadTexture(texturePath.c_str());
            }
            else {
                // Если текстура не найдена, создаем белую текстуру
                unsigned char whiteTexture[] = { 255, 255, 255, 255 };
                glGenTextures(1, &m.textureID);
                glBindTexture(GL_TEXTURE_2D, m.textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whiteTexture);
                glGenerateMipmap(GL_TEXTURE_2D);
            }
        }

        meshes.push_back(m);
    }

    return meshes;
}

// Функция создания плоскости (асфальта)
Mesh createPlane(float width, float height) {
    Mesh plane;
    float vertices[] = {
        // Координаты      // Нормали          // Текстурные координаты
        -width / 2, 0.0f, -height / 2, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
         width / 2, 0.0f, -height / 2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         width / 2, 0.0f,  height / 2, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -width / 2, 0.0f,  height / 2, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    plane.indexCount = 6;

    glGenVertexArrays(1, &plane.VAO);
    glGenBuffers(1, &plane.VBO);
    glGenBuffers(1, &plane.EBO);

    glBindVertexArray(plane.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, plane.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Устанавливаем атрибуты вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return plane;
}

int main() {
    // Инициализация GLFW
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW");
        return -1;
    }

    // Настройка версии OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создание окна
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "VAZ 2121 Model", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Включаем тест глубины
    glEnable(GL_DEPTH_TEST);

    // Создание и компиляция шейдеров
    Shader ourShader("vertex.vs", "fragment.fs");

    // Загрузка модели
    std::vector<Mesh> modelMeshes = loadModel("models/vaz2121.obj");
    if (modelMeshes.empty()) {
        std::cerr << "Failed to load model" << std::endl;
        return -1;
    }

    // Создание плоскости для асфальта
    Mesh asphaltPlane = createPlane(10.0f, 10.0f);
    unsigned int asphaltTexture = loadTexture("models/textures/road_texture.png"); // Путь к текстуре асфальта

    // Настройка камеры
    glm::mat4 view = glm::lookAt(
        glm::vec3(3.0f, 2.0f, 3.0f), // Позиция камеры
        glm::vec3(0.0f, 0.0f, 0.0f), // Точка, на которую смотрит камера
        glm::vec3(0.0f, 1.0f, 0.0f)  // Вектор "вверх"
    );
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    // Установка параметров освещения
    ourShader.use();
    ourShader.setMat4("view", view);
    ourShader.setMat4("projection", projection);
    ourShader.setVec3("lightPos", glm::vec3(2.0f, 2.0f, 2.0f));
    ourShader.setVec3("viewPos", glm::vec3(3.0f, 2.0f, 3.0f));
    ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

    // Основной цикл рендеринга
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // Очистка буферов
        glClearColor(0.2f, 0.5f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // Отрисовка плоскости (асфальта)
        glm::mat4 asphaltModel = glm::mat4(1.0f);
        ourShader.setMat4("model", asphaltModel);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, asphaltTexture);
        ourShader.setInt("texture1", 0);
        glBindVertexArray(asphaltPlane.VAO);
        glDrawElements(GL_TRIANGLES, asphaltPlane.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Матрица модели для автомобиля
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, modelPosition);
        model = glm::scale(model, glm::vec3(0.5f)); // Уменьшаем модель в 2 раза
        model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
        ourShader.setMat4("model", model);

        // Отрисовка модели
        for (const auto& mesh : modelMeshes) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);
            ourShader.setInt("texture1", 0);

            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Освобождение ресурсов
    for (auto& mesh : modelMeshes) {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
        glDeleteTextures(1, &mesh.textureID);
    }
    glDeleteVertexArrays(1, &asphaltPlane.VAO);
    glDeleteBuffers(1, &asphaltPlane.VBO);
    glDeleteBuffers(1, &asphaltPlane.EBO);
    glDeleteTextures(1, &asphaltTexture);

    glfwTerminate();
    return 0;
}
