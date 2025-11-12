#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include "Camera.h"
#include "Animator.h"
#include <learnopengl/model_animation.h>
#include "VerticesData.h"

#include <iostream>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;
const unsigned int TARGET_FRAME_RATE = 60;

// camera
Camera camera(glm::vec3(0.0f, 5.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float camFollowDistance = 5.0f;
float camHeight = 1.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


enum AnimState {
	IDLE = 1,
	IDLE_GRAB,
	GRAB_IDLE,
	IDLE_RUN,
	RUN_IDLE,
	IDLE_WALK,
	RUN,
	RUN_WALK,
	WALK_RUN,
	WALK_IDLE,
	WALK
};
unsigned int cubeMapTexture;
GLuint skyboxVAO, skyboxVBO, skyboxEBO;
GLuint cubeVAO, cubeVBO, cubeEBO;
GLuint outlineVAO, outlineVBO, outlineEBO;

struct BoxCollider {
	BoxCollider() : ownerPosition(nullptr), offset(0.0f), size(1.0f) {}
	glm::vec3* ownerPosition;
	glm::vec3 offset;
	glm::vec3 size;
};

enum class MovementState {
	IDLE,
	WALKING,
	SPRINTING
};

// player
MovementState currentMovementState = MovementState::IDLE;
float walkSpeed = 5.0f;
float sprintSpeed = walkSpeed * 4.0f;
glm::vec3 playerPos = glm::vec3(0, 0.5f, 0);
glm::vec3 lastPlayerPos = playerPos;
glm::vec3 playerForward = glm::vec3(0, 0, 1);
glm::vec3 playerRight = glm::vec3(1, 0, 0);
glm::vec3 playerUp = glm::vec3(0, 1, 0);
float playerLerpRate = glm::radians(540.0f);
glm::vec3 playerCurrentForward = playerForward;
glm::vec3 playerCurrentRight = playerRight;
glm::vec3 playerCurrentUp = playerUp;
bool isSwordInHand = false;

void initCube() {
	// bind VAO
	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);

	// generate VBO
	glGenBuffers(1, &cubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(
		GL_ARRAY_BUFFER,
		8 * sizeof(float) * 6,
		CUBE_VERTICES,
		GL_STATIC_DRAW
	);

	// positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// normals
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// generate EBO
	glGenBuffers(1, &cubeEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		36 * sizeof(unsigned int),
		SKYBOX_INDICES,
		GL_STATIC_DRAW
	);
}

void initColliderOutline() {
	glGenVertexArrays(1, &outlineVAO);
	glGenBuffers(1, &outlineVBO);
	glGenBuffers(1, &outlineEBO);

	glBindVertexArray(outlineVAO);

	glBindBuffer(GL_ARRAY_BUFFER, outlineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_OUTLINE_VERTICES), CUBE_OUTLINE_VERTICES, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outlineEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

void drawCube() {
	glBindVertexArray(cubeVAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

unsigned int getCubeMapTexture(std::string cubeMapPath[]) {
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

void initSkybox() {
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

void drawSkybox() {
	glDepthFunc(GL_LEQUAL);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}

bool keyDownHandler(unsigned int key, GLFWwindow* window, std::map<unsigned int, bool>& keyDown) {
	if (glfwGetKey(window, key) == GLFW_PRESS && !keyDown.at(key)) {
		keyDown[key] = true;
		return true;
	}
	else if (glfwGetKey(window, key) == GLFW_RELEASE) keyDown[key] = false;
	return false;
}

glm::mat4 getPerspective() {
	return glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
}

glm::vec3 rotateVector(glm::vec3 v, glm::vec3 axis, float deg) {
	float angle = glm::radians(deg);
	glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, axis);
	return rotMat * glm::vec4(v, 1.0f);
}

glm::vec3 rotateTowards(glm::vec3 current, glm::vec3 target, float maxRadiansDelta) {
	current = glm::normalize(current);
	target = glm::normalize(target);

	float cos = glm::dot(current, target);
	cos = glm::clamp(cos, -1.0f, 1.0f);

	float angle = glm::acos(cos);

	if (angle < 1e-5f || angle <= maxRadiansDelta) return target;

	glm::vec3 axis = glm::normalize(glm::cross(current, target));
	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), maxRadiansDelta, axis);

	return glm::normalize(glm::vec3(rot * glm::vec4(current, 0.0f)));
}

void handlePlayerLerp(float dt) {
	float cos = glm::clamp(glm::dot(playerCurrentForward, playerForward), -1.0f, 1.0f);
	float angle = glm::acos(cos);

	if (angle < 1e-5f) {
		playerCurrentForward = playerForward;
	}
	else {
		float rate = playerLerpRate * (currentMovementState == MovementState::SPRINTING ? 1.5f : 1.0f) * dt;
		float step = glm::min(rate, angle);
		float sign = (glm::cross(playerCurrentForward, playerForward).y >= 0.0f) ? 1.0f : -1.0f;

		// Rotate current forward around Y by 'step'
		glm::mat4 rot = glm::rotate(glm::mat4(1.0f), step * sign, glm::vec3(0, 1, 0));
		playerCurrentForward = glm::normalize(glm::vec3(rot * glm::vec4(playerCurrentForward, 0.0f)));
	}
	
	playerCurrentRight = glm::normalize(glm::cross(playerCurrentForward, glm::vec3(0, 1, 0)));
	playerCurrentUp = glm::normalize(glm::vec3(0, 1, 0));
}

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader animShader("anim_model.vs", "anim_model.fs");
	Shader modelShader("model.vs", "model.fs");

	
	// load models
	// -----------
	// idle 3.3, walk 2.06, run 0.83, punch 1.03, kick 1.6
	Model playerModel(FileSystem::getPath("resources/objects/mixamo/Knight D Pelegrini.dae"));
	Animation idleAnimation(FileSystem::getPath("resources/objects/mixamo/WarriorIdle.dae"),&playerModel);
	Animation walkAnimation(FileSystem::getPath("resources/objects/mixamo/Walking.dae"), &playerModel);
	Animation runAnimation(FileSystem::getPath("resources/objects/mixamo/Running.dae"), &playerModel);
	Animation grabAnimation(FileSystem::getPath("resources/objects/mixamo/Grab.dae"), &playerModel);
	Animation punchAnimation(FileSystem::getPath("resources/objects/mixamo/Boxing.dae"), &playerModel);
	Animator animator(&idleAnimation);
	enum AnimState charState = IDLE;
	float blendAmount = 0.0f;
	float blendRate = 0.055f;

	Model swordModel(FileSystem::getPath("resources/objects/sword/sword.obj"));

	//float blendRate = 0.8f;

	//stbi_set_flip_vertically_on_load(false);

	//Model boatModel(FileSystem::getPath("resources/objects/boat/boat.dae"));
	glm::mat4 boatToWorld =
		glm::translate(glm::mat4(1.0f), glm::vec3(0,0,22)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(0.015f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 groundToWorld = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 1.0f, 50.0f));

	glm::mat4 cubeToWorld = glm::mat4(1.0f);
	float swordSize = 0.05f;
	glm::mat4 swordToWorld =
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1)) * 
		glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0, 0));

	glm::mat4 swordHandOffset =
		glm::rotate(glm::mat4(1.0), glm::radians(-30.0f), glm::vec3(0, 0, 1)) *
		glm::translate(glm::mat4(1.0f), glm::vec3(17, 5, 5)) *
		glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(1, 0, 0)) *
		glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(0, 0, 1)) *
		glm::translate(glm::mat4(1.0f), glm::vec3(0, 70, 0));

	//stbi_set_flip_vertically_on_load(true);

	initSkybox();
	initCube();
	Shader skyboxShader("skybox.vs", "skybox.fs");

	std::map<unsigned int, bool> keyDown;
	keyDown[GLFW_KEY_1] = false;
	keyDown[GLFW_KEY_2] = false;
	keyDown[GLFW_KEY_3] = false;
	keyDown[GLFW_KEY_4] = false;
	keyDown[GLFW_KEY_E] = false;

	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//std::vector<std::string> animName = animator.GetAllNodeNames();
	//for (std::string s : animName)
	//	printf("%s\n", s.c_str());

	std::string rightHandNodeName = "mixamorig_LeftHand";

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;

		if (deltaTime < (1.0f / (float)TARGET_FRAME_RATE)) continue;

		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);
		if (keyDownHandler(GLFW_KEY_1, window, keyDown))
			animator.PlayAnimation(&idleAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (keyDownHandler(GLFW_KEY_2, window, keyDown))
			animator.PlayAnimation(&walkAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (keyDownHandler(GLFW_KEY_3, window, keyDown))
			animator.PlayAnimation(&runAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (keyDownHandler(GLFW_KEY_4, window, keyDown))
			animator.PlayAnimation(&grabAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (keyDownHandler(GLFW_KEY_5, window, keyDown))
			animator.PlayAnimation(&punchAnimation, NULL, 0.0f, 0.0f, 0.0f);

		if (keyDownHandler(GLFW_KEY_F, window, keyDown))
			isSwordInHand = !isSwordInHand;

		//printf("playerspeed: %f\n", playerSpeed);
		//printf("current state %i\n", charState);
		switch (charState) {
			case IDLE:
				if (currentMovementState == MovementState::WALKING) {
					blendAmount = 0.0f;
					animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
					charState = IDLE_WALK;
				} 
				else if (keyDownHandler(GLFW_KEY_E, window, keyDown)) {
					blendAmount = 0.0f;
					animator.PlayAnimation(&idleAnimation, &grabAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
					charState = IDLE_GRAB;
				}
				else if (currentMovementState == MovementState::SPRINTING) {
					blendAmount = 0.0f;
					animator.PlayAnimation(&idleAnimation, &runAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
					charState = IDLE_RUN;
				}
				printf("idle \n");
				break;
			case IDLE_WALK:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime,animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&walkAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = WALK;
				}
				printf("idle_walk \n");
				break;
			case WALK:
				animator.PlayAnimation(&walkAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (currentMovementState == MovementState::SPRINTING) {
					charState = WALK_RUN;
				} 
				else if (currentMovementState == MovementState::IDLE) {
					charState = WALK_IDLE;
				}
				printf("walking\n");
				break;
			case WALK_IDLE:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&walkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				printf("walk_idle \n");
				break;
			case IDLE_GRAB:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&idleAnimation, &grabAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&grabAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = GRAB_IDLE;
				}
				printf("idle_grab\n");
				break;
			case GRAB_IDLE:
				if (animator.m_CurrentTime > 2.5f) {
					blendAmount += blendRate;
					blendAmount = fmod(blendAmount, 1.0f);
					animator.PlayAnimation(&grabAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
					if (blendAmount > 0.9f) {
						blendAmount = 0.0f;
						float startTime = animator.m_CurrentTime2;
						animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
						charState = IDLE;
					}
					printf("grab_idle \n");
				}
				else {
					printf("grabbing\n");
				}
				break;
			case IDLE_RUN:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&idleAnimation, &runAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&runAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = RUN;
				}
				printf("idle_run\n");
				break;
			case RUN:
				animator.PlayAnimation(&runAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (currentMovementState == MovementState::WALKING) {
					charState = RUN_WALK;
				}
				else if (currentMovementState == MovementState::IDLE) {
					charState = RUN_IDLE;
				}
				break;

			case RUN_WALK:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&runAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&walkAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = WALK;
				}
				printf("run_idle \n");
				break;

			case WALK_RUN:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&walkAnimation, &runAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&runAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = RUN;
				}
				printf("walk_run \n");
				break;

			case RUN_IDLE:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&runAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				printf("run_idle \n");

				break;
		}

		animator.UpdateAnimation(deltaTime);
		
		// player position update
		lastPlayerPos = playerPos;

		// camera
		camera.Position = playerPos - (glm::normalize(camera.Forward) * camFollowDistance) + (glm::normalize(camera.Up) * camHeight);

		// player lerp
		handlePlayerLerp(deltaTime);

		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// skybox
		skyboxShader.use();
		glm::mat4 skyboxView = glm::mat4(1.0f);
		glm::mat4 skyboxProjection = glm::mat4(1.0f);
		//skyboxView = glm::mat4(glm::mat3(glm::lookAt(camera.Position, camera.Position + camera.Forward, camera.Up)));
		skyboxView = glm::mat4((glm::mat3)camera.GetViewMatrix());
		skyboxProjection = getPerspective();
		skyboxShader.setMat4("view", skyboxView);
		skyboxShader.setMat4("projection", skyboxProjection);
		drawSkybox();

		modelShader.use();
		modelShader.setVec3("viewPos", camera.Position);
		modelShader.setFloat("shininess", 32.0f);
		modelShader.setMat4("projection", getPerspective());
		modelShader.setMat4("view", camera.GetViewMatrix());
		glm::mat4 boatModelMat = glm::mat4(1.0f) * boatToWorld;
		modelShader.setMat4("model", boatModelMat);
		modelShader.setVec3("color", glm::vec3(0.6f));

		modelShader.setVec3("pointLights[0].ambient", glm::vec3(0.9f));
		modelShader.setVec3("pointLights[0].diffuse", glm::vec3(0.4f));
		modelShader.setVec3("pointLights[0].specular", glm::vec3(0.2f));
		modelShader.setVec3("pointLights[0].position", glm::vec3(0, 10, 0));
		modelShader.setFloat("pointLights[0].constant", 1.0f);
		modelShader.setFloat("pointLights[0].linear", 0.00009f);
		modelShader.setFloat("pointLights[0].quadratic", 0.000032f);

		//boatModel.Draw(modelShader);
		modelShader.setMat4("model", glm::mat4(1.0f) * groundToWorld);
		modelShader.setBool("useColor", true);
		modelShader.setVec3("color", glm::vec3(0.5f, 0.5f, 0.5f));
		drawCube();

		// don't forget to enable shader before setting uniforms
		animShader.use();

		// view/projection transformations
		glm::mat4 projection = getPerspective();
		glm::mat4 view = camera.GetViewMatrix();
		animShader.setMat4("projection", projection);
		animShader.setMat4("view", view);

        auto transforms = animator.GetFinalBoneMatrices();
		for (int i = 0; i < transforms.size(); ++i)
			animShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);


		// render the loaded model
		glm::mat4 playerModelMat = glm::mat4(1.0f);
		playerModelMat = glm::scale(playerModelMat, glm::vec3(1.f, 1.f, 1.f));	// it's a bit too big for our scene, so scale it down
		glm::mat4 rotMat(
			glm::vec4(playerCurrentRight, 0.0f),
			glm::vec4(playerCurrentUp, 0.0f),
			glm::vec4(playerCurrentForward, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);
		playerModelMat = glm::translate(glm::mat4(1.0f), playerPos) * rotMat * playerModelMat;
		animShader.setMat4("model", playerModelMat);
		playerModel.Draw(animShader);


		modelShader.use();
		modelShader.setBool("useColor", false);
		modelShader.setVec3("color", glm::vec3(1, 0, 0));
		glm::mat4 toHand = animator.m_BoneGlobalTransform.at(rightHandNodeName);
		glm::vec3 handScale;
		handScale.x = glm::length(glm::vec3(toHand[0]));
		handScale.y = glm::length(glm::vec3(toHand[1]));
		handScale.z = glm::length(glm::vec3(toHand[2]));
		glm::vec3 swordScale = glm::vec3(swordSize) / handScale;
		
		if (isSwordInHand) modelShader.setMat4("model", playerModelMat * toHand * swordHandOffset * swordToWorld * glm::scale(glm::mat4(1.0f), glm::vec3(swordScale)));
		else modelShader.setMat4("model", swordToWorld * glm::scale(glm::mat4(1.0f), glm::vec3(swordSize)));
		swordModel.Draw(modelShader);
		//drawCube();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	glm::vec3 movement(0.0f);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		movement += glm::vec3(0, 0, 1);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		movement += glm::vec3(0, 0, -1);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		movement += glm::vec3(-1, 0, 0);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		movement += glm::vec3(1, 0, 0);
	//if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	//	movement += glm::vec3(0, 1, 0);
	//if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	//	movement += glm::vec3(0, -1, 0);
	//camera.ProcessKeyboard(movement * camera.MovementSpeed, deltaTime);

	if (glm::length(movement) > 0.0f) {
		movement = glm::normalize(movement);
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) currentMovementState = MovementState::SPRINTING;
		else currentMovementState = MovementState::WALKING;

		glm::vec3 forward = camera.Forward * movement.z + camera.Right * movement.x;
		forward.y = 0.0f;
		forward = glm::normalize(forward);
		playerForward = forward;
		playerRight = glm::cross(forward, glm::vec3(0,1,0));
		playerPos += (currentMovementState == MovementState::WALKING ? walkSpeed : sprintSpeed) * forward * deltaTime;
	}
	else {
		currentMovementState = MovementState::IDLE;
	}

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
