#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

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
    TerrainData(VerticesData& verticesData, GLuint& vao, GLuint& vbo, GLuint& ebo, Shader& terrainShader): 
        verticesData(verticesData), terrainVAO(vao), terrainVBO(vbo), terrainEBO(ebo), terrainShader(terrainShader) {}
    VerticesData& verticesData;
    GLuint& terrainVAO;
    GLuint& terrainVBO;
    GLuint& terrainEBO;
    Shader& terrainShader;
};

struct SunData {
    SunData(GLuint& sunVAO, GLuint& sunVBO, GLuint& sunEBO, Shader& sunShader, glm::vec3& position):
    sunVAO(sunVAO), sunVBO(sunVBO), sunEBO(sunEBO), sunShader(sunShader), position(position) {}
    glm::vec3& position;
    GLuint& sunVAO;
    GLuint& sunVBO;
    GLuint& sunEBO;
    Shader& sunShader;
};

float maxTerrainHeight;
float minTerrainHeight;

float maxSeaHeight;
float minSeaHeight;

GLuint seaVAO;
GLuint seaVBO;
GLuint seaEBO;
glm::vec3 seaPosition = glm::vec3();
VerticesData* seaVertsData;


float sunStrength = 1.0f;

Model* plane;
Shader* planeShader;
glm::vec3 planePosition = glm::vec3(0.0f, 75.0f, 0.0f);
glm::vec3 planeForward = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 planeUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 planeRight = glm::vec3(1.0f, 0.0f, 0.0f);
float defaultPlaneSpeed = 100.0f;
float planeSpeed = defaultPlaneSpeed;
float maxPlaneSpeed = 300.0f;
float minPlaneSpeed = 30.0f;
float planeAccelerationRate = 50.0f;
float pitchRate = 150.0f;
float yawRate = 50.0f;
float rollRate = 200.0f;
float camDistanceFromPlane = 50.0f;
float maxFov = 90.0f;
float minFov = 60.0f;
void yawPlane(float deg);
void pitchPlane(float deg);
void rollPlane(float deg);
void updateCamAfterPlane();

void initTerrain(GLuint& terrainVAO, GLuint& terrainVBO, GLuint& terrainEBO, VerticesData& verticesData);
void initSun(GLuint& sunVAO, GLuint& sunVBO, GLuint& sunEBO);
void initSea(GLuint& seaVAO, GLuint& seaVBO, GLuint& seaEBO);
void updateObjects(SunData& sunData, float dt);
void update(GLFWwindow*& window, TerrainData& terrainData, SunData& sunData);
void render(TerrainData& terrainData, SunData& sunData);
void drawTerrain(GLuint& terrainVAO, VerticesData& verticesData);
void drawSun(GLuint& sunVAO);
void drawSea(GLuint& seaVAO);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 50.0f, 0.0f));
float moveSpeed = 100.0f;
float speedMultiplier = 2.5f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float lastFrame = 0.0f;

const float PI = 3.14159265358979323846;

void yawPlane(float deg) {
    glm::vec3 axis = glm::normalize(planeUp);
    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::vec4 forward = rotMat * glm::vec4(planeForward, 1.0f);
    glm::vec4 up = rotMat * glm::vec4(planeUp, 1.0f);
    glm::vec4 right = rotMat * glm::vec4(planeRight, 1.0f);

    planeForward = forward;
    planeUp = up;
    planeRight = right;

    updateCamAfterPlane();
}

void pitchPlane(float deg){
    glm::vec3 axis = glm::normalize(planeRight);
    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::vec4 forward = rotMat * glm::vec4(planeForward, 1.0f);
    glm::vec4 up = rotMat * glm::vec4(planeUp, 1.0f);
    glm::vec4 right = rotMat * glm::vec4(planeRight, 1.0f);

    planeForward = forward;
    planeUp = up;
    planeRight = right;

    updateCamAfterPlane();
}

void rollPlane(float deg){
    glm::vec3 axis = glm::normalize(planeForward);
    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::vec4 forward = rotMat * glm::vec4(planeForward, 1.0f);
    glm::vec4 up = rotMat * glm::vec4(planeUp, 1.0f);
    glm::vec4 right = rotMat * glm::vec4(planeRight, 1.0f);

    planeForward = forward;
    planeUp = up;
    planeRight = right;

    updateCamAfterPlane();
}

void updateCamAfterPlane() {
    camera.Front = planeForward;
    camera.Right = planeRight;
    camera.WorldUp = planeUp;
}

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

void initTerrain(GLuint& terrainVAO, GLuint& terrainVBO, GLuint& terrainEBO, VerticesData& verticesData) {
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
    glGenBuffers(1, &seaEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, seaEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        SquareFaceIndicesCount * sizeof(unsigned int) * 3,
        SquareFaceIndices,
        GL_STATIC_DRAW
    );
}

void drawTerrain(GLuint& terrainVAO, VerticesData& verticesData) {
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

void drawSun(GLuint& sunVAO) {
    glBindVertexArray(sunVAO);
    glDrawElements(GL_TRIANGLES, SphereFaceIndicesCount * 6, GL_UNSIGNED_INT, 0);
}

void drawSea(GLuint& seaVAO) {
    glBindVertexArray(seaVAO);
    //glDrawElements(GL_TRIANGLES, SquareFaceIndicesCount * 3, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void render(TerrainData& terrainData, SunData& sunData) {
    glClearColor(0.678f, 0.847f, 0.902f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // terrain
    terrainData.terrainShader.use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
    glm::mat4 view = camera.GetViewMatrix();
    terrainData.terrainShader.setMat4("projection", projection);
    terrainData.terrainShader.setMat4("view", view);
    terrainData.terrainShader.setBool("lerpColor", true);
    terrainData.terrainShader.setVec3("startColor", glm::vec3(1.0f, 0.5f, 0.2f));
    terrainData.terrainShader.setVec3("endColor", glm::vec3(0.588f, 0.294f, 0.0f));
    terrainData.terrainShader.setFloat("maxHeight", maxTerrainHeight + HEIGHT_SCALING_FACTOR);
    terrainData.terrainShader.setFloat("minHeight", minTerrainHeight + HEIGHT_SCALING_FACTOR);

    // lighting from sun
    terrainData.terrainShader.setVec3("color", glm::vec3(1.0f, 0.5f, 0.2f));
    terrainData.terrainShader.setFloat("shininess", 30.0f);
    terrainData.terrainShader.setVec3("viewPos", camera.Position);
    terrainData.terrainShader.setVec3("pointLights[0].position", sunData.position);
    terrainData.terrainShader.setVec3("pointLights[0].ambient", sunStrength * glm::vec3(0.2f) + glm::vec3(0.2f));
    terrainData.terrainShader.setVec3("pointLights[0].diffuse", sunStrength * glm::vec3(0.2f));
    terrainData.terrainShader.setVec3("pointLights[0].specular", sunStrength * glm::vec3(0.1f));
    terrainData.terrainShader.setFloat("pointLights[0].constant", 0.4f);
    terrainData.terrainShader.setFloat("pointLights[0].linear", 0.0000014f);
    terrainData.terrainShader.setFloat("pointLights[0].quadratic", 0.0000001f);
    // lighting from camera plane
    // point light
    terrainData.terrainShader.setVec3("viewPos", planePosition);
    terrainData.terrainShader.setVec3("pointLights[1].position", planePosition);
    terrainData.terrainShader.setVec3("pointLights[1].ambient", glm::vec3(0.7f));
    terrainData.terrainShader.setVec3("pointLights[1].diffuse", glm::vec3(0.7f));
    terrainData.terrainShader.setVec3("pointLights[1].specular", glm::vec3(0.2f));
    terrainData.terrainShader.setFloat("pointLights[1].constant", 0.6f);
    terrainData.terrainShader.setFloat("pointLights[1].linear", 0.0014f);
    terrainData.terrainShader.setFloat("pointLights[1].quadratic", 0.0001f);
    // spotlight
    terrainData.terrainShader.setVec3("spotLight.position", planePosition + planeForward * 2.0f);
    terrainData.terrainShader.setVec3("spotLight.direction", -planeForward);
    terrainData.terrainShader.setVec3("spotLight.ambient", 50.0f * glm::vec3(0.0f, 0.0f, 0.0f));
    terrainData.terrainShader.setVec3("spotLight.diffuse", 50.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    terrainData.terrainShader.setVec3("spotLight.specular", 50.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    terrainData.terrainShader.setFloat("spotLight.constant", 1.0f);
    terrainData.terrainShader.setFloat("spotLight.linear", 0.09f);
    terrainData.terrainShader.setFloat("spotLight.quadratic", 0.032f);
    terrainData.terrainShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(30.5f)));
    terrainData.terrainShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(45.0f)));

    glm::mat4 model = glm::mat4(1.0f);
    float halfWidth = (terrainData.verticesData.stripsCount + 1) / 2.0f;
    model = glm::translate(model, HORIZONTAL_SCALING_FACTOR * glm::vec3(-halfWidth, 0.0f, -halfWidth));
    model = glm::translate(model, HEIGHT_SCALING_FACTOR * glm::vec3(0.0f, 1.0f, 0.0f));
    terrainData.terrainShader.setMat4("model", model);
    drawTerrain(terrainData.terrainVAO, terrainData.verticesData);

    // sea
    terrainData.terrainShader.use();
    terrainData.terrainShader.setBool("lerpColor", true);
    terrainData.terrainShader.setVec3("startColor", glm::vec3(0.0f, 0.0f, 1.0f));
    terrainData.terrainShader.setVec3("endColor", glm::vec3(1.0f, 1.0f, 1.0f));
    terrainData.terrainShader.setFloat("maxHeight", maxSeaHeight + 1);
    terrainData.terrainShader.setFloat("minHeight", minSeaHeight + 1);
    terrainData.terrainShader.setMat4("projection", projection);
    terrainData.terrainShader.setMat4("view", view);
    glm::mat4 seaModel = glm::mat4(1.0f);
    //seaModel = glm::scale(seaModel, glm::vec3(5000.0f, 1.0f, 5000.0f));
    float seaHalfWidth = (seaVertsData->stripsCount + 1) / 2.0f;
    seaModel = glm::translate(model, HORIZONTAL_SCALING_FACTOR * glm::vec3(-seaHalfWidth, 0.0f, -seaHalfWidth));
    seaModel = glm::translate(model, 1.0f * glm::vec3(0.0f, 1.0f, 0.0f));
    seaModel = glm::translate(seaModel, seaPosition);
    terrainData.terrainShader.setMat4("model", seaModel);
    terrainData.terrainShader.setFloat("shininess", 128.0f);
    //drawSea(seaVAO);
    drawTerrain(seaVAO, *seaVertsData);

    // sun
    sunData.sunShader.use();
    sunData.sunShader.setMat4("projection", projection);
    sunData.sunShader.setMat4("view", view);
    glm::mat4 sunModel = glm::mat4(1.0f);
    sunModel = glm::translate(sunModel, sunData.position);
    sunModel = glm::scale(sunModel, glm::vec3(100.0f, 100.0f, 100.0f));
    sunData.sunShader.setMat4("model", sunModel);
    drawSun(sunData.sunVAO);

    // plane
    planeShader->use();
    // lighting from sun
    planeShader->setVec3("color", glm::vec3(1.0f, 1.0f, 1.0f));
    planeShader->setFloat("shininess", 30.0f);
    planeShader->setVec3("viewPos", camera.Position);
    planeShader->setVec3("pointLight.position", sunData.position);
    planeShader->setVec3("pointLight.ambient", sunStrength * glm::vec3(0.2f) + glm::vec3(0.2f));
    planeShader->setVec3("pointLight.diffuse", sunStrength * glm::vec3(0.2f));
    planeShader->setVec3("pointLight.specular", sunStrength * glm::vec3(0.1f));
    planeShader->setFloat("pointLight.constant", 0.4f);
    planeShader->setFloat("pointLight.linear", 0.0000014f);
    planeShader->setFloat("pointLight.quadratic", 0.0000001f);

    planeShader->setMat4("projection", projection);
    planeShader->setMat4("view", view);
    glm::mat4 planeModel = glm::mat4(1.0f);
    glm::mat4 rotMat(
        glm::vec4(planeRight, 0.0f),
        glm::vec4(planeUp, 0.0f),
        glm::vec4(planeForward, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) 
    );
    planeModel = glm::translate(glm::mat4(1.0f), planePosition) * rotMat * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    planeModel = glm::scale(planeModel, glm::vec3(0.25f));
    planeShader->setMat4("model", planeModel);
    plane->Draw(*planeShader);
}

void updateObjects(SunData& sunData, float dt) {
    static float t = 0.0f;
    t += dt;
    sunData.position.x = 2048.0f * cosf(0.15f * t);
    sunData.position.y = 2500.0f * sinf(0.15f * t);
    sunStrength = sunData.position.y / 800;
    if (sunStrength < 0.0f) sunStrength = 0.0f;

    seaPosition.y = 200.0f * sinf(0.125f * t) - 500.0f;

    planePosition += glm::normalize(planeForward) * planeSpeed * dt;
    glm::vec3 camPos = planePosition - (planeForward * camDistanceFromPlane);
    glm::vec3 camLook = planePosition - camPos;
    camPos += planeUp * 10.0f;
    camera.Position = camPos;
    camera.SetFrontVector(camLook);
}

void update(GLFWwindow*& window, TerrainData& terrainData, SunData& sunData) {
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "TerrainPlane", NULL, NULL);
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
    
    HeightMap heightMap(2048 + 1);
    //HeightMap heightMap(4 + 1);
    HeightMapData heightMapData = heightMap.getData();
    VerticesData vertsData = getVerticesFromHeightMap(heightMapData.data, heightMapData.width);

    HeightMap seaMap(2048 + 1);
    HeightMapData seaMapData = seaMap.getData();
    VerticesData seaData = getVerticesFromHeightMap(seaMapData.data, seaMapData.width, HORIZONTAL_SCALING_FACTOR, 1.0f);
    seaVertsData = &seaData;

    minTerrainHeight = vertsData.vertsAndNormals[1];
    maxTerrainHeight = vertsData.vertsAndNormals[1];
    for (int i = 1; i < vertsData.verticesCount; i++) {
        float h = vertsData.vertsAndNormals[i * 6 + 2];
        if (h < minTerrainHeight) {
            minTerrainHeight = h;
        }
        
        if (h > maxTerrainHeight) {
            maxTerrainHeight = h;
        }
    }

    minSeaHeight = seaVertsData->vertsAndNormals[1];
    maxSeaHeight = seaVertsData->vertsAndNormals[1];
    for (int i = 1; i < seaVertsData->verticesCount; i++) {
        float h = seaVertsData->vertsAndNormals[i * 6 + 2];
        if (h < minSeaHeight) {
            minSeaHeight = h;
        }

        if (h > maxSeaHeight) {
            maxSeaHeight = h;
        }
    }

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

    initTerrain(seaVAO, seaVBO, seaEBO, *seaVertsData);

    Model planeModel(FileSystem::getPath("resources/objects/fighterjet/fighterjet.obj"));
    Shader planeshader("PlaneVertexShader.vs", "PlaneFragmentShader.fs");
    planeShader = &planeshader;
    plane = &planeModel;

    planePosition.y = maxTerrainHeight * 0.5f;

    while (!glfwWindowShouldClose(window))
    {
        update(window, terrainData, sunData);
    }

    if (vertsData.vertsAndNormals != nullptr) delete vertsData.vertsAndNormals;
    if (vertsData.indices != nullptr) delete vertsData.indices;

    if (seaVertsData->vertsAndNormals != nullptr) delete seaVertsData->vertsAndNormals;
    if (seaVertsData->indices != nullptr) delete seaVertsData->indices;

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, float dt)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //glm::vec3 movement = glm::vec3();
    //if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    //    movement.z += 1.0f;
    //if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    //    movement.z -= 1.0f;
    //if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    //    movement.x -= 1.0f;
    //if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    //    movement.x += 1.0f;
    //if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    //    movement.y += 1.0f;
    //if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    //    movement.y -= 1.0f;

    //if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    //    movement *= speedMultiplier;
    //movement *= moveSpeed;
    //camera.MyProcessKeyboard(movement, dt);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pitchPlane(pitchRate * dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pitchPlane(-pitchRate * dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        rollPlane(rollRate * dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        rollPlane(-rollRate * dt);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        yawPlane(-yawRate * dt);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        yawPlane(yawRate * dt);

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        planeSpeed += planeAccelerationRate * dt;
    else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        planeSpeed -= planeAccelerationRate * dt;
    else {
        if (planeSpeed + 5.0f > defaultPlaneSpeed) {
            planeSpeed -= planeAccelerationRate * 0.5f * dt;
        }
        else if (planeSpeed - 5.0f < defaultPlaneSpeed) {
            planeSpeed += planeAccelerationRate * 0.5f * dt;
        }
    }

    planeSpeed = glm::clamp(planeSpeed, minPlaneSpeed, maxPlaneSpeed);
    float t = (planeSpeed - minPlaneSpeed) / (maxPlaneSpeed - minPlaneSpeed);
    float fov = (1 - t) * minFov + t * maxFov;
    camera.Zoom = fov;
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

    //camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    //camera.ProcessMouseScroll(static_cast<float>(yoffset));
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
