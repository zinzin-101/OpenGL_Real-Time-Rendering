#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <map>
#include <learnopengl/shader_m.h>
#include <learnopengl/model_animation.h>
#include "Animation.h"
#include "Animator.h"
#include "Game.h"
#include <string>

class Renderer {
	private:
		std::string windowName;
		unsigned int screenWidth;
		unsigned int screenHeight;

		GLFWwindow* window;
		std::map<unsigned int, Shader> idToShader;
		std::map<unsigned int, Model> idToModel;
		std::map<unsigned int, Animation> idToAnimation;
		std::map<unsigned int, Animator> idToAnimator;
		std::map<unsigned int, glm::mat4> idToWorldTransform;

	public:
		Renderer(std::string windowName, unsigned int width, unsigned int height);
		bool init();
		void addShader(unsigned int id, Shader shader);
		void addModel(unsigned int id, std::string filepath);
		void addAnimation(unsigned int id, std::string filepath);
		void addAnimator(unsigned int id, std::string filepath);
		void setObjectToWorldTransform(unsigned int id, glm::mat4 t);
		void render(std::vector<Object>& objects);
		void renderCollider(std::vector<BoxCollider>& colliders);
		GLFWwindow* getWindow();
		Shader& getShaderById();
		Model& getModelById();
		Animation& getAnimationById();
		Animator& getAnimatorById();
		glm::mat4 getObjectToWorldTransformById() const;
};