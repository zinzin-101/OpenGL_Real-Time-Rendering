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
    waveShader("waves.vs", "waves.fs"),
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

void Game::init() {
    Random::init();

    //GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    //const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    //int refreshRate = mode->refreshRate;

    initSkybox();
    initColliderOutline();

}

void Game::setup() {

}

//Object& Game::getNewObject() {
//    Object obj;
//    obj.id = objects.size();
//    obj.model = nullptr;
//    obj.name = "Object";
//    obj.position = glm::vec3();
//    obj.forward = glm::vec3(0, 0, 1);
//    obj.right = glm::vec3(1, 0, 0);
//    obj.up = glm::vec3(0, 1, 0);
//
//    objects.emplace_back(obj);
//
//    return objects[obj.id];
//}
//
//Object& Game::getNewObjectWithCollider() {
//    Object& obj = getNewObject();
//    BoxCollider collider;
//    collider.offset = glm::vec3();
//    collider.size = glm::vec3(1.0f);
//    collider.ownerId = obj.id;
//    colliders.emplace_back(collider);
//    return obj;
//}
//
//Object& Game::getObjectById(int id) {
//    return objects[id];
//}

//std::vector<BoxCollider*> Game::getCollidersById(int id) {
//    std::vector<BoxCollider*> cols;
//    for (BoxCollider& col : colliders) {
//        if (col.ownerId == id) cols.emplace_back(&col);
//    }
//    return cols;
//}
//
//std::vector<Physics*> Game::getPhysicsById(int id) {
//    std::vector<Physics*> phys;
//    for (Physics& p : physics) {
//        if (p.ownerId == id) phys.emplace_back(&p);
//    }
//    return phys;
//}

//void Game::computeCollision() {
//    int numOfColliders = colliders.size();
//    for (int i = 0; i < numOfColliders; i++) {
//        for (int j = i + 1; j < numOfColliders; j++) {
//            BoxCollider& col1 = colliders[i];
//            BoxCollider& col2 = colliders[j];
//
//            if (col1.ownerId == col2.ownerId) continue;
//
//            if (isColliding(col1, col2)) {
//                //std::cout << getObjectById(col1.ownerId).name << " and " << getObjectById(col2.ownerId).name << " is colliding" << std::endl;
//                handleCollision(col1, col2);
//            }
//        }
//    }
//}

//void Game::accelerate(Physics& phys, glm::vec3 a) {
//    phys.acceleration += a;
//}
//
//void Game::setVelocity(Physics& phys, glm::vec3 vel, float dt){
//    phys.lastPosition = getObjectById(phys.ownerId).position - (vel * dt);
//}
//
//void Game::addVelocity(Physics& phys, glm::vec3 vel, float dt){
//    phys.lastPosition -= vel * dt;
//}
//
//glm::vec3 Game::getVelocity(Physics& phys, float dt) {
//    Object& obj = getObjectById(phys.ownerId);
//    return (obj.position - phys.lastPosition) / dt;
//}

//void Game::computePhysics(float dt) {
//    for (Physics& p : physics) {
//        Object& obj = getObjectById(p.ownerId);
//
//        if (toggleGravity) accelerate(p, gravity[currentGravityDirection]);
//
//        glm::vec3 displacement = obj.position - p.lastPosition;
//        p.lastPosition = obj.position;
//        obj.position += displacement + p.acceleration * dt * dt;
//        p.acceleration = glm::vec3(0.0f);
//    }
//}

//glm::vec3 Game::getRotatedVector(glm::vec3 v, glm::vec3 axis, float deg) {
//    float angle = glm::radians(deg);
//    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
//    v = rotMat * glm::vec4(v, 1.0f);
//    return v;
//}
//
//void Game::rotateObject(Object& obj, glm::vec3 axis, float deg) {
//    float angle = glm::radians(deg);
//    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
//    glm::vec4 forward = rotMat * glm::vec4(obj.forward, 1.0f);
//    glm::vec4 up = rotMat * glm::vec4(obj.up, 1.0f);
//    glm::vec4 right = rotMat * glm::vec4(obj.right, 1.0f);
//
//    obj.forward = forward;
//    obj.up = up;
//    obj.right = right;
//}
//
//void Game::rotateEveryThing(glm::vec3 axis, float deg) {
//    if ((int)glm::round(deg) % 90 != 0) return; // Can only rotate orthogonally due to AABB
//
//    float angle = glm::radians(deg);
//    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
//    
//    for (Object& obj : objects) {
//        rotateObject(obj, axis, deg);
//        obj.position = rotMat * glm::vec4(obj.position, 1.0f);
//    }
//
//    for (BoxCollider& col : colliders) {
//        col.offset = rotMat * glm::vec4(col.offset, 1.0f);
//        col.size = rotMat * glm::vec4(col.size, 1.0f);
//    }
//
//    for (Physics& phys : physics) {
//        phys.lastPosition = rotMat * glm::vec4(phys.lastPosition, 1.0f);
//    }
//
//    for (int i = 0; i < NUM_CAM_TYPES; i++) {
//        CameraType type = (CameraType)i;
//        if (type == CameraType::FREE || type == CameraType::FOLLOW_BALL) continue;
//
//        Camera& camera = *cameras[i];
//        camera.Forward = rotMat * glm::vec4(camera.Forward, 0.0f);
//        camera.WorldUp = rotMat * glm::vec4(camera.Up, 0.0f);
//        camera.SetForwardVector(camera.Forward);
//    }
//}
//
//void Game::rotatePlayerPaddle(glm::vec3 axis, float deg) {
//    if ((int)glm::round(deg) % 90 != 0) return; // Can only rotate orthogonally due to AABB
//
//    float angle = glm::radians(deg);
//    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
//
//    Object& player = getObjectById(playerId);
//    BoxCollider& playerCol = *getCollidersById(playerId)[0];
//
//    rotateObject(player, axis, deg);
//    player.position = glm::translate(glm::mat4(1.0f), player.position) * rotMat * glm::translate(glm::mat4(1.0f), -player.position) * glm::vec4(player.position, 1.0f);
//
//    playerCol.offset = rotMat * glm::vec4(playerCol.offset, 1.0f);
//    //playerCol.size = rotMat * glm::vec4(playerCol.size, 1.0f);
//
//    for (int i = 0; i < NUM_CAM_TYPES; i++) {
//        CameraType type = (CameraType)i;
//        if (type == CameraType::FREE || type == CameraType::FOLLOW_BALL) continue;
//
//        Camera& camera = *cameras[i];
//        float oldZ = camera.Position.z;
//        camera.Position = center;
//        camera.Position.z = oldZ;
//        camera.Forward = rotMat * glm::vec4(camera.Forward, 0.0f);
//        camera.WorldUp = rotMat * glm::vec4(camera.Up, 0.0f);
//        camera.SetForwardVector(camera.Forward);
//    }
//
//    if (deg > 0) {
//        int nextDir = currentGravityDirection - 1;
//        if (nextDir < 0) nextDir = GravityDirection::LEFT;
//        currentGravityDirection = (GravityDirection)nextDir;
//    }
//    else {
//        currentGravityDirection = (GravityDirection)((currentGravityDirection + 1) % 4);
//    }
//        
//}

void Game::update(float dt) {

    this->dt = dt;
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
    skyboxView = glm::mat4((glm::mat3)camera.GetViewMatrix());
    skyboxProjection = getProjection();
    skyboxShader.setMat4("view", skyboxView);
    skyboxShader.setMat4("projection", skyboxProjection);
    drawSkybox();

    waveShader.use();
    // view/projection transformations
    glm::mat4 projection = getProjection();
    glm::mat4 view = camera.GetViewMatrix();
    waveShader.setMat4("projection", projection);
    waveShader.setMat4("view", view);
    waveShader.setVec3("viewPos", camera.Position);

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

    waveShader.setVec3("pointLights[0].position", lightPos0);
    waveShader.setVec3("pointLights[0].ambient", glm::vec3(0.1f));
    waveShader.setVec3("pointLights[0].diffuse", glm::vec3(0.2f));
    waveShader.setVec3("pointLights[0].specular", glm::vec3(0.3f));
    waveShader.setFloat("pointLights[0].constant", 0.5f);
    waveShader.setFloat("pointLights[0].linear", 0.000014f);
    waveShader.setFloat("pointLights[0].quadratic", 0.00001f);

    waveShader.setVec3("pointLights[1].position", lightPos1);
    waveShader.setVec3("pointLights[1].ambient", glm::vec3(0.1f));
    waveShader.setVec3("pointLights[1].diffuse", glm::vec3(0.2f));
    waveShader.setVec3("pointLights[1].specular", glm::vec3(0.3f));
    waveShader.setFloat("pointLights[1].constant", 0.5f);
    waveShader.setFloat("pointLights[1].linear", 0.000014f);
    waveShader.setFloat("pointLights[1].quadratic", 0.00001f);

    waveShader.setVec3("pointLights[2].position", lightPos2);
    waveShader.setVec3("pointLights[2].ambient", glm::vec3(0.1f));
    waveShader.setVec3("pointLights[2].diffuse", glm::vec3(0.2f));
    waveShader.setVec3("pointLights[2].specular", glm::vec3(0.3f));
    waveShader.setFloat("pointLights[2].constant", 0.5f);
    waveShader.setFloat("pointLights[2].linear", 0.000014f);
    waveShader.setFloat("pointLights[2].quadratic", 0.00001f);

    waveShader.setVec3("pointLights[3].position", lightPos3);
    waveShader.setVec3("pointLights[3].ambient", glm::vec3(0.1f));
    waveShader.setVec3("pointLights[3].diffuse", glm::vec3(0.2f));
    waveShader.setVec3("pointLights[3].specular", glm::vec3(0.3f));
    waveShader.setFloat("pointLights[3].constant", 0.5f);
    waveShader.setFloat("pointLights[3].linear", 0.000014f);
    waveShader.setFloat("pointLights[3].quadratic", 0.00001f);
    waveShader.setFloat("shininess", 20.0f);

    //opacityRenderQueue = std::priority_queue<RenderingObject, std::vector<RenderingObject>, RenderComparator>(); // clear priority queue

    // rendering object
    //for (const Object& obj : objects) {
    //    if (obj.model != nullptr) {
    //        glm::mat4 rotMat(
    //            glm::vec4(obj.right, 0.0f),
    //            glm::vec4(obj.up, 0.0f),
    //            glm::vec4(obj.forward, 0.0f),
    //            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    //        );

    //        glm::mat4 model =
    //            glm::translate(glm::mat4(1.0f), obj.position) *
    //            rotMat *
    //            modelToWorld[obj.model];

    //        shader.setMat4("model", model);

    //        shader.setFloat("opacity", 1.0f);
    //        obj.model->Draw(shader);
    //    }
    //}

    // rendering translucent object
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDepthMask(GL_FALSE);
    //while (!opacityRenderQueue.empty()) {
    //    RenderingObject renderingObj = opacityRenderQueue.top();
    //    opacityRenderQueue.pop();

    //    const Object& obj = *renderingObj.object;

    //    glm::mat4 rotMat(
    //        glm::vec4(obj.right, 0.0f),
    //        glm::vec4(obj.up, 0.0f),
    //        glm::vec4(obj.forward, 0.0f),
    //        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    //    );

    //    glm::mat4 model =
    //        glm::translate(glm::mat4(1.0f), obj.position) *
    //        rotMat *
    //        modelToWorld[obj.model];

    //    shader.setMat4("model", model);

    //    shader.setFloat("opacity", PLAYER_OPACITY);
    //    obj.model->Draw(shader);

    //}
    //glDisable(GL_BLEND);
    //glDepthMask(GL_TRUE);

    // collider
    //if (showCollider) {
    //    for (const BoxCollider& collider : colliders) {
    //        drawCollider(collider);
    //    }
    //}
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

//void Game::drawCollider(const BoxCollider& collider) {
//    outlineShader.use();
//
//    const Object& obj = getObjectById(collider.ownerId);
//
//    glm::mat4 model = glm::translate(glm::mat4(1.0f), collider.offset + obj.position) * glm::scale(glm::mat4(1.0f), collider.size);
//    glm::mat4 view = cameras[currentCameraType]->GetViewMatrix();
//    glm::mat4 projection = getProjection();
//    outlineShader.setMat4("model", model);
//    outlineShader.setMat4("view", view);
//    outlineShader.setMat4("projection", projection);
//    outlineShader.setVec4("lineColor", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
//    glBindVertexArray(outlineVAO);
//    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
//    //glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
//    glBindVertexArray(0);
//}

//bool Game::isColliding(const BoxCollider& c1, const BoxCollider& c2) {
//    const Object& p1 = getObjectById(c1.ownerId);
//    const Object& p2 = getObjectById(c2.ownerId);
//    glm::vec3 pos1 = c1.offset + p1.position;
//    glm::vec3 pos2 = c2.offset + p2.position;
//    glm::vec3 halfSize1 = c1.size / 2.0f;
//    glm::vec3 halfSize2 = c2.size / 2.0f;
//
//    // AABB collision detection
//    bool xCollided = abs(pos1.x - pos2.x) < (halfSize1.x + halfSize2.x);
//    bool yCollided = abs(pos1.y - pos2.y) < (halfSize1.y + halfSize2.y);
//    bool zCollided = abs(pos1.z - pos2.z) < (halfSize1.z + halfSize2.z);
//    
//    bool collided = xCollided && yCollided && zCollided;
//
//    return collided;
//}

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