#include "Game.h"
#include <learnopengl/filesystem.h>
#include "VerticesData.h"
#include <vector>
#include <cmath>

Game::Game(Camera& camera):
    camera(camera),
    shader("vertex.vs", "fragment.fs"),
    outlineShader("collider_outline.vs", "collider_outline.fs"),
    skyboxShader("skybox.vs", "skybox.fs"),
    f22Model(FileSystem::getPath("resources/objects/f22/F22Raptor.obj")),
    missileModel(FileSystem::getPath("resources/objects/missile/AIM120D.obj")),
    mig29Model(FileSystem::getPath("resources/objects/mig29/MiG-29.obj"))
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

    Plane& plane = getNewPlane();
    plane.position = glm::vec3(-2, 0, 0);
    plane.model = &f22Model;
    BoxCollider collider;
    collider.offset = glm::vec3();
    collider.size = glm::vec3(1.0f);
    collider.ownerId = plane.id;
    colliders.emplace_back(collider);
    playerId = plane.id;

    Plane& p = getNewPlane();
    p.model = &mig29Model;
    collider.ownerId = p.id;
    colliders.emplace_back(collider);

    Plane& p2 = getNewPlane();
    p2.position = glm::vec3(2, 0, 0);
    p2.model = &f22Model;
    collider.ownerId = p2.id;
    colliders.emplace_back(collider);
}

Plane& Game::getNewPlane() {
    Plane plane;
    plane.id = planes.size();
    plane.position = glm::vec3();
    plane.forward = glm::vec3(0, 0, 1);
    plane.right = glm::vec3(1, 0, 0);
    plane.up = glm::vec3(0, 1, 0);
    plane.speed = 0.0f;

    planes.emplace_back(plane);

    return planes[plane.id];
}

Plane& Game::getNewPlaneWithCollider() {
    Plane& plane = getNewPlane();
    BoxCollider collider;
    collider.offset = glm::vec3();
    collider.size = glm::vec3(1.0f);
    collider.ownerId = plane.id;
    colliders.emplace_back(collider);
    return plane;
}

Plane& Game::getPlaneFromId(unsigned int id) {
    return planes[id];
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

void Game::render(float dt) {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // don't forget to enable shader before setting uniforms
    shader.use();

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    
    for (const Plane& plane : planes) {
        glm::mat4 rotMat(
            glm::vec4(plane.right, 0.0f),
            glm::vec4(plane.up, 0.0f),
            glm::vec4(plane.forward, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );

        glm::mat4 model = 
            glm::translate(glm::mat4(1.0f), plane.position) * 
            rotMat *
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));
        shader.setMat4("model", model);
        plane.model->Draw(shader);
    }

    // collider
    for (const BoxCollider& collider : colliders) {
        drawCollider(collider);
    }

    // skybox
    skyboxShader.use();
    glm::mat4 skyboxView = glm::mat4(1.0f);
    glm::mat4 skyboxProjection = glm::mat4(1.0f);
    skyboxView = glm::mat4(glm::mat3(glm::lookAt(camera.Position, camera.Position + camera.Forward, camera.Up)));
    skyboxProjection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
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

    const Plane& plane = getPlaneFromId(collider.ownerId);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), collider.offset + plane.position) * glm::scale(glm::mat4(1.0f), collider.size);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
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
    const Plane& p1 = getPlaneFromId(c1.ownerId);
    const Plane& p2 = getPlaneFromId(c2.ownerId);
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

void Game::yawPlane(Plane& plane, float deg) {
    glm::vec3 axis = glm::normalize(plane.up);
    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::vec4 forward = rotMat * glm::vec4(plane.forward, 1.0f);
    glm::vec4 up = rotMat * glm::vec4(plane.up, 1.0f);
    glm::vec4 right = rotMat * glm::vec4(plane.right, 1.0f);

    plane.forward = forward;
    plane.up = up;
    plane.right = right;
}

void Game::pitchPlane(Plane& plane, float deg) {
    glm::vec3 axis = glm::normalize(plane.right);
    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::vec4 forward = rotMat * glm::vec4(plane.forward, 1.0f);
    glm::vec4 up = rotMat * glm::vec4(plane.up, 1.0f);
    glm::vec4 right = rotMat * glm::vec4(plane.right, 1.0f);

    plane.forward = forward;
    plane.up = up;
    plane.right = right;
}

void Game::rollPlane(Plane& plane, float deg) {
    glm::vec3 axis = glm::normalize(plane.forward);
    float angle = glm::radians(deg);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::vec4 forward = rotMat * glm::vec4(plane.forward, 1.0f);
    glm::vec4 up = rotMat * glm::vec4(plane.up, 1.0f);
    glm::vec4 right = rotMat * glm::vec4(plane.right, 1.0f);

    plane.forward = forward;
    plane.up = up;
    plane.right = right;
}

void Game::updatePlayerPlaneCamera(Plane& playerPlane) {
    camera.Forward = playerPlane.forward;
    camera.Right = playerPlane.right;
    camera.WorldUp = playerPlane.up;
}