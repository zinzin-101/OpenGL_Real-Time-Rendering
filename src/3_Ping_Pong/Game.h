#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>

#include "Camera.h"
#include "Model.h"

#include <map>
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
const float FOV = 60.0f;
const unsigned int NUM_CAM_TYPES = 3;
const float MAX_FOLLOW_CAM_DISTANCE = 10.0f;
const float MIN_FOLLOW_CAM_DISTANCE = 1.0f;
enum CameraType {
	STATIONARY = 0,
	FOLLOW,
	FREE
};

struct Object {
	unsigned int id;
	std::string name;
	Model* model;
	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
};

struct BoxCollider {
	BoxCollider(): ownerId(-1), offset(0.0f), size(1.0f) {}
	int ownerId;
	glm::vec3 offset;
	glm::vec3 size;
};

class Game {
	private:
		Shader shader;
		Shader outlineShader;
		Shader skyboxShader;
		Model paddleModel;
		Model tableModel;
		Model ballModel;

		std::map<Model*, glm::mat4> modelToWorld;

		unsigned int cubeMapTexture;
		GLuint skyboxVAO, skyboxVBO, skyboxEBO;

		GLuint outlineVAO, outlineVBO, outlineEBO;

		std::vector<Object> objects;
		std::vector<BoxCollider> colliders;

		int playerId;
		int ballId;

		Camera stationaryCamera;
		Camera followCamera;
		Camera freeCamera;
		Camera* cameras[NUM_CAM_TYPES];
		CameraType currentCameraType;
		float cameraFollowDistance;
		void processFollowCamera(float xoffset, float yoffset, GLboolean constrainPitch = true);
		void updateFollowCamera();
		
		std::map<unsigned int, bool> keyDebounce;
		void initKeyDebounce();

		void initSkybox();
		void initColliderOutline();
		void init();

		Object& getNewObject();
		Object& getNewObjectWithCollider();
		Object& getObjectFromId(unsigned int id);

		glm::mat4 getProjection() const;

		void checkCollision();
		void handleCollision(const BoxCollider& col1, const BoxCollider& col2);

	public:
		Game();
		unsigned int getCubeMapTexture(std::string cubeMapPath[]);
		
		void render(float dt);
		void update(float dt);

		void drawSkybox();

		bool isColliding(const BoxCollider& c1, const BoxCollider& c2);
		void drawCollider(const BoxCollider& collider);

		void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
		void processMouseScroll(float yoffset);
		void processKeyboard(GLFWwindow* window, float dt);
};