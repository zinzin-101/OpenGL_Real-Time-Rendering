#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>

#include "Camera.h"
#include "Model.h"

#include <vector>

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

const double PI = 3.14159265358979323846;

// Game settings
const float DEFAULT_PLANE_SPEED = 100.0f;
const float DEFAULT_PITCH_RATE = 150.0f;
const float DEFAULT_YAW_RATE = 50.0f;
const float DEFAULT_ROLL_RATE = 200.0f;

// Player settings
const float CAM_DIST_FROM_PLANE = 50.0f;
const float MAX_FOV = 90.0f;
const float MIN_FOV = 60.0f;

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

class Game {
	private:
		Shader shader;
		Shader outlineShader;
		Shader skyboxShader;
		Model tempModel1;
		Model tempModel2;

		unsigned int cubeMapTexture;
		GLuint skyboxVAO, skyboxVBO, skyboxEBO;

		GLuint outlineVAO, outlineVBO;

		std::vector<BoxCollider> colliders;

		Camera& camera;

	public:
		Game(Camera& camera);
		unsigned int getCubeMapTexture(std::string cubeMapPath[]);
		void initSkybox();
		void initColliderOutline();

		void init();
		void render(float dt);
		void update(float dt);

		void drawSkybox();

		bool isColliding(const BoxCollider& c1, const BoxCollider& c2);
		void drawCollider(const BoxCollider& collider);

		void yawPlane(Plane& plane, float deg);
		void pitchPlane(Plane& plane, float deg);
		void rollPlane(Plane& plane, float deg);
		void updatePlayerPlaneCamera(Plane& playerPlane);
};