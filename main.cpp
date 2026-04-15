#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

constexpr int SCREEN_WIDTH = 400;
constexpr int SCREEN_HEIGHT = 300;

// Shader RAII class
class Shader {
public:
    GLuint ID;
    Shader(const char* vertexSrc, const char* fragmentSrc) {
        GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);

        ID = glCreateProgram();
        glAttachShader(ID, vs);
        glAttachShader(ID, fs);
        glLinkProgram(ID);

        int success;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success){
            char info[512];
            glGetProgramInfoLog(ID, 512, nullptr, info);
            std::cerr << "Shader Program link error: " << info << std::endl;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    ~Shader() { glDeleteProgram(ID); }
    void use() { glUseProgram(ID); }
    void setMat4(const std::string& name, const glm::mat4& mat){
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    GLuint compile(GLenum type, const char* src){
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success){
            char info[512];
            glGetShaderInfoLog(shader, 512, nullptr, info);
            std::cerr << "Shader compile error: " << info << std::endl;
        }
        return shader;
    }
};

// Vertex Shader
const char* vertexShaderSrc = R"(
#version 440 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vertexColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    vertexColor = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Fragment Shader
const char* fragmentShaderSrc = R"(
#version 440 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

// Simple bounding-sphere frustum culling
bool inFrustum(const glm::vec3& cubePos, float cubeRadius, const glm::mat4& view, float fovY, float aspect, float nearPlane, float farPlane) {
    glm::vec4 viewPos = view * glm::vec4(cubePos,1.0f);
    float z = -viewPos.z;

    if(z - cubeRadius > farPlane || z + cubeRadius < nearPlane)
        return false;

    float tanHalfFovY = tan(fovY/2.0f);
    float yBound = z * tanHalfFovY;
    float xBound = yBound * aspect;

    if(viewPos.x - cubeRadius > xBound || viewPos.x + cubeRadius < -xBound)
        return false;
    if(viewPos.y - cubeRadius > yBound || viewPos.y + cubeRadius < -yBound)
        return false;

    return true;
}

int main() {
    if(SDL_Init(SDL_INIT_VIDEO)<0){
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("Interactive Cube C++", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0); // disable vsync

    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK){
        std::cerr << "GLEW init failed!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    Shader shader(vertexShaderSrc, fragmentShaderSrc);

    float vertices[] = {
       -0.5f,-0.5f,-0.5f, 1,0,0,
        0.5f,-0.5f,-0.5f, 0,1,0,
        0.5f, 0.5f,-0.5f, 0,0,1,
       -0.5f, 0.5f,-0.5f, 1,1,0,
       -0.5f,-0.5f, 0.5f, 1,0,1,
        0.5f,-0.5f, 0.5f, 0,1,1,
        0.5f, 0.5f, 0.5f, 1,1,1,
       -0.5f, 0.5f, 0.5f, 0,0,0
    };
    unsigned int indices[] = {
        0,1,2, 0,2,3,
        4,6,5, 4,7,6,
        4,5,1, 4,1,0,
        3,2,6, 3,6,7,
        1,5,6, 1,6,2,
        4,0,3, 4,3,7
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), float(SCREEN_WIDTH)/SCREEN_HEIGHT, 0.1f, 100.0f);

    glm::vec3 pos(0.0f,0.0f,-3.0f);
    float rotX=0.0f, rotY=0.0f;

    bool running=true;
    SDL_Event event;
    const Uint8* keystate;

    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;

    int frameCount=0;
    double fpsTimer=0.0;

    float cubeRadius = std::sqrt(0.5f*0.5f*3); // bounding sphere radius

    while(running){
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = double(NOW - LAST)/SDL_GetPerformanceFrequency();

        frameCount++;
        fpsTimer += deltaTime;
        if(fpsTimer>=1.0){
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount=0;
            fpsTimer=0.0;
        }

        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) running=false;
        }

        keystate = SDL_GetKeyboardState(nullptr);
        float rotSpeed = 2.0f;
        float moveSpeed = 2.0f;

        if(keystate[SDL_SCANCODE_UP]) rotX -= rotSpeed*deltaTime;
        if(keystate[SDL_SCANCODE_DOWN]) rotX += rotSpeed*deltaTime;
        if(keystate[SDL_SCANCODE_LEFT]) rotY -= rotSpeed*deltaTime;
        if(keystate[SDL_SCANCODE_RIGHT]) rotY += rotSpeed*deltaTime;
        if(keystate[SDL_SCANCODE_W]) pos.z += moveSpeed*deltaTime;
        if(keystate[SDL_SCANCODE_S]) pos.z -= moveSpeed*deltaTime;
        if(keystate[SDL_SCANCODE_A]) pos.x += moveSpeed*deltaTime;
        if(keystate[SDL_SCANCODE_D]) pos.x -= moveSpeed*deltaTime;
        if(keystate[SDL_SCANCODE_ESCAPE]) running=false;

        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), rotX, glm::vec3(1,0,0));
        model = glm::rotate(model, rotY, glm::vec3(0,1,0));
        glm::mat4 view = glm::translate(glm::mat4(1.0f), pos);

        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        if(inFrustum(pos, cubeRadius, view, glm::radians(45.0f), float(SCREEN_WIDTH)/SCREEN_HEIGHT, 0.1f, 100.0f)){
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }

        SDL_GL_SwapWindow(window);
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
