#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>

#include "Camera.h"
#include "Model.h"

#include <queue>
#include <map>
#include <vector>

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;
const int TARGET_FPS = 144;
const double MIN_TIME_PER_FRAME = 1.0 / (double)TARGET_FPS;

const double PI = 3.14159265358979323846;

// Game settings
const unsigned int WAVES_VERTS_WIDTH_NUM = 300;
const float WAVES_VERTS_SCALE = 0.25f;
const float WAVES_SPEED = 3.0f;
const float WAVES_AMPLITUDE = 0.5f;
//const float WAVES_LENGTH = 0.25f;
const glm::vec3 WAVES_DIRECTION[4] = { glm::vec3(1.0f, 0.0f, 2.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, -1.0f) };
const float WAVES_LENGTH = 4.0f;

// Player settings
const float FOV = 60;
const float FREE_CAM_FAST_MOVE_SPEED = 50;
const float FREE_CAM_MOVE_SPEED = 10;

//struct Object {
//	int id;
//	std::string name;
//	Model* model;
//	glm::vec3 position;
//	glm::vec3 forward;
//	glm::vec3 right;
//	glm::vec3 up;
//};

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

//struct RenderingObject {
//	RenderingObject(const Object* object, float distanceFromCamera): object(object), distanceFromCamera(distanceFromCamera) {}
//	const Object* object;
//	float distanceFromCamera;
//};

//struct RenderComparator {
//	bool operator()(const RenderingObject& obj1, const RenderingObject& obj2);
//};

class Game {
	private:
		Shader wavesShader;
		Shader outlineShader;
		Shader skyboxShader;

		//std::map<Model*, glm::mat4> modelToWorld;

		unsigned int cubeMapTexture;
		GLuint skyboxVAO, skyboxVBO, skyboxEBO;
		GLuint outlineVAO, outlineVBO, outlineEBO;

		unsigned int wavesStripCount, wavesVertsPerStrip;
		GLuint wavesVAO, wavesVBO, wavesEBO;
		float wavesTime;

		//std::vector<Object> objects;
		//std::vector<BoxCollider> colliders;
		//std::vector<Physics> physics;

		//std::priority_queue<RenderingObject, std::vector<RenderingObject>, RenderComparator> opacityRenderQueue;

		float dt;

		bool showCollider;

		//int playerId;

		Camera camera;
		
		std::map<unsigned int, bool> keyDown;
		bool handleKeyDown(GLFWwindow* window, unsigned int key);

		void initSkybox();
		void drawSkybox();

		void initWaves();
		void drawWaves();

		void initColliderOutline();
		void init();
		void setup();

		//Object& getNewObject();
		//Object& getNewObjectWithCollider();
		//Object& getObjectById(int id);
		//std::vector<BoxCollider*> getCollidersById(int id);
		//std::vector<Physics*> getPhysicsById(int id);


		//bool isColliding(const BoxCollider& c1, const BoxCollider& c2);
		//void drawCollider(const BoxCollider& collider);

		glm::mat4 getProjection() const;

		void accelerate(Physics& phys, glm::vec3 a);
		void setVelocity(Physics& phys, glm::vec3 vel, float dt);
		void addVelocity(Physics& phys, glm::vec3 vel, float dt);
		glm::vec3 getVelocity(Physics& phys, float dt);
		void computePhysics(float dt);
		
		//glm::vec3 getRotatedVector(glm::vec3 v, glm::vec3 axis, float deg);
		//void rotateObject(Object& obj, glm::vec3 axis, float deg);

		//void rotateEveryThing(glm::vec3 axis, float deg);
		//void rotatePlayerPaddle(glm::vec3 axis, float deg);

		//void handleBallBounce(Object& ball, Object& wall, BoxCollider& wallCol);
		//void handlePaddleBounce(Object& ball, Object& paddle);

	public:
		Game();
		unsigned int getCubeMapTexture(std::string cubeMapPath[]);
		
		void render(float dt);
		void update(float dt);

		void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
		void processMouseScroll(float yoffset);
		void processMouseButton(int button, int action);
		void processKeyboard(GLFWwindow* window, float dt);
};