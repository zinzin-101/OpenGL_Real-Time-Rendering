#include "Game.h"

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Game settings
const float DEFAULT_PLANE_SPEED = 100.0f;
const float DEFAULT_PITCH_RATE = 150.0f;
const float DEFAULT_YAW_RATE = 50.0f;
const float DEFAULT_ROLL_RATE = 200.0f;

// Player settings
const float CAM_DIST_FROM_PLANE = 50.0f;
const float MAX_FOV = 90.0f;
const float MIN_FOV = 60.0f;

Shader* tempShader;
Model* tempModel1;
Model* tempModel2;

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window, float dt)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    glm::vec3 movement = glm::vec3();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += glm::vec3(0, 0, 5);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement += glm::vec3(0, 0, -5);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement += glm::vec3(-5, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += glm::vec3(5, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        movement += glm::vec3(0, 5, 0);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        movement += glm::vec3(0, -5, 0);
    camera.MyProcessKeyboard(movement, dt);
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
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

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
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void update(float dt) {

}

void render(float dt) {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // don't forget to enable shader before setting uniforms
    tempShader->use();

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    tempShader->setMat4("projection", projection);
    tempShader->setMat4("view", view);

    // render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));	// it's a bit too big for our scene, so scale it down
    tempShader->setMat4("model", model);
    tempModel1->Draw(*tempShader);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
    tempShader->setMat4("model", model);
    tempModel2->Draw(*tempShader);
}

void yawPlane(Plane& plane, float deg) {
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

void pitchPlane(Plane& plane, float deg) {
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

void rollPlane(Plane& plane, float deg) {
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

void updatePlayerPlaneCamera(Plane& playerPlane) {
    camera.Forward = playerPlane.forward;
    camera.Right = playerPlane.right;
    camera.WorldUp = playerPlane.up;
}