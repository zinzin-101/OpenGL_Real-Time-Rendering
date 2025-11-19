#include "Game.h"
#include "Random.h"
#include "VerticesData.h"
#include <learnopengl/filesystem.h>
#include <vector>
#include <cmath>
#include <ostream>

static ostream& operator<<(ostream& out, const glm::vec3& v);

//bool RenderComparator::operator()(const RenderingObject& obj1, const RenderingObject& obj2) {
//    return obj1.distanceFromCamera > obj2.distanceFromCamera;
//}

Game::Game() :
    wavesShader("waves.vs", "waves.fs"),
    outlineShader("collider_outline.vs", "collider_outline.fs"),
    skyboxShader("skybox.vs", "skybox.fs")
{
    init();
}

unsigned int Game::getCubeMapTexture(std::string cubeMapPath[]) {
    unsigned int cubeMapTex;
    glGenTextures(1, &cubeMapTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // prevent seam
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    for (unsigned int i = 0; i < 6; i++)
    {
        int width, height, nrChannels;
        unsigned char* data = stbi_load(cubeMapPath[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D
            (
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                GL_RGB,
                width,
                height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load texture: " << cubeMapPath[i] << std::endl;
            stbi_image_free(data);
        }
    }

    return cubeMapTex;
}

void Game::initSkybox() {
    std::string cubeMapFaces[6] =
    {
        FileSystem::getPath("resources/objects/skybox/right.jpg"),
        FileSystem::getPath("resources/objects/skybox/left.jpg"),
        FileSystem::getPath("resources/objects/skybox/top.jpg"),
        FileSystem::getPath("resources/objects/skybox/bottom.jpg"),
        FileSystem::getPath("resources/objects/skybox/front.jpg"),
        FileSystem::getPath("resources/objects/skybox/back.jpg")
    };

    cubeMapTexture = getCubeMapTexture(cubeMapFaces);

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SKYBOX_VERTICES), &SKYBOX_VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(SKYBOX_INDICES), &SKYBOX_INDICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Game::initColliderOutline() {
    glGenVertexArrays(1, &outlineVAO);
    glGenBuffers(1, &outlineVBO);
    glGenBuffers(1, &outlineEBO);

    glBindVertexArray(outlineVAO);

    glBindBuffer(GL_ARRAY_BUFFER, outlineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outlineEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Game::initWaves() {
    // create flat plane vertex and indices
    std::vector<float> verts;
    float offset = (float)WAVES_VERTS_WIDTH / 2.0f;
    for (int x = 0; x < WAVES_VERTS_WIDTH; x++) {
        for (float z = 0; z < WAVES_VERTS_WIDTH; z++) {
            verts.emplace_back(((float)x - offset) * WAVES_VERTS_SCALE);
            verts.emplace_back(0.0f);
            verts.emplace_back(((float)z - offset) * WAVES_VERTS_SCALE);
        }
    }

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < WAVES_VERTS_WIDTH - 1; i++) {
        for (unsigned int j = 0; j < WAVES_VERTS_WIDTH; j++) {
            for (unsigned int k = 0; k < 2; k++) {
                indices.emplace_back(j + WAVES_VERTS_WIDTH * (i + k));
            }
        }
    }

    wavesStripCount = WAVES_VERTS_WIDTH - 1;
    wavesVertsPerStrip = WAVES_VERTS_WIDTH * 2;

    // bind VAO
    glGenVertexArrays(1, &wavesVAO);
    glBindVertexArray(wavesVAO);

    // generate VBO
    glGenBuffers(1, &wavesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, wavesVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        verts.size() * sizeof(float),
        verts.data(),
        GL_STATIC_DRAW
    );

    // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    // normals
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    //glEnableVertexAttribArray(1);

    // generate EBO
    glGenBuffers(1, &wavesEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wavesEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    wavesPhase = 0.0f;
}

void Game::drawWaves() {
    glBindVertexArray(wavesVAO);
    for (unsigned int i = 0; i < wavesStripCount; i++) {
        glDrawElements(
            GL_TRIANGLE_STRIP,
            wavesVertsPerStrip,
            GL_UNSIGNED_INT,
            (void*)(sizeof(unsigned int) * wavesVertsPerStrip * i)
        );
    }
}

void Game::init() {
    Random::init();

    //GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    //const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    //int refreshRate = mode->refreshRate;

    initSkybox();
    initColliderOutline();
    
    initWaves();
}

void Game::setup() {

}

void Game::update(float dt) {
    std::cout << "cam pos: " << camera.Position << std::endl;
    this->dt = dt;
    wavesPhase += WAVES_SPEED * dt;
}

glm::mat4 Game::getProjection() const {
    return glm::perspective(glm::radians(FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
}

void Game::render(float dt) {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // skybox
    skyboxShader.use();
    glm::mat4 skyboxView = glm::mat4(1.0f);
    glm::mat4 skyboxProjection = glm::mat4(1.0f);
    skyboxView = glm::mat4((glm::mat3)camera.GetViewMatrix());
    skyboxProjection = getProjection();
    skyboxShader.setMat4("view", skyboxView);
    skyboxShader.setMat4("projection", skyboxProjection);
    drawSkybox();

    wavesShader.use();
    // view/projection transformations
    glm::mat4 projection = getProjection();
    glm::mat4 view = camera.GetViewMatrix();
    wavesShader.setMat4("projection", projection);
    wavesShader.setMat4("view", view);
    wavesShader.setMat4("model", glm::mat4(1.0f));
    wavesShader.setVec3("viewPos", camera.Position);
    wavesShader.setVec3("color", glm::vec3(0.498f, 0.804f, 1.0f));
    wavesShader.setBool("useLighting", true);
    wavesShader.setFloat("phase", wavesPhase);
    wavesShader.setFloat("amplitude", WAVES_AMPLITUDE);
    wavesShader.setFloat("frequency", WAVES_FREQUENCY);

    glm::vec3 lightPos0(0.0f, 50.0f, 0.0f);
    wavesShader.setVec3("dirLight.direction", glm::vec3(0, -1, 0));
    wavesShader.setVec3("dirLight.ambient", glm::vec3(0.5f));
    wavesShader.setVec3("dirLight.diffuse", glm::vec3(0.01f));
    wavesShader.setVec3("dirLight.specular", glm::vec3(0.001f));
    //wavesShader.setVec3("pointLights[0].position", lightPos0);
    //wavesShader.setVec3("pointLights[0].ambient", glm::vec3(1.0f));
    //wavesShader.setVec3("pointLights[0].diffuse", glm::vec3(0.6f));
    //wavesShader.setVec3("pointLights[0].specular", glm::vec3(0.01f));
    //wavesShader.setFloat("pointLights[0].constant", 0.95f);
    //wavesShader.setFloat("pointLights[0].linear", 0.0009f);
    //wavesShader.setFloat("pointLights[0].quadratic", 0.00005f);
    //wavesShader.setFloat("shininess", 128.0f);

    drawWaves();
}

void Game::drawSkybox() {
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void Game::processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void Game::processMouseScroll(float yoffset) {

}

void Game::processMouseButton(int button, int action) {
    //if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) isMovingPaddle = true;
    //else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) isMovingPaddle = false;

    //if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) isAdjustingLook = true;
    //else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) isAdjustingLook = false;
}

bool Game::handleKeyDown(GLFWwindow* window, unsigned int key) {
    if (keyDown.count(key) == 0) { // first press
        keyDown[key] = true;
        return true;
    }

    if (glfwGetKey(window, key) == GLFW_PRESS && !keyDown.at(key)) {
        keyDown[key] = true;
        return true;
    }
    
    if (glfwGetKey(window, key) == GLFW_RELEASE) keyDown[key] = false;
    return false;
}

void Game::processKeyboard(GLFWwindow* window, float dt) {
    glm::vec3 movement = glm::vec3();
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += glm::vec3(0, 0, 1);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement += glm::vec3(0, 0, -1);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement += glm::vec3(-1, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += glm::vec3(1, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        movement += glm::vec3(0, 1, 0);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        movement += glm::vec3(0, -1, 0);

    movement *= glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? FREE_CAM_FAST_MOVE_SPEED : FREE_CAM_MOVE_SPEED;
    camera.ProcessKeyboard(movement, dt);
}

ostream& operator<<(ostream& out, const glm::vec3& v) {
    out << v.x << " " << v.y << " " << v.z;
    return out;
}