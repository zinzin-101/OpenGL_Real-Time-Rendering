#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <thread>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource = "#version 330 core\n"
    "uniform float scale;\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x * scale, aPos.y * scale, aPos.z, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "uniform vec3 color;\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(color.r, color.g, color.b, 1.0f);\n"
    "}\n\0";


double elapsedTime;
double lastElapsedTime;
const int TARGET_FPS = 60;
const double MIN_TIME_PER_FRAME = 1.0 / (double)TARGET_FPS;
const int COMPUTE_RESOLUTION = 4;
int windowPosX;
int windowPosY;
float scale = 0.01f;
float color_t = 0.0f;

const float BORDER_WIDTH = 100.0f;
const float BORDER_HEIGHT = 100.0f;

unsigned int shaderProgram;
unsigned int VBO, VAO;
GLint scaleUniformId;
GLint colorUniformId;

struct Vec3 {
    Vec3(): x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z){}
    float x, y, z;
    Vec3 operator*(const float& x);
    void operator*=(const float& x);
    void operator+=(const Vec3& other);
    void operator-=(const Vec3& other);
};

Vec3 Vec3::operator*(const float& x) {
    return Vec3(
        this->x * x,
        this->y * x,
        this->z * x
    );
}

void Vec3::operator*=(const float& x) {
    this->x *= x;
    this->y *= x;
    this->z *= x;
}

void Vec3::operator+=(const Vec3 & other) {
    this->x += other.x;
    this->y += other.y;
    this->z += other.z;
}

void Vec3::operator-=(const Vec3& other) {
    this->x -= other.x;
    this->y -= other.y;
    this->z -= other.z;
}

Vec3 operator+(const Vec3& v1, const Vec3& v2) {
    return Vec3(
        v1.x + v2.x,
        v1.y + v2.y,
        v1.z + v2.z
    );
}

Vec3 operator-(const Vec3& v1, const Vec3& v2) {
    return Vec3(
        v1.x - v2.x,
        v1.y - v2.y,
        v1.z - v2.z
    );
}

Vec3 operator*(const float& x, const Vec3& v) {
    return Vec3(
        v.x * x,
        v.y * x,
        v.z * x
    );
}

Vec3 operator/(const Vec3& v, const float& x) {
    return Vec3(
        v.x / x,
        v.y / x,
        v.z / x
    );
}

float getMagnitude(const Vec3& v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float deg2Rad(float deg) {
    return (deg * 3.14159f) / 180.0f;
}

inline float rad2Deg(float rad) {
    return (rad * 180.0f) / 3.14159f;
}

struct Ball {
    Ball(): position(Vec3()), lastPosition(Vec3()), acceleration(Vec3()), radius(1.0f) {}
    Vec3 position;
    Vec3 lastPosition;
    Vec3 acceleration;
    float radius;

    void update(float dt);
    void accelerate(Vec3 a);
    void setVelocity(Vec3 vel, float dt);
    void addVelocity(Vec3 vel, float dt);
    Vec3 getVelocity(float dt) const;
};

std::vector<Ball> balls;

void Ball::update(float dt) {
    Vec3 displacement = position - lastPosition;
    lastPosition = position;
    position = position + displacement + acceleration * dt * dt;

    acceleration = Vec3();
}

void Ball::accelerate(Vec3 a) {
    acceleration += a;
}

void Ball::setVelocity(Vec3 vel, float dt) {
    lastPosition = position - (vel * dt);
}

void Ball::addVelocity(Vec3 vel, float dt) {
    lastPosition -= vel * dt;
}

Vec3 Ball::getVelocity(float dt) const {
    return (position - lastPosition) / dt;
}

void applyGravity(float dt) {
    for (Ball& ball : balls) {
        ball.accelerate(Vec3(0, -9.81f, 0));
    }
}

void computeCollision(float dt) {
    const float responseCoeff = 0.25f;
    int numOfBalls = balls.size();
    for (int i = 0; i < numOfBalls; i++) {
        Ball& b1 = balls[i];
        for (int j = i + 1; j < numOfBalls; j++) {
            Ball& b2 = balls[j];

            Vec3 vec = b1.position - b2.position;
            float dist2 = vec.x * vec.x + vec.y * vec.y;
            float minDist = b1.radius + b2.radius;

            if (dist2 > minDist * minDist) {
                continue;
            }

            float dist = sqrtf(dist2);
            Vec3 normalVec = vec / dist;
            float delta = 0.5f * responseCoeff * (dist - minDist);
            b1.position -= normalVec * 0.5f * delta;
            b2.position += normalVec * 0.5f * delta;
        }
    }
}

void applyConstraint(float dt) {
    for (Ball& ball : balls) {
        if (ball.position.x > BORDER_WIDTH - ball.radius) {
            ball.lastPosition.x = ball.position.x;
            ball.position.x = BORDER_WIDTH - ball.radius;
        }

        if (ball.position.x < -BORDER_WIDTH + ball.radius) {
            ball.lastPosition.x = ball.position.x;
            ball.position.x = -BORDER_WIDTH + ball.radius;
        }

        if (ball.position.y > BORDER_HEIGHT - ball.radius) {
            ball.lastPosition.y = ball.position.y;
            ball.position.y = BORDER_HEIGHT - ball.radius;
        }

        if (ball.position.y < -BORDER_HEIGHT + ball.radius) {
            ball.lastPosition.y = ball.position.y;
            ball.position.y = -BORDER_HEIGHT+ ball.radius;
        }
    }
}

void updateBalls(float dt) {
    for (int i = 0; i < COMPUTE_RESOLUTION; i++) {
        //applyGravity(dt);
        computeCollision(dt);
        applyConstraint(dt);

        for (Ball& ball : balls) {
            ball.update(dt);
        }
    }
}

const unsigned int CircleVertsNum = 362;
float circleVerts[CircleVertsNum * 3];

void initCircleVerts() {
    circleVerts[0] = 0.0f;
    circleVerts[1] = 0.0f;
    circleVerts[2] = 0.0f;
    for (int i = 1; i <= 361; i++) {
        circleVerts[i * 3] = cos(deg2Rad(i));
        circleVerts[i * 3 + 1] = sin(deg2Rad(i));;
        circleVerts[i * 3 + 2] = 0.0f;
    }
}

float* getTranslatedCircleVerts(const Vec3& v, float scale = 1.0f) {
    float* verts = new float[CircleVertsNum * 3];
    for (int i = 0; i < CircleVertsNum; i++) {
        verts[i * 3 + 0] = (scale * circleVerts[i * 3 + 0]) + v.x;
        verts[i * 3 + 1] = (scale * circleVerts[i * 3 + 1]) + v.y;
        verts[i * 3 + 2] = (scale * circleVerts[i * 3 + 2]) + v.z;
    }
    return verts;
}

Vec3 getRainbow(float t)
{
    const float r = sin(t);
    const float g = sin(t + 0.33f * 2.0f * 3.14159f);
    const float b = sin(t + 0.66f * 2.0f * 3.14159f);
    return Vec3(
        r * r,
        g * g,
        b * b
    );
}

void drawCircle(Vec3 position, float radius) {
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    float* verts = getTranslatedCircleVerts(position, radius);
    glUniform1f(scaleUniformId, scale);

    Vec3 color = getRainbow(color_t);
    glUniform3f(colorUniformId, color.x, color.y , color.z);

    glBufferData(GL_ARRAY_BUFFER, sizeof(circleVerts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // draw our first triangle
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLE_FAN, 0, 362);
    // glBindVertexArray(0); // no need to unbind it every time

    delete[] verts;
}

void updateBallColor(float dt) {
    float totalSpeed = 0.0f;
    for (const Ball& ball : balls) {
        totalSpeed += getMagnitude(ball.getVelocity(dt));
    }

    int numOfBalls = balls.size();
    float averageSpeed = totalSpeed / (float)numOfBalls;

    color_t +=  0.01f * averageSpeed * dt;
}

void renderBalls(GLFWwindow* window) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const Ball& ball : balls) {
        drawCircle(ball.position, ball.radius);
    }

    glfwSwapBuffers(window);
}

void update(GLFWwindow* window, float dt) {
    //std::cout << (1.0f / dt) << std::endl;
    updateBalls(dt);
    updateBallColor(dt);
    renderBalls(window);
}

void computeWindowMovement(GLFWwindow* window, float dt) {
    int lastX = windowPosX;
    int lastY = windowPosY;
    glfwGetWindowPos(window, &windowPosX, &windowPosY);
    int currentX = windowPosX;
    int currentY = windowPosY;

    Vec3 movement(
        currentX - lastX,
        currentY - lastY,
        0.0f
    );

    movement *= 2.0f * dt * (0.1f / scale);
    //movement *= dt;

    for (Ball& ball : balls) {
        ball.position.x -= movement.x;
        ball.position.y += movement.y;
        ball.lastPosition.x -= movement.x;
        ball.lastPosition.y += movement.y;
    }
}

void updateLoop(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    glfwGetWindowPos(window, &windowPosX, &windowPosY);
    lastElapsedTime = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        elapsedTime = glfwGetTime();
        float dt = elapsedTime - lastElapsedTime;
        if (dt > MIN_TIME_PER_FRAME) {
            computeWindowMovement(window, dt);
            update(window, dt);
            lastElapsedTime = elapsedTime;
        }
        //std::cout << "x: " << windowPosX << " y: " << windowPosY << std::endl;
    }
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, 0);

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

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    scaleUniformId = glGetUniformLocation(shaderProgram, "scale");
    if (scaleUniformId == -1) {
        std::cout << "scale not found in vertex shader" << std::endl;
        return -1;
    }

    colorUniformId = glGetUniformLocation(shaderProgram, "color");
    if (colorUniformId == -1) {
        std::cout << "color not found in fragment shader" << std::endl;
        return -1;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    //float vertices[] = {
    //    -0.5f, -0.5f, 0.0f, // left  
    //     0.5f, -0.5f, 0.0f, // right 
    //     0.0f,  0.5f, 0.0f  // top   
    //}; 
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    initCircleVerts();

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    //Ball ball;
    //ball.radius = 1.0f;
    //ball.position = {0,0,0};
    //balls.emplace_back(ball);
    //for (int i = 0; i < 100; i++) {
    //    Ball ball;
    //    ball.radius = 1.0f;
    //    ball.position = { -50.0f + 5.0f * (i % 10), 50.0f + 5.0f * (float)(-(int)(i / 10)), 0.0f };
    //    balls.emplace_back(ball);
    //}

    for (float y = -90.0f; y <= 90.0f; y += 20.0f) {
        for (float x = -90.0f; x <= 90.0f; x += 20.0f) {
            Ball ball;
            ball.radius = 1.0f;
            ball.position = { x, y, 0.0f };
            balls.emplace_back(ball);
        }
    }


    glfwMakeContextCurrent(NULL);
    std::thread updateThread(updateLoop, window);
    lastElapsedTime = 0.0f;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        //update(window);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        //glfwSwapBuffers(window);
        glfwPollEvents();

        //lastElapsedTime = elapsedTime;
    }

    updateThread.join();

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}