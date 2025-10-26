#include "Game.h"
#include <learnopengl/filesystem.h>
#include "VerticesData.h"
#include <vector>
#include <cmath>

Game::Game():
    shader("vertex.vs", "fragment.fs"),
    outlineShader("collider_outline.vs", "collider_outline.fs"),
    skyboxShader("skybox.vs", "skybox.fs"),
    paddleModel(FileSystem::getPath("resources/objects/paddle/paddle.obj")),
    ballModel(FileSystem::getPath("resources/objects/pingpongball/10539_tennis_ball_L3.obj")),
    tableModel(FileSystem::getPath("resources/objects/tabletennistable/tableTennisTable.obj"))
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
        FileSystem::getPath("resources/objects/playroomskybox/right.jpg"),
        FileSystem::getPath("resources/objects/playroomskybox/left.jpg"),
        FileSystem::getPath("resources/objects/playroomskybox/top.jpg"),
        FileSystem::getPath("resources/objects/playroomskybox/bottom.jpg"),
        FileSystem::getPath("resources/objects/playroomskybox/front.jpg"),
        FileSystem::getPath("resources/objects/playroomskybox/back.jpg")
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

void Game::init() {
    initSkybox();
    initColliderOutline();

    stationaryCamera = Camera();
    followCamera = Camera();
    freeCamera = Camera();
    cameras[CameraType::STATIONARY] = &stationaryCamera;
    cameras[CameraType::FOLLOW] = &followCamera;
    cameras[CameraType::FREE] = &freeCamera;
    currentCameraType = CameraType::STATIONARY;

    glm::mat4 tableToWorld = 
        glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -75.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    modelToWorld[&tableModel] = tableToWorld;

    glm::mat4 paddleToWorld =
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.03f));
    modelToWorld[&paddleModel] = paddleToWorld;

    glm::mat4 ballToWorld = 
        glm::scale(glm::mat4(1.0f), glm::vec3(0.03f));
    modelToWorld[&ballModel] = ballToWorld;

    Object& paddle = getNewObject();
    paddle.position = glm::vec3(-2, 0, 0);
    paddle.model = &paddleModel;
    BoxCollider collider;
    collider.offset = glm::vec3();
    collider.size = glm::vec3(1.0f);
    collider.ownerId = paddle.id;
    colliders.emplace_back(collider);
    playerId = paddle.id;

    Object& table = getNewObject();
    table.model = &tableModel;
    collider.ownerId = table.id;
    colliders.emplace_back(collider);

    Object& ball = getNewObject();
    ball.position = glm::vec3(2, 0, 0);
    ball.model = &ballModel;
    collider.ownerId = ball.id;
    colliders.emplace_back(collider);
}

Object& Game::getNewObject() {
    Object obj;
    obj.id = objects.size();
    obj.position = glm::vec3();
    obj.forward = glm::vec3(0, 0, 1);
    obj.right = glm::vec3(1, 0, 0);
    obj.up = glm::vec3(0, 1, 0);

    objects.emplace_back(obj);

    return objects[obj.id];
}

Object& Game::getNewObjectWithCollider() {
    Object& plane = getNewObject();
    BoxCollider collider;
    collider.offset = glm::vec3();
    collider.size = glm::vec3(1.0f);
    collider.ownerId = plane.id;
    colliders.emplace_back(collider);
    return plane;
}

Object& Game::getObjectFromId(unsigned int id) {
    return objects[id];
}

void Game::update(float dt) {
    int numOfColliders = colliders.size();
    for (int i = 0; i < numOfColliders; i++) {
        for (int j = i + 1; j < numOfColliders; j++) {
            BoxCollider& col1 = colliders[i];
            BoxCollider& col2 = colliders[j];

            if (isColliding(col1, col2)) {
                std::cout << "is colliding" << std::endl;
            }
        }
    }
}

glm::mat4 Game::getProjection() const {
    return glm::perspective(glm::radians(FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
}

void Game::render(float dt) {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // don't forget to enable shader before setting uniforms
    shader.use();

    // view/projection transformations
    glm::mat4 projection = getProjection();
    glm::mat4 view = cameras[currentCameraType]->GetViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    
    for (const Object& obj : objects) {
        glm::mat4 rotMat(
            glm::vec4(obj.right, 0.0f),
            glm::vec4(obj.up, 0.0f),
            glm::vec4(obj.forward, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );

        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), obj.position) *
            rotMat *
            modelToWorld[obj.model];

        shader.setMat4("model", model);
        obj.model->Draw(shader);
    }

    // collider
    for (const BoxCollider& collider : colliders) {
        drawCollider(collider);
    }

    // skybox
    skyboxShader.use();
    glm::mat4 skyboxView = glm::mat4(1.0f);
    glm::mat4 skyboxProjection = glm::mat4(1.0f);
    Camera& camera = *cameras[currentCameraType];
    skyboxView = glm::mat4(glm::mat3(glm::lookAt(camera.Position, camera.Position + camera.Forward, camera.Up)));
    skyboxProjection = getProjection();
    skyboxShader.setMat4("view", skyboxView);
    skyboxShader.setMat4("projection", skyboxProjection);
    drawSkybox();
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

void Game::drawCollider(const BoxCollider& collider) {
    outlineShader.use();

    const Object& plane = getObjectFromId(collider.ownerId);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), collider.offset + plane.position) * glm::scale(glm::mat4(1.0f), collider.size);
    glm::mat4 view = cameras[currentCameraType]->GetViewMatrix();
    glm::mat4 projection = getProjection();
    outlineShader.setMat4("model", model);
    outlineShader.setMat4("view", view);
    outlineShader.setMat4("projection", projection);
    outlineShader.setVec4("lineColor", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    glBindVertexArray(outlineVAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    //glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool Game::isColliding(const BoxCollider& c1, const BoxCollider& c2) {
    const Object& p1 = getObjectFromId(c1.ownerId);
    const Object& p2 = getObjectFromId(c2.ownerId);
    glm::vec3 pos1 = c1.offset + p1.position;
    glm::vec3 pos2 = c2.offset + p2.position;
    glm::vec3 halfSize1 = c1.size / 2.0f;
    glm::vec3 halfSize2 = c2.size / 2.0f;

    // AABB collision detection
    bool xCollided = abs(pos1.x - pos2.x) < (halfSize1.x + halfSize2.x);
    bool yCollided = abs(pos1.y - pos2.y) < (halfSize1.y + halfSize2.y);
    bool zCollided = abs(pos1.z - pos2.z) < (halfSize1.z + halfSize2.z);
    
    bool collided = xCollided && yCollided && zCollided;

    return collided;
}

void Game::processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    cameras[currentCameraType]->ProcessMouseMovement(xoffset, yoffset);
}

void Game::initKeyDebounce() {
    keyDebounce[GLFW_KEY_V] = false;
}

void Game::processKeyboard(GLFWwindow* window, float dt) {
    glm::vec3 movement = glm::vec3();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += glm::vec3(0, 0, 5);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement += glm::vec3(0, 0, -5);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement += glm::vec3(-5, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += glm::vec3(5, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        movement += glm::vec3(0, 5, 0);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        movement += glm::vec3(0, -5, 0);

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !keyDebounce.at(GLFW_KEY_V)) {
        keyDebounce[GLFW_KEY_V] = true;
        currentCameraType = (CameraType)(((int)currentCameraType + 1) % NUM_CAM_TYPES);
    }
    else if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) keyDebounce[GLFW_KEY_V] = false;


    cameras[currentCameraType]->ProcessKeyboard(movement, dt);
}