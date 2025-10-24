#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>

#include "Camera.h"
#include "Model.h"

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

const double PI = 3.14159265358979323846;

unsigned int getCubeMapTexture(std::string cubeMapPath[]);
void initSkybox();
void initColliderOutline();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, float dt);

void setShader(Shader* shader);
void setOutlineShader(Shader* shader);
void setSkyboxShader(Shader* shader);
void setModel1(Model* shader);
void setModel2(Model* shader);

void init();
void render(float dt);
void update(float dt);

void drawSkybox();

struct Plane {
	Model* model;
	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
	float speed;
};

struct BoxCollider {
	Plane* owner;
	glm::vec3 offset;
	glm::vec3 size;
};

bool isColliding(const BoxCollider& c1, const BoxCollider& c2);
void drawCollider(const BoxCollider& collider);

void yawPlane(Plane& plane, float deg);
void pitchPlane(Plane& plane, float deg);
void rollPlane(Plane& plane, float deg);
void updatePlayerPlaneCamera(Plane& playerPlane);