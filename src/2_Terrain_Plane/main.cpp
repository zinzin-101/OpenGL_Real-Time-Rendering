#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>

#include "HeightMap.h"
#include "VertexData.h"
#include "Utilities.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, float dt);
unsigned int loadTexture(const char *path);

struct TerrainData {
    TerrainData(const VerticesData& verticesData, const GLuint& vao, const GLuint& vbo, const GLuint& ebo, const Shader& terrainShader): 
        verticesData(verticesData), terrainVAO(vao), terrainVBO(vbo), terrainEBO(ebo), terrainShader(terrainShader) {}
    const VerticesData& verticesData;
    const GLuint& terrainVAO;
    const GLuint& terrainVBO;
    const GLuint& terrainEBO;
    const Shader& terrainShader;
};

struct SunData {
    SunData(const GLuint& sunVAO, const GLuint& sunVBO, const GLuint& sunEBO, const Shader& sunShader, glm::vec3& position):
    sunVAO(sunVAO), sunVBO(sunVBO), sunEBO(sunEBO), sunShader(sunShader), position(position) {}
    glm::vec3& position;
    const GLuint& sunVAO;
    const GLuint& sunVBO;
    const GLuint& sunEBO;
    const Shader& sunShader;
};

GLuint seaVAO;
GLuint seaVBO;
GLuint seaEBO;
glm::vec3 seaPosition = glm::vec3();

float sunStrength = 1.0f;

void initTerrain(GLuint& terrainVAO, GLuint& terrainVBO, GLuint& terrainEBO, const VerticesData& verticesData);
void initSun(GLuint& sunVAO, GLuint& sunVBO, GLuint& sunEBO);
void initSea(GLuint& seaVAO, GLuint& seaVBO, GLuint& seaEBO);
void updateObjects(SunData& sunData, float dt);
void update(GLFWwindow*& window, const TerrainData& terrainData, SunData& sunData);
void render(const TerrainData& terrainData, const SunData& sunData);
void drawTerrain(GLuint& terrainVAO, const VerticesData& verticesData);
void drawSun(const GLuint& sunVAO);
void drawSea(const GLuint& seaVAO);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 10.0f, 0.0f));
float moveSpeed = 100.0f;
float speedMultiplier = 2.5f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float lastFrame = 0.0f;

const float PI = 3.14159265358979323846;

void initSphere() {
    initSphereVertices();
    initSphereIndices();
}

void initSphereVertices() {
    std::vector<float> tempVerts;
    for (int i = 0; i <= SphereRings; i++) {
        float phi = PI * (float)i / SphereRings;
        for (int j = 0; j <= SphereSegments; j++) {
            float theta = 2.0f * PI * (float)j / SphereSegments;
            float x = sinf(phi) * cosf(theta);
            float y = cosf(phi);
            float z = sinf(phi) * sinf(theta);

            tempVerts.emplace_back(x);
            tempVerts.emplace_back(y);
            tempVerts.emplace_back(z);
        }
    }

    unsigned int count = tempVerts.size();
    for (int i = 0; i < count; i++) {
        SphereVertices[i] = tempVerts[i];
    }
}

void initSphereIndices() {
    std::vector<unsigned int> tempIndices;
    for (int i = 0; i < SphereRings; i++) {
        for (int j = 0; j < SphereSegments; j++) {
            unsigned int v0 = i * (SphereSegments + 1) + j;
            unsigned int v1 = i * (SphereSegments + 1) + j + 1;
            unsigned int v2 = (i + 1) * (SphereSegments + 1) + j;
            unsigned int v3 = (i + 1) * (SphereSegments + 1) + j + 1;

            tempIndices.emplace_back(v0);
            tempIndices.emplace_back(v2);
            tempIndices.emplace_back(v1);

            tempIndices.emplace_back(v1);
            tempIndices.emplace_back(v2);
            tempIndices.emplace_back(v3);
        }
    }

    unsigned int count = tempIndices.size();
    for (int i = 0; i < count; i++) {
        SphereFaceIndices[i] = tempIndices[i];
    }
}

void initTerrain(GLuint& terrainVAO, GLuint& terrainVBO, GLuint& terrainEBO, const VerticesData& verticesData) {
    // bind VAO
    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO); 

    // generate VBO
    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        verticesData.verticesCount * sizeof(float) * 6,
        verticesData.vertsAndNormals,
        GL_STATIC_DRAW
        );

    // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    // normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // generate EBO
    glGenBuffers(1, &terrainEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        verticesData.indicesCount * sizeof(unsigned int),
        verticesData.indices,
        GL_STATIC_DRAW
    );
}

void initSun(GLuint& sunVAO, GLuint& sunVBO, GLuint& sunEBO) {
    // bind VAO
    glGenVertexArrays(1, &sunVAO);
    glBindVertexArray(sunVAO);

    // generate VBO
    glGenBuffers(1, &sunVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sunVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        SphereVerticesCount * sizeof(float) * 3,
        SphereVertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // generate EBO
    glGenBuffers(1, &sunEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sunEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        SphereFaceIndicesCount * sizeof(unsigned int) * 6,
        SphereFaceIndices,
        GL_STATIC_DRAW
    );
}

void initSea(GLuint& seaVAO, GLuint& seaVBO, GLuint& seaEBO) {
    // bind VAO
    glGenVertexArrays(1, &seaVAO);
    glBindVertexArray(seaVAO);

    // generate VBO
    glGenBuffers(1, &seaVBO);
    glBindBuffer(GL_ARRAY_BUFFER, seaVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(cubeVertices),
        cubeVertices,
        GL_STATIC_DRAW
    );

    // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    // normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //// generate EBO
    //glGenBuffers(1, &seaEBO);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, seaEBO);
    //glBufferData(
    //    GL_ELEMENT_ARRAY_BUFFER,
    //    SquareFaceIndicesCount * sizeof(unsigned int) * 3,
    //    SquareFaceIndices,
    //    GL_STATIC_DRAW
    //);
}

void drawTerrain(const GLuint& terrainVAO, const VerticesData& verticesData) {
    glBindVertexArray(terrainVAO);
    for (unsigned int i = 0; i < verticesData.stripsCount; i++) {
        glDrawElements(
            GL_TRIANGLE_STRIP,
            verticesData.numOfverticesPerStrip,
            GL_UNSIGNED_INT,
            (void*)(sizeof(unsigned int) * verticesData.numOfverticesPerStrip * i)
        );
    }
}

void drawSun(const GLuint& sunVAO) {
    glBindVertexArray(sunVAO);
    glDrawElements(GL_TRIANGLES, SphereFaceIndicesCount * 6, GL_UNSIGNED_INT, 0);
}

void drawSea(const GLuint& seaVAO) {
    glBindVertexArray(seaVAO);
    //glDrawElements(GL_TRIANGLES, SquareFaceIndicesCount * 3, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void render(const TerrainData& terrainData, const SunData& sunData) {
    glClearColor(0.678f, 0.847f, 0.902f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // terrain
    terrainData.terrainShader.use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 2000.0f);
    glm::mat4 view = camera.GetViewMatrix();
    terrainData.terrainShader.setMat4("projection", projection);
    terrainData.terrainShader.setMat4("view", view);

    // lighting from sun
    terrainData.terrainShader.setVec3("color", glm::vec3(1.0f, 0.5f, 0.2f));
    terrainData.terrainShader.setFloat("shininess", 30.0f);
    terrainData.terrainShader.setVec3("viewPos", camera.Position);
    terrainData.terrainShader.setVec3("pointLights[0].position", sunData.position);
    terrainData.terrainShader.setVec3("pointLights[0].ambient", sunStrength * glm::vec3(0.9f) + glm::vec3(0.3f));
    terrainData.terrainShader.setVec3("pointLights[0].diffuse", sunStrength * glm::vec3(0.9f));
    terrainData.terrainShader.setVec3("pointLights[0].specular", sunStrength * glm::vec3(0.5f));
    terrainData.terrainShader.setFloat("pointLights[0].constant", 0.8f);
    terrainData.terrainShader.setFloat("pointLights[0].linear", 0.0000014f);
    terrainData.terrainShader.setFloat("pointLights[0].quadratic", 0.000001f);

    glm::mat4 model = glm::mat4(1.0f);
    float halfWidth = (terrainData.verticesData.stripsCount + 1) / 2.0f;
    model = glm::translate(model, HORIZONTAL_SCALING_FACTOR * glm::vec3(-halfWidth, 0, -halfWidth));
    terrainData.terrainShader.setMat4("model", model);
    drawTerrain(terrainData.terrainVAO, terrainData.verticesData);

    // sea
    terrainData.terrainShader.use();
    terrainData.terrainShader.setMat4("projection", projection);
    terrainData.terrainShader.setMat4("view", view);
    glm::mat4 seaModel = glm::mat4(1.0f);
    seaModel = glm::scale(seaModel, glm::vec3(2000.0f, 1.0f, 2000.0f));
    seaModel = glm::translate(seaModel, seaPosition);
    terrainData.terrainShader.setMat4("model", seaModel);
    terrainData.terrainShader.setVec3("color", glm::vec3(0.0f, 0.0f, 1.0f));
    terrainData.terrainShader.setFloat("shininess", 128.0f);
    drawSea(seaVAO);

    // sun
    sunData.sunShader.use();
    sunData.sunShader.setMat4("projection", projection);
    sunData.sunShader.setMat4("view", view);
    glm::mat4 sunModel = glm::mat4(1.0f);
    sunModel = glm::translate(sunModel, sunData.position);
    sunModel = glm::scale(sunModel, glm::vec3(100.0f, 100.0f, 100.0f));
    sunData.sunShader.setMat4("model", sunModel);
    drawSun(sunData.sunVAO);
}

void updateObjects(SunData& sunData, float dt) {
    static float t = 0.0f;
    t += dt;
    sunData.position.x = 1024.0f * cosf(0.5f * t);
    sunData.position.y = 800.0f * sinf(0.5f * t);
    sunStrength = sunData.position.y / 500.0f;
    if (sunStrength < 0.0f) sunStrength = 0.0f;

    seaPosition.y = 50.0f * sinf(0.125f * t) - 30.0f;
}

void update(GLFWwindow*& window, const TerrainData& terrainData, SunData& sunData) {
    float currentFrame = static_cast<float>(glfwGetTime());
    float dt = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window, dt);
    updateObjects(sunData, dt);
    render(terrainData, sunData);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    
    HeightMap heightMap(1024 + 1);
    //HeightMap heightMap(4 + 1);
    HeightMapData heightMapData = heightMap.getData();
    VerticesData vertsData = getVerticesFromHeightMap(heightMapData.data, heightMapData.width);

    initSphere();

    GLuint terrainVAO;
    GLuint terrainVBO;
    GLuint terrainEBO;
    initTerrain(terrainVAO, terrainVBO, terrainEBO, vertsData);
    Shader terrainShader("TerrainVertexShader.vs", "TerrainFragmentShader.fs");
    terrainShader.use();
    terrainShader.setVec3("color", glm::vec3(1.0f, 0.5f, 0.2f));
    terrainShader.setFloat("shininess", 50.0f);
    TerrainData terrainData = TerrainData(vertsData, terrainVAO, terrainVBO, terrainEBO, terrainShader);

    glm::vec3 sunPosition(0.0f, 5.0f, 0.0f);
    GLuint sunVAO;
    GLuint sunVBO;
    GLuint sunEBO;
    initSun(sunVAO, sunVBO, sunEBO);
    Shader sunShader("LightSphere.vs", "LightSphere.fs");
    sunShader.use();
    sunShader.setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
    SunData sunData = SunData(sunVAO, sunVBO, sunEBO, sunShader, sunPosition);

    initSea(seaVAO, seaVBO, seaEBO);

    while (!glfwWindowShouldClose(window))
    {
        update(window, terrainData, sunData);
    }

    if (vertsData.vertsAndNormals != nullptr) delete vertsData.vertsAndNormals;
    if (vertsData.indices != nullptr) delete vertsData.indices;

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, float dt)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    //    camera.ProcessKeyboard(FORWARD, dt);
    //if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    //    camera.ProcessKeyboard(BACKWARD, dt);
    //if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    //    camera.ProcessKeyboard(LEFT, dt);
    //if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    //    camera.ProcessKeyboard(RIGHT, dt);
    //if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    //    camera.ProcessKeyboard(UP, dt);
    //if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    //    camera.ProcessKeyboard(DOWN, dt);

    glm::vec3 movement = glm::vec3();
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement.z += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement.z -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        movement.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        movement.y -= 1.0f;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        movement *= speedMultiplier;
    movement *= moveSpeed;
    camera.MyProcessKeyboard(movement, dt);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
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

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
