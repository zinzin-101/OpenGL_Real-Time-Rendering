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

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


enum AnimState {
	IDLE = 1,
	IDLE_PUNCH,
	PUNCH_IDLE,
	IDLE_KICK,
	KICK_IDLE,
	IDLE_WALK,
	WALK_IDLE,
	WALK
};
unsigned int cubeMapTexture;
GLuint skyboxVAO, skyboxVBO, skyboxEBO;
GLuint cubeVAO, cubeVBO, cubeEBO;

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
	Shader ourShader("anim_model.vs", "anim_model.fs");
	Shader modelShader("model.vs", "model.fs");

	
	// load models
	// -----------
	// idle 3.3, walk 2.06, run 0.83, punch 1.03, kick 1.6
	Model ourModel(FileSystem::getPath("resources/objects/mixamo/Knight D Pelegrini.dae"));
	Animation idleAnimation(FileSystem::getPath("resources/objects/mixamo/WarriorIdle.dae"),&ourModel);
	Animation walkAnimation(FileSystem::getPath("resources/objects/mixamo/Walking.dae"), &ourModel);
	Animation runAnimation(FileSystem::getPath("resources/objects/mixamo/Running.dae"), &ourModel);
	Animation punchAnimation(FileSystem::getPath("resources/objects/mixamo/Boxing.dae"), &ourModel);
	Animation kickAnimation(FileSystem::getPath("resources/objects/mixamo/HurricaneKick.dae"), &ourModel);
	Animator animator(&idleAnimation);
	enum AnimState charState = IDLE;
	float blendAmount = 0.0f;
	float blendRate = 0.055f;

	//stbi_set_flip_vertically_on_load(false);

	Model boatModel(FileSystem::getPath("resources/objects/boat/boat.dae"));
	glm::mat4 boatToWorld =
		glm::translate(glm::mat4(1.0f), glm::vec3(0,0,22)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(0.015f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 groundToWorld = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 1.0f, 50.0f));

	//stbi_set_flip_vertically_on_load(true);

	initSkybox();
	initCube();
	Shader skyboxShader("skybox.vs", "skybox.fs");

	std::map<unsigned int, bool> keyDown;
	keyDown[GLFW_KEY_1] = false;
	keyDown[GLFW_KEY_2] = false;
	keyDown[GLFW_KEY_3] = false;
	keyDown[GLFW_KEY_4] = false;

	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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
			animator.PlayAnimation(&punchAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (keyDownHandler(GLFW_KEY_5, window, keyDown))
			animator.PlayAnimation(&kickAnimation, NULL, 0.0f, 0.0f, 0.0f);


		switch (charState) {
			case IDLE:
				if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
					blendAmount = 0.0f;
					animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
					charState = IDLE_WALK;
				} else if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
					blendAmount = 0.0f;
					animator.PlayAnimation(&idleAnimation, &punchAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
					charState = IDLE_PUNCH;
				}
				else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
					blendAmount = 0.0f;
					animator.PlayAnimation(&idleAnimation, &kickAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
					charState = IDLE_KICK;
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
				if (glfwGetKey(window, GLFW_KEY_UP) != GLFW_PRESS) {
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
			case IDLE_PUNCH:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&idleAnimation, &punchAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&punchAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = PUNCH_IDLE;
				}
				printf("idle_punch\n");
				break;
			case PUNCH_IDLE:
				if (animator.m_CurrentTime > 0.7f) {
					blendAmount += blendRate;
					blendAmount = fmod(blendAmount, 1.0f);
					animator.PlayAnimation(&punchAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
					if (blendAmount > 0.9f) {
						blendAmount = 0.0f;
						float startTime = animator.m_CurrentTime2;
						animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
						charState = IDLE;
					}
					printf("punch_idle \n");
				}
				else {
					// punching
					printf("punching \n");
				}
				break;
			case IDLE_KICK:
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&idleAnimation, &kickAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&kickAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = KICK_IDLE;
				}
				printf("idle_kick\n");
				break;
			case KICK_IDLE:
				if (animator.m_CurrentTime > 1.0f) {
					blendAmount += blendRate;
					blendAmount = fmod(blendAmount, 1.0f);
					animator.PlayAnimation(&kickAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
					if (blendAmount > 0.9f) {
						blendAmount = 0.0f;
						float startTime = animator.m_CurrentTime2;
						animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
						charState = IDLE;
					}
					printf("kick_idle \n");
				}
				else {
					// punching
					printf("kicking \n");
				}
				break;
		}



		animator.UpdateAnimation(deltaTime);
		
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

		boatModel.Draw(modelShader);
		modelShader.setMat4("model", glm::mat4(1.0f) * groundToWorld);
		drawCube();

		// don't forget to enable shader before setting uniforms
		ourShader.use();

		// view/projection transformations
		glm::mat4 projection = getPerspective();
		glm::mat4 view = camera.GetViewMatrix();
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

        auto transforms = animator.GetFinalBoneMatrices();
		for (int i = 0; i < transforms.size(); ++i)
			ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);


		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.6f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(1.f, 1.f, 1.f));	// it's a bit too big for our scene, so scale it down
		ourShader.setMat4("model", model);
		ourModel.Draw(ourShader);


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
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		movement += glm::vec3(0, 1, 0);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		movement += glm::vec3(0, -1, 0);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		movement *= 20.0f;

	camera.ProcessKeyboard(movement * camera.MovementSpeed, deltaTime);
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
