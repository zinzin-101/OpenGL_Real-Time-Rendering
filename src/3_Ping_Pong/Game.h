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
const glm::vec3 DEFAULT_GRAVITY = glm::vec3(0.0f, -20.0f, 0.0f);
const float MAX_PADDLE_BOUNCE_ANGLE = 65.0f;
const unsigned int PHYSICS_RESOLUTION = 3;

// Player settings
const float FOV = 60.0f;
const float FREE_CAM_MOVE_SPEED = 6.0f;
const float FREE_CAM_FAST_MOVE_SPEED = 24.0f;
const unsigned int NUM_CAM_TYPES = 4;
const float MAX_FOLLOW_CAM_DISTANCE = 40.0f;
const float MIN_FOLLOW_CAM_DISTANCE = 1.0f;
enum CameraType {
	STATIONARY = 0,
	FOLLOW_PADDLE,
	FOLLOW_BALL,
	FREE
};

struct Object {
	int id;
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

struct Physics {
	Physics(): ownerId(-1), lastPosition(0.0f), acceleration(0.0f) {}
	int ownerId;
	glm::vec3 lastPosition;
	glm::vec3 acceleration;
};

class Game {
	private:
		Shader shader;
		Shader outlineShader;
		Shader skyboxShader;
		Model paddleModel;
		Model tableModel;
		Model ballModel;
		Model floorModel;

		std::map<Model*, glm::mat4> modelToWorld;

		unsigned int cubeMapTexture;
		GLuint skyboxVAO, skyboxVBO, skyboxEBO;

		GLuint outlineVAO, outlineVBO, outlineEBO;

		std::vector<Object> objects;
		std::vector<BoxCollider> colliders;
		std::vector<Physics> physics;

		bool toggleGravity;
		bool togglePause;
		bool autoplay;

		int playerId;
		int ballId;
		int opponentId;

		Camera stationaryCamera;
		Camera behindCamera;
		Camera followCamera;
		Camera freeCamera;
		Camera* cameras[NUM_CAM_TYPES];
		CameraType currentCameraType;
		float cameraFollowDistance;
		void processFollowCamera(float xoffset, float yoffset, GLboolean constrainPitch = true);
		void updateFollowCamera();
		void updateBehindCamera();
		
		std::map<unsigned int, bool> keyDown;
		void initKeyDown();

		void initSkybox();
		void initColliderOutline();
		void init();

		Object& getNewObject();
		Object& getNewObjectWithCollider();
		Object& getObjectById(int id);
		std::vector<BoxCollider*> getCollidersById(int id);
		std::vector<Physics*> getPhysicsById(int id);

		glm::mat4 getProjection() const;

		void computeCollision();
		void handleCollision(BoxCollider& col1, BoxCollider& col2);

		void accelerate(Physics& phys, glm::vec3 a);
		void setVelocity(Physics& phys, glm::vec3 vel, float dt);
		void addVelocity(Physics& phys, glm::vec3 vel, float dt);
		glm::vec3 getVelocity(Physics& phys, float dt);
		void computePhysics(float dt);
		
		glm::vec3 getRotatedVector(glm::vec3 v, glm::vec3 axis, float deg);
		void rotateObject(Object& obj, glm::vec3 axis, float deg);

		void handleBallBounce(Object& ball, Object& wall, BoxCollider& wallCol);
		void handlePaddleBounce(Object& ball, Object& paddle);

		void reset(float dt);

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