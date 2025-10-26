#include "Game.h"
#include <learnopengl/filesystem.h>
#include "VerticesData.h"
#include <vector>
#include <cmath>

Game::Game() :
    shader("vertex.vs", "fragment.fs"),
    outlineShader("collider_outline.vs", "collider_outline.fs"),
    skyboxShader("skybox.vs", "skybox.fs"),
    paddleModel(FileSystem::getPath("resources/objects/paddle/paddle.obj")),
    ballModel(FileSystem::getPath("resources/objects/pingpongball/10539_tennis_ball_L3.obj")),
    tableModel(FileSystem::getPath("resources/objects/tabletennistable/tableTennisTable.obj")),
    floorModel(tableModel)
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
    cameraFollowDistance = (MAX_FOLLOW_CAM_DISTANCE + MIN_FOLLOW_CAM_DISTANCE) / 2.0f;

    toggleGravity = false;

    glm::mat4 tableToWorld = 
        glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -75.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    modelToWorld[&tableModel] = tableToWorld;

    glm::mat4 paddleToWorld =
        glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
    modelToWorld[&paddleModel] = paddleToWorld;

    glm::mat4 ballToWorld = 
        glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.5f));
    modelToWorld[&ballModel] = ballToWorld;

    glm::mat4 floorToWorld =
        glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -75.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    modelToWorld[&floorModel] = floorToWorld;

    Object& paddle = getNewObject();
    paddle.position = glm::vec3(-2, 5, 0);
    paddle.model = &paddleModel;
    paddle.name = "PlayerPaddle";
    BoxCollider collider;
    collider.offset = glm::vec3(0.0f, 1.7f, 0.895f);
    collider.size = glm::vec3(3.1f, 3.1f, 2.0f);
    collider.ownerId = paddle.id;
    colliders.emplace_back(collider);
    playerId = paddle.id;

    Object& table = getNewObject();
    table.model = &tableModel;
    table.name = "Table";
    collider = BoxCollider();
    collider.offset = glm::vec3(0.0f, -1.9f, 0.0f);
    collider.size = glm::vec3(16.0f, 5.0f, 27.5f);
    collider.ownerId = table.id;
    colliders.emplace_back(collider);

    collider = BoxCollider();
    collider.size = glm::vec3(16.0f, 1.3f, 0.3f);
    collider.offset = glm::vec3(0.0f, 1.3f, 0.0f);
    collider.ownerId = table.id;
    colliders.emplace_back(collider);

    Object& ball = getNewObject();
    ball.position = glm::vec3(-2, 10, 5);
    ball.model = &ballModel;
    ball.name = "Ball";
    collider = BoxCollider();
    collider.ownerId = ball.id;
    ballId = ball.id;
    colliders.emplace_back(collider);
    Physics phys;
    phys.ownerId = ball.id;
    phys.lastPosition = ball.position;
    physics.emplace_back(phys);

    Object& leftWall = getNewObject();
    leftWall.name = "LeftWall";
    leftWall.model = &tableModel;
    rotateObject(leftWall, leftWall.forward, -90.0f);
    leftWall.position = glm::vec3(-11.0f, 8.0f, 0.0f);
    collider = BoxCollider();
    collider.ownerId = leftWall.id;
    collider.offset = glm::vec3(0.0f, -1.0f, 0.0f);
    collider.size = glm::vec3(2.0f, 28.0f, 30.0f);
    colliders.emplace_back(collider);

    Object& rightWall = getNewObject();
    rightWall.name = "RightWall";
    rightWall.model = &tableModel;
    rotateObject(rightWall, rightWall.forward, 90.0f);
    rightWall.position = glm::vec3(11.0f, 8.0f, 0.0f);
    collider = BoxCollider();
    collider.ownerId = rightWall.id;
    collider.offset = glm::vec3(0.0f, -1.0f, 0.0f);
    collider.size = glm::vec3(2.0f, 28.0f, 30.0f);
    colliders.emplace_back(collider);

    Object& floor = getNewObject();
    floor.name = "Floor";
    floor.model = &floorModel;
    floor.position = glm::vec3(0.0f, -8.5f, 0.0f);
    collider = BoxCollider();
    collider.ownerId = floor.id;
    collider.size = glm::vec3(50.0f, 2.0f, 50.0f);
    colliders.emplace_back(collider);
}

Object& Game::getNewObject() {
    Object obj;
    obj.id = objects.size();
    obj.model = nullptr;
    obj.name = "Object";
    obj.position = glm::vec3();
    obj.forward = glm::vec3(0, 0, 1);
    obj.right = glm::vec3(1, 0, 0);
    obj.up = glm::vec3(0, 1, 0);

    objects.emplace_back(obj);

    return objects[obj.id];
}

Object& Game::getNewObjectWithCollider() {
    Object& obj = getNewObject();
    BoxCollider collider;
    collider.offset = glm::vec3();
    collider.size = glm::vec3(1.0f);
    collider.ownerId = obj.id;
    colliders.emplace_back(collider);
    return obj;
}

Object& Game::getObjectFromId(int id) {
    return objects[id];
}

std::vector<BoxCollider*> Game::getCollidersFromId(int id) {
    std::vector<BoxCollider*> cols;
    for (BoxCollider& col : colliders) {
        if (col.ownerId == id) cols.emplace_back(&col);
    }
    return cols;
}

std::vector<Physics*> Game::getPhysicsFromId(int id) {
    std::vector<Physics*> phys;
    for (Physics& p : physics) {
        if (p.ownerId == id) phys.emplace_back(&p);
    }
    return phys;
}

void Game::computeCollision() {
    int numOfColliders = colliders.size();
    for (int i = 0; i < numOfColliders; i++) {
        for (int j = i + 1; j < numOfColliders; j++) {
            BoxCollider& col1 = colliders[i];
            BoxCollider& col2 = colliders[j];

            if (col1.ownerId == col2.ownerId) continue;

            if (isColliding(col1, col2)) {
                std::cout << getObjectFromId(col1.ownerId).name << " and " << getObjectFromId(col2.ownerId).name << " is colliding" << std::endl;
                handleCollision(col1, col2);
            }
        }
    }
}

void Game::handleCollision(BoxCollider& col1, BoxCollider& col2) {
    Object& obj1 = getObjectFromId(col1.ownerId);
    Object& obj2 = getObjectFromId(col2.ownerId);

    if (obj1.model == &ballModel && (obj2.model == &tableModel))
        handleBallBounce(obj1, obj2);
    if (obj1.model == &tableModel && obj2.model == &ballModel)
        handleBallBounce(obj2, obj1);
}

void Game::handleBallBounce(Object& ball, Object& wall) {
    Physics& ballPhysics = *(getPhysicsFromId(ball.id))[0];
    BoxCollider& ballCol = *(getCollidersFromId(ball.id))[0];
    BoxCollider& wallCol = *(getCollidersFromId(wall.id))[0];
    glm::vec3 wallPos = wall.position;

    if (wallPos.x > 0.0f) { // right wall
        ballPhysics.lastPosition.x = ball.position.x;
        ball.position.x = wall.position.x + wallCol.offset.x + wallCol.size.x * 0.5f + ballCol.offset.x + ballCol.size.x * 0.5f;
    }
    else if (wallPos.x < 0.0f) { // left wall
        ballPhysics.lastPosition.x = ball.position.x;
        ball.position.x = wall.position.x + wallCol.offset.x + wallCol.size.x * 0.5f + ballCol.offset.x + ballCol.size.x * 0.5f;
    }
    else if (wallPos.y <= 0.0f) { // table
        ballPhysics.lastPosition.y = ball.position.y;
        ball.position.y = wall.position.y + wallCol.offset.y + wallCol.size.y * 0.5f + ballCol.offset.y + ballCol.size.y * 0.5f;
    }
}

void Game::accelerate(Physics& phys, glm::vec3 a) {
    phys.acceleration += a;
}

void Game::setVelocity(Physics& phys, glm::vec3 vel, float dt){
    phys.lastPosition = getObjectFromId(phys.ownerId).position - (vel * dt);
}

void Game::addVelocity(Physics& phys, glm::vec3 vel, float dt){
    phys.lastPosition -= vel * dt;
}

glm::vec3 Game::getVelocity(Physics& phys, float dt) {
    Object& obj = getObjectFromId(phys.ownerId);
    return (obj.position - phys.lastPosition) / dt;
}

void Game::computePhysics(float dt) {
    for (Physics& p : physics) {
        Object& obj = getObjectFromId(p.ownerId);

        if (toggleGravity) accelerate(p, DEFAULT_GRAVITY);

        glm::vec3 displacement = obj.position - p.lastPosition;
        p.lastPosition = obj.position;
        obj.position += displacement + p.acceleration * dt * dt;
        p.acceleration = glm::vec3(0.0f);
    }
}

void Game::rotateObject(Object& obj, glm::vec3 axis, float deg) {
    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::vec4 forward = rotMat * glm::vec4(obj.forward, 1.0f);
    glm::vec4 up = rotMat * glm::vec4(obj.up, 1.0f);
    glm::vec4 right = rotMat * glm::vec4(obj.right, 1.0f);

    obj.forward = forward;
    obj.up = up;
    obj.right = right;
}

void Game::update(float dt) {
    computePhysics(dt);
    computeCollision();

    Object& ball = getObjectFromId(ballId);
    Physics& ballPhys = *getPhysicsFromId(ballId)[0];
    glm::vec3 ballvel = getVelocity(ballPhys, dt);
    std::cout << "ball vel: " << ballvel.x << " " << ballvel.y << " " << ballvel.z << std::endl;
    std::cout << "last pos: " << ballPhys.lastPosition.x << " " << ballPhys.lastPosition.y << " " << ballPhys.lastPosition.z << std::endl;
    std::cout << "pos: " << ball.position.x << " " << ball.position.y << " " << ball.position.z << std::endl;
    if (currentCameraType == CameraType::FOLLOW) updateFollowCamera();
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
        if (obj.model != nullptr) 
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
    switch (currentCameraType) {
        case CameraType::STATIONARY:
            break;

        case CameraType::FOLLOW:
            processFollowCamera(xoffset, yoffset);
            break;

        case CameraType::FREE:
            cameras[currentCameraType]->ProcessMouseMovement(xoffset, yoffset);
            break;
    }
}

void Game::processFollowCamera(float xoffset, float yoffset, GLboolean constrainPitch) {
    Camera& cam = *cameras[currentCameraType];
    cam.ProcessMouseMovement(xoffset, yoffset);
    updateFollowCamera();
}

void Game::updateFollowCamera() {
    glm::vec3 targetPos = getObjectFromId(ballId).position;
    cameras[CameraType::FOLLOW]->Position = targetPos - cameras[CameraType::FOLLOW]->Forward * cameraFollowDistance;
}

void Game::processMouseScroll(float yoffset) {
    if (currentCameraType == CameraType::FOLLOW) {
        cameraFollowDistance -= yoffset;
        cameraFollowDistance = glm::clamp(cameraFollowDistance, MIN_FOLLOW_CAM_DISTANCE, MAX_FOLLOW_CAM_DISTANCE);
    }
}

void Game::initKeyDebounce() {
    keyDebounce[GLFW_KEY_V] = false;
    keyDebounce[GLFW_KEY_G] = false;
}

void Game::processKeyboard(GLFWwindow* window, float dt) {
    glm::vec3 movement = glm::vec3();


    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !keyDebounce.at(GLFW_KEY_V)) {
        keyDebounce[GLFW_KEY_V] = true;
        currentCameraType = (CameraType)(((int)currentCameraType + 1) % NUM_CAM_TYPES);
    }
    else if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) keyDebounce[GLFW_KEY_V] = false;

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !keyDebounce.at(GLFW_KEY_G)) {
        keyDebounce[GLFW_KEY_G] = true;
        toggleGravity = !toggleGravity;
    }
    else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) keyDebounce[GLFW_KEY_G] = false;

    if (currentCameraType == CameraType::FREE) {
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
        cameras[currentCameraType]->ProcessKeyboard(movement, dt);
    }
}