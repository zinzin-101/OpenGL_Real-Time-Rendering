#include "Game.h"
#include "Random.h"
#include "VerticesData.h"
#include <learnopengl/filesystem.h>
#include <vector>
#include <cmath>
#include <ostream>

static ostream& operator<<(ostream& out, const glm::vec3& v);

bool RenderComparator::operator()(const RenderingObject& obj1, const RenderingObject& obj2) {
    return obj1.distanceFromCamera > obj2.distanceFromCamera;
}

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
    Random::init();

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    int refreshRate = mode->refreshRate;

    initSkybox();
    initColliderOutline();

    camLookVector = getRotatedVector(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), -5.0f);

    stationaryCamera = Camera(glm::vec3(0.0f, 7.0f, 26.0f));
    stationaryCamera.SetForwardVector(camLookVector);
    stationaryCameraPosition = stationaryCamera.Position;
    behindCamera = Camera();
    followCamera = Camera();
    freeCamera = Camera();
    cameras[CameraType::STATIONARY] = &stationaryCamera;
    cameras[CameraType::FOLLOW_PADDLE] = &behindCamera;
    cameras[CameraType::FOLLOW_BALL] = &followCamera;
    cameras[CameraType::FREE] = &freeCamera;
    currentCameraType = CameraType::STATIONARY;
    cameraFollowDistance = DEFAULT_FOLLOW_CAM_DISTANCE;
    cameraHeight = DEFAULT_CAM_HEIGHT;

    toggleGravity = true;
    togglePause = true;
    autopilot = false;
    showCollider = false;

    isMovingPaddle = false;
    isAdjustingLook = false;
    sensitivity = DEFAULT_SENSITIVITY;

    opponentSpeed = 0.0f;
    opponentLastSpeed = opponentSpeed;

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

    dt = 1.0f / (float)refreshRate;
    reset(dt);
}

void Game::setup() {
    Object& playerPaddle = getNewObject();
    playerPaddle.position = glm::vec3(0.0f, 3.0f, 14.0f);
    playerPaddle.model = &paddleModel;
    playerPaddle.name = "PlayerPaddle";
    BoxCollider collider;
    collider.offset = glm::vec3(0.0f, 1.7f, 0.895f);
    collider.size = glm::vec3(3.1f, 3.1f, 2.0f);
    collider.ownerId = playerPaddle.id;
    colliders.emplace_back(collider);
    playerId = playerPaddle.id;

    Object& opponentPaddle = getNewObject();
    opponentPaddle.position = glm::vec3(0.0f, 3.0f, -14.0f);
    rotateObject(opponentPaddle, opponentPaddle.up, 180.0f);
    opponentPaddle.model = &paddleModel;
    opponentPaddle.name = "OpponentPadddle";
    opponentId = opponentPaddle.id;
    collider.offset = glm::vec3(0.0f, 1.7f, -0.895f);
    collider.ownerId = opponentPaddle.id;
    colliders.emplace_back(collider);

    Object& table = getNewObject();
    table.model = &tableModel;
    table.name = "Table";
    collider = BoxCollider();
    collider.offset = glm::vec3(0.0f, -1.9f, 0.0f);
    collider.size = glm::vec3(16.0f, 5.0f, 27.5f);
    collider.ownerId = table.id;
    colliders.emplace_back(collider);

    collider = BoxCollider();
    collider.size = glm::vec3(17.75f, 1.3f, 0.3f);
    collider.offset = glm::vec3(0.0f, 1.3f, 0.0f);
    collider.ownerId = table.id;
    colliders.emplace_back(collider);

    Object& ball = getNewObject();
    ball.position = glm::vec3(0, 10.0f, 0.0f);
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

    Object& topWall = getNewObject();
    rotateObject(topWall, topWall.forward, -180.0f);
    topWall.position = glm::vec3(0.0f, 18.0f, 0.0f);
    topWall.model = &tableModel;
    topWall.name = "TopWall";
    collider = BoxCollider();
    collider.offset = glm::vec3(0.0f, 1.9f, 0.0f);
    collider.size = glm::vec3(16.0f, 5.0f, 27.5f);
    collider.ownerId = topWall.id;
    colliders.emplace_back(collider);

    collider = BoxCollider();
    collider.size = glm::vec3(17.75f, 1.3f, 0.3f);
    collider.offset = glm::vec3(0.0f, -1.3f, 0.0f);
    collider.ownerId = topWall.id;
    colliders.emplace_back(collider);

    Object& leftWall = getNewObject();
    leftWall.name = "LeftWall";
    leftWall.model = &tableModel;
    rotateObject(leftWall, leftWall.forward, -90.0f);
    leftWall.position = glm::vec3(-11.0f, 8.0f, 0.0f);
    collider = BoxCollider();
    collider.ownerId = leftWall.id;
    collider.offset = glm::vec3(-0.45f, -1.0f, 0.0f);
    collider.size = glm::vec3(2.0f, 28.0f, 30.0f);
    colliders.emplace_back(collider);

    collider = BoxCollider();
    collider.size = glm::vec3(1.3f, 17.75f, 0.3f);
    collider.offset = glm::vec3(1.3f, 0.0f, 0.0f);
    collider.ownerId = leftWall.id;
    colliders.emplace_back(collider);

    Object& rightWall = getNewObject();
    rightWall.name = "RightWall";
    rightWall.model = &tableModel;
    rotateObject(rightWall, rightWall.forward, 90.0f);
    rightWall.position = glm::vec3(11.0f, 8.0f, 0.0f);
    collider = BoxCollider();
    collider.ownerId = rightWall.id;
    collider.offset = glm::vec3(0.45f, -1.0f, 0.0f);
    collider.size = glm::vec3(2.0f, 28.0f, 30.0f);
    colliders.emplace_back(collider);

    collider = BoxCollider();
    collider.size = glm::vec3(1.3f, 17.75f, 0.3f);
    collider.offset = glm::vec3(-1.3f, 0.0f, 0.0f);
    collider.ownerId = rightWall.id;
    colliders.emplace_back(collider);

    Object& floor = getNewObject();
    floor.name = "Floor";
    floor.model = &floorModel;
    floor.position = glm::vec3(0.0f, -8.5f, 0.0f);
    collider = BoxCollider();
    collider.ownerId = floor.id;
    collider.size = glm::vec3(50.0f, 2.0f, 50.0f);
    colliders.emplace_back(collider);

    center = (topWall.position + table.position) / 2.0f;
    center.z = playerPaddle.position.z;
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

Object& Game::getObjectById(int id) {
    return objects[id];
}

std::vector<BoxCollider*> Game::getCollidersById(int id) {
    std::vector<BoxCollider*> cols;
    for (BoxCollider& col : colliders) {
        if (col.ownerId == id) cols.emplace_back(&col);
    }
    return cols;
}

std::vector<Physics*> Game::getPhysicsById(int id) {
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
                std::cout << getObjectById(col1.ownerId).name << " and " << getObjectById(col2.ownerId).name << " is colliding" << std::endl;
                handleCollision(col1, col2);
            }
        }
    }
}

void Game::handleCollision(BoxCollider& col1, BoxCollider& col2) {
    Object& obj1 = getObjectById(col1.ownerId);
    Object& obj2 = getObjectById(col2.ownerId);
        
    if (obj1.id == ballId) {
        if (obj2.model == &tableModel) {
            handleBallBounce(obj1, obj2, col2);
            return;
        }

        if (obj2.id == playerId || obj2.id == opponentId) {
            handlePaddleBounce(obj1, obj2);
            return;
        }
    }

    if (obj2.id == ballId) {
        if (obj1.model == &tableModel) {
            handleBallBounce(obj2, obj1, col1);
            return;
        }

        if (obj1.id == playerId || obj1.id == opponentId) {
            handlePaddleBounce(obj2, obj1);
            return;
        }
    }
}

void Game::handleBallBounce(Object& ball, Object& wall, BoxCollider& wallCol) {
    Physics& ballPhysics = *(getPhysicsById(ball.id))[0];
    BoxCollider& ballCol = *(getCollidersById(ball.id))[0];
    BoxCollider& netCol = *(getCollidersById(wall.id))[1];

    glm::vec3 wallPos = wall.position;

    if (wallPos.x > 0.0f) { // right wall
        if (&netCol == &wallCol) { 
            if (ballPhysics.lastPosition.z > 0.0f) { // net toward player
                float displacement = ball.position.z - ballPhysics.lastPosition.z;
                ball.position.z = wall.position.z + wallCol.offset.z + (wallCol.size.z * 0.5f + ballCol.offset.z + ballCol.size.z * 0.5f);
                ballPhysics.lastPosition.z = ball.position.z + displacement;
            }
            else if (ballPhysics.lastPosition.z < 0.0f) { // net toward opponent
                float displacement = ball.position.z - ballPhysics.lastPosition.z;
                ball.position.z = wall.position.z + wallCol.offset.z - (wallCol.size.z * 0.5f + ballCol.offset.z + ballCol.size.z * 0.5f);
                ballPhysics.lastPosition.z = ball.position.z + displacement;
            }
            return;
        }

        float displacement = ball.position.x - ballPhysics.lastPosition.x;
        ball.position.x = wall.position.x + wallCol.offset.x - wallCol.size.x * 0.5f - ballCol.offset.x - ballCol.size.x * 0.5f;
        ballPhysics.lastPosition.x = ball.position.x + displacement * BOUNCE_COEFFICIENT;
    }
    else if (wallPos.x < 0.0f) { // left wall
        if (&netCol == &wallCol) { 
            if (ballPhysics.lastPosition.z > 0.0f) { // net toward player
                float displacement = ball.position.z - ballPhysics.lastPosition.z;
                ball.position.z = wall.position.z + wallCol.offset.z + (wallCol.size.z * 0.5f + ballCol.offset.z + ballCol.size.z * 0.5f);
                ballPhysics.lastPosition.z = ball.position.z + displacement;
            }
            else if (ballPhysics.lastPosition.z < 0.0f) { // net toward opponent
                float displacement = ball.position.z - ballPhysics.lastPosition.z;
                ball.position.z = wall.position.z + wallCol.offset.z - (wallCol.size.z * 0.5f + ballCol.offset.z + ballCol.size.z * 0.5f);
                ballPhysics.lastPosition.z = ball.position.z + displacement;
            }
            return;
        }

        float displacement = ball.position.x - ballPhysics.lastPosition.x;
        ball.position.x = wall.position.x + wallCol.offset.x + wallCol.size.x * 0.5f + ballCol.offset.x + ballCol.size.x * 0.5f;
        ballPhysics.lastPosition.x = ball.position.x + displacement * BOUNCE_COEFFICIENT;
    }
    else if (wallPos.y <= 0.0f) { // table
        if (wallCol.offset.y < 0.0f) { // surface
            float displacement = ball.position.y - ballPhysics.lastPosition.y;
            ball.position.y = wall.position.y + wallCol.offset.y + wallCol.size.y * 0.5f + ballCol.offset.y + ballCol.size.y * 0.5f;
            ballPhysics.lastPosition.y = ball.position.y + displacement * BOUNCE_COEFFICIENT;
            return;
        }
        
        // net
        if (ballPhysics.lastPosition.z > 0.0f) { // toward player
            float displacement = ball.position.z - ballPhysics.lastPosition.z;
            ball.position.z = wall.position.z + wallCol.offset.z + wallCol.size.z * 0.5f + ballCol.offset.z + ballCol.size.z * 0.5f;
            ballPhysics.lastPosition.z = ball.position.z + displacement;
        }
        else if (ballPhysics.lastPosition.z < 0.0f) { // toward opponent
            float displacement = ball.position.z - ballPhysics.lastPosition.z;
            ball.position.z = wall.position.z - wallCol.offset.z - wallCol.size.z * 0.5f - ballCol.offset.z - ballCol.size.z * 0.5f;
            ballPhysics.lastPosition.z = ball.position.z + displacement;
        }
    }
    else if (wallPos.y > 0.0f) { // top
        if (wallCol.offset.y > 0.0f) { // surface
            float displacement = ball.position.y - ballPhysics.lastPosition.y;
            ball.position.y = wall.position.y + wallCol.offset.y - (wallCol.size.y * 0.5f + ballCol.offset.y + ballCol.size.y * 0.5f);
            ballPhysics.lastPosition.y = ball.position.y + displacement * BOUNCE_COEFFICIENT;
            return;
        }

        // net
        if (ballPhysics.lastPosition.z > 0.0f) { // toward player
            float displacement = ball.position.z - ballPhysics.lastPosition.z;
            ball.position.z = wall.position.z + wallCol.offset.z + wallCol.size.z * 0.5f + ballCol.offset.z + ballCol.size.z * 0.5f;
            ballPhysics.lastPosition.z = ball.position.z + displacement;
        }
        else if (ballPhysics.lastPosition.z < 0.0f) { // toward opponent
            float displacement = ball.position.z - ballPhysics.lastPosition.z;
            ball.position.z = wall.position.z - wallCol.offset.z - wallCol.size.z * 0.5f - ballCol.offset.z - ballCol.size.z * 0.5f;
            ballPhysics.lastPosition.z = ball.position.z + displacement;
        }
    }
}

void Game::handlePaddleBounce(Object& ball, Object& paddle) {
    Physics& ballPhys = *getPhysicsById(ball.id)[0];
    BoxCollider& ballCol = *getCollidersById(ball.id)[0];
    BoxCollider& paddleCol = *getCollidersById(paddle.id)[0];
    glm::vec3 displacement = ball.position - ballPhys.lastPosition;
    float distance = glm::length(displacement);
    glm::vec3 paddleOrigin = paddle.position + paddleCol.offset;
    glm::vec3 hitOffset = ball.position - paddleOrigin;
    float horizontalAngle = (hitOffset.x / (paddleCol.size.x * 0.5f)) * MAX_PADDLE_BOUNCE_ANGLE;
    float verticalAngle = (hitOffset.y / (paddleCol.size.y * 0.5f)) * MAX_PADDLE_BOUNCE_ANGLE;
    glm::vec3 direction = glm::vec3(
        sin(glm::radians(horizontalAngle)),
        sin(glm::radians(verticalAngle)),
        (paddle.position.z < 0.0f ? -1.0f : 1.0f)
    );

    float zOffset = paddleCol.offset.z + (paddle.position.z < 0.0f ? 1.0f : -1.0f) * (paddleCol.size.z * 0.5f + ballCol.offset.z + ballCol.size.z * 0.5f);
    glm::vec3 newDisplacement = glm::normalize(direction) * distance * PADDLE_BOUNCE_COEFFICIENT;
    ball.position.z = paddle.position.z + zOffset;
    ballPhys.lastPosition = ball.position + newDisplacement;
}

void Game::reset(float dt) {
    objects.clear();
    colliders.clear();
    physics.clear();

    setup();

    getObjectById(playerId).position = glm::vec3(0.0f, 3.0f, 14.0f);
    getObjectById(opponentId).position = glm::vec3(0.0f, 3.0f, -14.0f);
    
    Object& ball = getObjectById(ballId);
    Physics& phys = *getPhysicsById(ballId)[0];
    ball.position = glm::vec3(0.0f, 5.0f, 0.0f);
    phys.lastPosition = ball.position;
    setVelocity(phys, glm::vec3(0.0f, 0.0f, -6.0f), dt);
    //addVelocity(phys, glm::vec3(0.0f, 0.0f, 5.0f), dt);
}

void Game::accelerate(Physics& phys, glm::vec3 a) {
    phys.acceleration += a;
}

void Game::setVelocity(Physics& phys, glm::vec3 vel, float dt){
    phys.lastPosition = getObjectById(phys.ownerId).position - (vel * dt);
}

void Game::addVelocity(Physics& phys, glm::vec3 vel, float dt){
    phys.lastPosition -= vel * dt;
}

glm::vec3 Game::getVelocity(Physics& phys, float dt) {
    Object& obj = getObjectById(phys.ownerId);
    return (obj.position - phys.lastPosition) / dt;
}

void Game::computePhysics(float dt) {
    for (Physics& p : physics) {
        Object& obj = getObjectById(p.ownerId);

        if (toggleGravity) accelerate(p, DEFAULT_GRAVITY);

        glm::vec3 displacement = obj.position - p.lastPosition;
        p.lastPosition = obj.position;
        obj.position += displacement + p.acceleration * dt * dt;
        p.acceleration = glm::vec3(0.0f);
    }
}

glm::vec3 Game::getRotatedVector(glm::vec3 v, glm::vec3 axis, float deg) {
    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    v = rotMat * glm::vec4(v, 1.0f);
    return v;
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

void Game::rotateEveryThing(glm::vec3 axis, float deg) {
    if ((int)glm::round(deg) % 90 != 0) return; // Can only rotate orthogonally due to AABB

    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    
    for (Object& obj : objects) {
        rotateObject(obj, axis, deg);
        obj.position = rotMat * glm::vec4(obj.position, 1.0f);
    }

    for (BoxCollider& col : colliders) {
        col.offset = rotMat * glm::vec4(col.offset, 1.0f);
        col.size = rotMat * glm::vec4(col.size, 1.0f);
    }

    for (Physics& phys : physics) {
        phys.lastPosition = rotMat * glm::vec4(phys.lastPosition, 1.0f);
    }

    for (int i = 0; i < NUM_CAM_TYPES; i++) {
        CameraType type = (CameraType)i;
        if (type == CameraType::FREE || type == CameraType::FOLLOW_BALL) continue;

        Camera& camera = *cameras[i];
        camera.Forward = rotMat * glm::vec4(camera.Forward, 0.0f);
        camera.WorldUp = rotMat * glm::vec4(camera.Up, 0.0f);
        camera.SetForwardVector(camera.Forward);
    }
}

void Game::rotatePlayerPaddle(glm::vec3 axis, float deg) {
    if ((int)glm::round(deg) % 90 != 0) return; // Can only rotate orthogonally due to AABB

    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);

    Object& player = getObjectById(playerId);
    BoxCollider& playerCol = *getCollidersById(playerId)[0];

    rotateObject(player, axis, deg);
    player.position = rotMat * glm::vec4(player.position, 1.0f);

    playerCol.offset = rotMat * glm::vec4(playerCol.offset, 1.0f);
    //playerCol.size = rotMat * glm::vec4(playerCol.size, 1.0f);

    for (int i = 0; i < NUM_CAM_TYPES; i++) {
        CameraType type = (CameraType)i;
        if (type == CameraType::FREE || type == CameraType::FOLLOW_BALL) continue;

        Camera& camera = *cameras[i];
        float oldZ = camera.Position.z;
        camera.Position = center;
        camera.Position.z = oldZ;
        camera.Forward = rotMat * glm::vec4(camera.Forward, 0.0f);
        camera.WorldUp = rotMat * glm::vec4(camera.Up, 0.0f);
        camera.SetForwardVector(camera.Forward);
    }
}

void Game::update(float dt) {
    if (currentCameraType == CameraType::STATIONARY) updateStationaryCamera();
    if (currentCameraType == CameraType::FOLLOW_BALL) updateFollowCamera();
    if (currentCameraType == CameraType::FOLLOW_PADDLE) updateBehindCamera();

    this->dt = dt;

    if (togglePause) return;

    Object& ball = getObjectById(ballId);
    Physics& ballPhys = *getPhysicsById(ballId)[0];

    for (unsigned int i = 0; i < PHYSICS_RESOLUTION; i++) {
        computePhysics(dt / (float)PHYSICS_RESOLUTION);
        computeCollision();

        float ballSpeed = glm::length(getVelocity(ballPhys, dt / (float)PHYSICS_RESOLUTION));
        if (ballSpeed > MAX_BALL_SPEED) {
            glm::vec3 newVelocity = glm::normalize((ball.position - ballPhys.lastPosition)) * ballSpeed;
            setVelocity(ballPhys, newVelocity, dt / (float)PHYSICS_RESOLUTION);
        }

        if (ball.position.y < -10.0f) {
            reset(dt / (float)PHYSICS_RESOLUTION);
        }
    }

    if (autopilot) {
        Object& player = getObjectById(playerId);
        BoxCollider& playerCol = *getCollidersById(playerId)[0];
        player.position.x = ball.position.x - playerCol.offset.x;
        player.position.y = ball.position.y - playerCol.offset.y;
    }

    Object& opponent = getObjectById(opponentId);
    BoxCollider& opponentCol = *getCollidersById(opponentId)[0];
    glm::vec3 targetPos = opponent.position;
    targetPos.x = ball.position.x - opponentCol.offset.x;
    targetPos.y = ball.position.y - opponentCol.offset.y;
    float currentSpeed = (MAX_AI_MOVE_SPEED - MIN_AI_MOVE_SPEED) * Random::randFloat() * dt;
    if ((opponentSpeed - opponentLastSpeed) > MAX_AI_MOVE_ACCELERATION) currentSpeed = MAX_AI_MOVE_ACCELERATION;
    glm::vec3 moveVector = targetPos - opponent.position;
    opponent.position += moveVector * currentSpeed;

    opponentLastSpeed = opponentSpeed;
    opponentSpeed = currentSpeed;
}

glm::mat4 Game::getProjection() const {
    return glm::perspective(glm::radians(FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
}

void Game::render(float dt) {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    shader.use();
    // view/projection transformations
    glm::mat4 projection = getProjection();
    glm::mat4 view = cameras[currentCameraType]->GetViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setVec3("viewPos", cameras[currentCameraType]->Position);

    glm::vec3 lightPos0, lightPos1, lightPos2, lightPos3;
    lightPos3.y = 30.0f;
    lightPos0 = lightPos1 = lightPos2 = lightPos3;
    lightPos0.x = 30.0f;
    lightPos1.x = -30.0f;
    lightPos2.x = -30.0f;
    lightPos3.x = 30.0f;
    lightPos0.z = 10.0f;
    lightPos1.z = 10.0f;
    lightPos2.z = -10.0f;
    lightPos3.z = -10.0f;

    shader.setVec3("pointLights[0].position", lightPos0);
    shader.setVec3("pointLights[0].ambient", glm::vec3(0.1f));
    shader.setVec3("pointLights[0].diffuse", glm::vec3(0.2f));
    shader.setVec3("pointLights[0].specular", glm::vec3(0.3f));
    shader.setFloat("pointLights[0].constant", 0.5f);
    shader.setFloat("pointLights[0].linear", 0.000014f);
    shader.setFloat("pointLights[0].quadratic", 0.00001f);

    shader.setVec3("pointLights[1].position", lightPos1);
    shader.setVec3("pointLights[1].ambient", glm::vec3(0.1f));
    shader.setVec3("pointLights[1].diffuse", glm::vec3(0.2f));
    shader.setVec3("pointLights[1].specular", glm::vec3(0.3f));
    shader.setFloat("pointLights[1].constant", 0.5f);
    shader.setFloat("pointLights[1].linear", 0.000014f);
    shader.setFloat("pointLights[1].quadratic", 0.00001f);

    shader.setVec3("pointLights[2].position", lightPos2);
    shader.setVec3("pointLights[2].ambient", glm::vec3(0.1f));
    shader.setVec3("pointLights[2].diffuse", glm::vec3(0.2f));
    shader.setVec3("pointLights[2].specular", glm::vec3(0.3f));
    shader.setFloat("pointLights[2].constant", 0.5f);
    shader.setFloat("pointLights[2].linear", 0.000014f);
    shader.setFloat("pointLights[2].quadratic", 0.00001f);

    shader.setVec3("pointLights[3].position", lightPos3);
    shader.setVec3("pointLights[3].ambient", glm::vec3(0.1f));
    shader.setVec3("pointLights[3].diffuse", glm::vec3(0.2f));
    shader.setVec3("pointLights[3].specular", glm::vec3(0.3f));
    shader.setFloat("pointLights[3].constant", 0.5f);
    shader.setFloat("pointLights[3].linear", 0.000014f);
    shader.setFloat("pointLights[3].quadratic", 0.00001f);
    shader.setFloat("shininess", 20.0f);

    opacityRenderQueue = std::priority_queue<RenderingObject, std::vector<RenderingObject>, RenderComparator>(); // clear priority queue

    for (const Object& obj : objects) {
        if (obj.id == playerId && !autopilot) {
            opacityRenderQueue.push(RenderingObject(&obj, glm::length(obj.position - cameras[currentCameraType]->Position)));
            continue;
        }

        if (obj.model != nullptr) {
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

            shader.setFloat("opacity", 1.0f);
            obj.model->Draw(shader);
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    while (!opacityRenderQueue.empty()) {
        RenderingObject renderingObj = opacityRenderQueue.top();
        opacityRenderQueue.pop();

        const Object& obj = *renderingObj.object;

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

        shader.setFloat("opacity", PLAYER_OPACITY);
        obj.model->Draw(shader);

    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    // collider
    if (showCollider) {
        for (const BoxCollider& collider : colliders) {
            drawCollider(collider);
        }
    }
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

    const Object& obj = getObjectById(collider.ownerId);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), collider.offset + obj.position) * glm::scale(glm::mat4(1.0f), collider.size);
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
    const Object& p1 = getObjectById(c1.ownerId);
    const Object& p2 = getObjectById(c2.ownerId);
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
    if (isAdjustingLook && isMovingPaddle) {
        cameraHeight += yoffset * sensitivity;
        return;
    }

    if (isAdjustingLook) {
        camLookVector = getRotatedVector(camLookVector, cameras[CameraType::STATIONARY]->Right, yoffset * sensitivity);
        camLookVector = getRotatedVector(camLookVector, -cameras[CameraType::STATIONARY]->WorldUp, xoffset * sensitivity);
        return;
    }

    if (isMovingPaddle) {
        glm::vec3 movement = (cameras[currentCameraType]->Right * xoffset + cameras[CameraType::STATIONARY]->WorldUp * yoffset) * sensitivity;
        getObjectById(playerId).position += movement;
        return;
    }

    switch (currentCameraType) {
        case CameraType::FOLLOW_BALL:
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

void Game::updateStationaryCamera() {
    stationaryCamera.SetForwardVector(camLookVector);
    cameras[CameraType::STATIONARY]->Position = stationaryCameraPosition + (cameras[CameraType::STATIONARY]->Up * cameraHeight) - camLookVector * (cameraFollowDistance - DEFAULT_FOLLOW_CAM_DISTANCE);
}

void Game::updateFollowCamera() {
    glm::vec3 targetPos = getObjectById(ballId).position;
    cameras[CameraType::FOLLOW_BALL]->Position = targetPos - cameras[CameraType::FOLLOW_BALL]->Forward * cameraFollowDistance;
}

void Game::updateBehindCamera() {
    Object& player = getObjectById(playerId);
    cameras[CameraType::FOLLOW_PADDLE]->SetForwardVector(camLookVector);
    cameras[CameraType::FOLLOW_PADDLE]->Position = player.position + (cameras[CameraType::FOLLOW_PADDLE]->Up * cameraHeight) - camLookVector * cameraFollowDistance;
}

void Game::processMouseScroll(float yoffset) {
    cameraFollowDistance -= yoffset;
    cameraFollowDistance = glm::clamp(cameraFollowDistance, MIN_FOLLOW_CAM_DISTANCE, MAX_FOLLOW_CAM_DISTANCE);
}

void Game::processMouseButton(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) isMovingPaddle = true;
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) isMovingPaddle = false;

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) isAdjustingLook = true;
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) isAdjustingLook = false;
}

void Game::initKeyDown() {
    keyDown[GLFW_KEY_V] = false;
    keyDown[GLFW_KEY_G] = false;
    keyDown[GLFW_KEY_ENTER] = false;
    keyDown[GLFW_KEY_O] = false;
    keyDown[GLFW_KEY_C] = false;
    keyDown[GLFW_KEY_P] = false;
    keyDown[GLFW_KEY_Z] = false;
    keyDown[GLFW_KEY_X] = false;
}

void Game::processKeyboard(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !keyDown.at(GLFW_KEY_V)) {
        keyDown[GLFW_KEY_V] = true;
        CameraType nextCamType = (CameraType)(((int)currentCameraType + 1) % NUM_CAM_TYPES);
        if (nextCamType == CameraType::FREE) cameras[nextCamType] = cameras[currentCameraType];
        currentCameraType = nextCamType;
    }
    else if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) keyDown[GLFW_KEY_V] = false;

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !keyDown.at(GLFW_KEY_G)) {
        keyDown[GLFW_KEY_G] = true;
        toggleGravity = !toggleGravity;
    }
    else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) keyDown[GLFW_KEY_G] = false;

    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !keyDown.at(GLFW_KEY_ENTER)) {
        keyDown[GLFW_KEY_ENTER] = true;
        togglePause = !togglePause;
    }
    else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) keyDown[GLFW_KEY_ENTER] = false;

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !keyDown.at(GLFW_KEY_P)) {
        keyDown[GLFW_KEY_P] = true;
        togglePause = !togglePause;
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) keyDown[GLFW_KEY_P] = false;

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !keyDown.at(GLFW_KEY_O)) {
        keyDown[GLFW_KEY_O] = true;
        autopilot = !autopilot;
    }
    else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE) keyDown[GLFW_KEY_O] = false;

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !keyDown.at(GLFW_KEY_C)) {
        keyDown[GLFW_KEY_C] = true;
        showCollider = !showCollider;
    }
    else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) keyDown[GLFW_KEY_C] = false;

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && !keyDown.at(GLFW_KEY_Z)) {
        keyDown[GLFW_KEY_Z] = true;
        //rotateEveryThing(glm::vec3(0, 0, 1), 90.0f);
        rotatePlayerPaddle(glm::vec3(0, 0, -1), 90.0f);
    }
    else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE) keyDown[GLFW_KEY_Z] = false;

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && !keyDown.at(GLFW_KEY_X)) {
        keyDown[GLFW_KEY_X] = true;
        rotatePlayerPaddle(glm::vec3(0, 0, -1), -90.0f);
    }
    else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE) keyDown[GLFW_KEY_X] = false;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE) keyDown[GLFW_KEY_DOWN] = false;

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        reset(dt);
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        sensitivity += 0.1f * dt;
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        sensitivity -= 0.1f * dt;
    }
    sensitivity = max(sensitivity, 0.001f);

    if (currentCameraType == CameraType::FREE) {
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

        cameras[currentCameraType]->ProcessKeyboard(movement, dt);
    }
}

ostream& operator<<(ostream& out, const glm::vec3& v) {
    out << v.x << " " << v.y << " " << v.z;
    return out;
}