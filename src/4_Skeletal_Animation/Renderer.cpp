#include "Renderer.h"

Renderer::Renderer(std::string windowName, unsigned int width, unsigned int height): windowName(windowName), screenWidth(width), screenHeight(height), window(nullptr) {}

bool Renderer::init() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(screenWidth, screenHeight, windowName.c_str(), NULL, NULL);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
}

void Renderer::addShader(unsigned int id, Shader shader) {

}

void Renderer::addModel(unsigned int id, std::string filepath) {

}

void Renderer::addAnimation(unsigned int id, std::string filepath) {

}

void Renderer::addAnimator(unsigned int id, std::string filepath) {

}

void Renderer::setObjectToWorldTransform(unsigned int id, glm::mat4 t) {

}

void Renderer::render(std::vector<Object>& objects) {

}

void Renderer::renderCollider(std::vector<BoxCollider>& colliders) {

}

GLFWwindow* Renderer::getWindow() {
	return this->window;
}

Shader& Renderer::getShaderById() {

}

Model& Renderer::getModelById() {

}

Animation& Renderer::getAnimationById() {

}

Animator& Renderer::getAnimatorById() {

}

glm::mat4 Renderer::getObjectToWorldTransformById() const {

}