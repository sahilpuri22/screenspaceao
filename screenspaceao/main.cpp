/*
Framework based on LearnOpenGL (de Vries, 2014)
https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/9.ssao/ssao.cpp
Code adjusted for purposes of this application and implementation of HBAO and Alchemy SSAO
Including additions of preset cameras, lighting, UI, testing methods 
*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <C:/Users/Admin/Dissertation/screenspaceao/screenspaceao/shader_s.h>
#include <C:/Users/Admin/Dissertation/screenspaceao/screenspaceao/camera.h>
#include <C:/Users/Admin/Dissertation/screenspaceao/screenspaceao/model.h>

#include <iostream>
#include <random>
#include <fstream>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

void renderQuad();


// Screen Dimensions
unsigned int SCR_WIDTH = 1920;
unsigned int SCR_HEIGHT = 1080;


// Camera Setup
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool cameraEnabled = true;

struct CameraPreset {
    glm::vec3 position;
    glm::vec3 lookAt; 
    float yaw;
    float pitch;
    float zoom;
};

// setup preset camera angles for testing
std::vector<CameraPreset> cameraPresets;

// Manual values for angles that show off differences in AO
void initializeCameraPresets() {
    cameraPresets.push_back({ glm::vec3(97.6318f, 25.6399f, -20.6731f), glm::vec3(0.0f, 0.0f, 0.0f), 24.5001f, -9.90001f, 45.0f });
    cameraPresets.push_back({ glm::vec3(-131.494f, 26.2515f, 4.82874f), glm::vec3(0.0f, 0.0f, 0.0f), 350.9f, 3.79998f, 45.0f });
    cameraPresets.push_back({ glm::vec3(33.8566f, 16.6878f, -3.60557f), glm::vec3(0.0f, 0.0f, 0.0f), 312.9f, -8.20003f, 45.0f });
    cameraPresets.push_back({ glm::vec3(-94.2336f, 65.0085f, 18.673f), glm::vec3(0.0f, 0.0f, 0.0f), 333.496f, -12.9f, 45.0f });
}

int currentPresetIndex = 0;

void switchCameraPreset(Camera& camera) {
    currentPresetIndex = (currentPresetIndex + 1) % cameraPresets.size();
    std::cout << "Switching to camera preset index: " << currentPresetIndex << std::endl;

    const CameraPreset& preset = cameraPresets[currentPresetIndex];
    camera.Position = preset.position;
    camera.Yaw = preset.yaw;
    camera.Pitch = preset.pitch;
    camera.Zoom = preset.zoom;

    camera.updateCameraVectors();
}

// logs the current state of the camera
void logCameraState(const Camera& camera) {
    std::ofstream logFile("camera_presets.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << "Camera Preset: \n"
            << "Position: " << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << "\n"
            << "Yaw: " << camera.Yaw << "\n"
            << "Pitch: " << camera.Pitch << "\n"
            << "Zoom: " << camera.Zoom << "\n\n";
        logFile.close();
        std::cout << "Camera state logged to camera_presets.log" << std::endl;
    }
    else {
        std::cerr << "Unable to open log file." << std::endl;
    }
}

// toggles
bool showGui = true;
bool enableSSAO = true;
bool enableHBAO = false;
bool enableALCHAO = false;
bool enableTextures = true;


// logging of FPS test
bool isTesting = false;
int currentAOSetting = 0; // 0 = SSAO, 1 = HBAO, 2 = ALCHAO 3 = No AO
float testDuration = 5.0f; // Test each AO for 5 seconds
float elapsedTime = 0.0f; // Timer to keep track of elapsed time per setting

int frameCount = 0; // Count number of frames in 5 seconds
float fpsSum = 0.0f; // Sum the frames to be averaged later

// function to switch between AOs
void applyAOSetting(int setting) {
    enableSSAO = (setting == 0);
    enableHBAO = (setting == 1);
    enableALCHAO = (setting == 2);
    if (setting == 3) {
        enableSSAO = false;
        enableHBAO = false;
        enableALCHAO = false;
    }
}

// Testing function runs through each camera and measures fps for each AO for 5 seconds
void updateTesting(float deltaTime) {
    if (!isTesting) return;

    elapsedTime += deltaTime;
    fpsSum += 1.0f / deltaTime;
    frameCount++;

    // Check if the current test duration has elapsed
    if (elapsedTime >= testDuration) {
        float avgFPS = fpsSum / frameCount;
        std::cout << "Camera Preset " << currentPresetIndex << ", AO Setting " << currentAOSetting << ": " << avgFPS << std::endl;

        // Reset timers and counters for the next test
        elapsedTime = 0.0f;
        fpsSum = 0.0f;
        frameCount = 0;

        // Move to the next AO setting
        currentAOSetting++;
        if (currentAOSetting > 3) {
            currentAOSetting = 0;

            if (currentPresetIndex >= cameraPresets.size()-1) {
                isTesting = false;
                std::cout << "All tests complete." << std::endl;
                return;
            }
            switchCameraPreset(camera);
        }

        // Apply the current AO setting for the new preset or after resetting AO settings
        applyAOSetting(currentAOSetting);
    }
}


// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// linearly interpolates between two values `a` and `b` based on the fraction `f`
float ourLerp(float a, float b, float f)
{
    return a + f * (b - a);
}



int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
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

    // uncap fps
    glfwSwapInterval(0);


    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders

    Shader shaderGeometryPass("ssao_geometry.vs", "ssao_geometry.fs");
    Shader shaderLightingPass("ssao.vs", "ssao_lighting.fs");

    Shader shaderSSAO("ssao.vs", "ssao.fs");
    Shader shaderSSAOBlur("ssao.vs", "ssao_blur.fs");

    Shader shaderHBAO("ssao.vs", "hbao.fs");
    Shader shaderHBAOBlur("ssao.vs", "ssao_blur.fs");

    Shader shaderALCHAO("ssao.vs", "ssao_alch.fs");
    Shader shaderALCHAOBlur("ssao.vs", "ssao_blur.fs");

    // load models
    string modelPath = "C:/Users/Admin/Dissertation/screenspaceao/screenspaceao/crytek_sponza/sponza.obj";
    Model sponzaModel(modelPath);


    // configure g-buffer framebuffer
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedo;
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SSAO-------------------------------------------------------------------------------------
    // create framebuffer to hold SSAO processing stage 
    unsigned int ssaoFBO, ssaoBlurFBO;
    glGenFramebuffers(1, &ssaoFBO);  glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    unsigned int ssaoColorBuffer, ssaoColorBufferBlur;
    // SSAO color buffer
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    // blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // HBAO------------------------------------------------------------------------------------
    // create framebuffer to hold HBAO processing stage
    unsigned int hbaoFBO, hbaoBlurFBO;
    glGenFramebuffers(1, &hbaoFBO);
    glGenFramebuffers(1, &hbaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hbaoFBO);
    unsigned int hbaoColorBuffer, hbaoColorBufferBlur;
    // HBAO color buffer
    glGenTextures(1, &hbaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, hbaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hbaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "HBAO Framebuffer not complete!" << std::endl;
    // HBAO blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, hbaoBlurFBO);
    glGenTextures(1, &hbaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, hbaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hbaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "HBAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SSAO ALCHEMY------------------------------------------------------------------------------
    // create framebuffer to hold SSAOALCH processing stage
    unsigned int alchaoFBO, alchaoBlurFBO;
    glGenFramebuffers(1, &alchaoFBO);
    glGenFramebuffers(1, &alchaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, alchaoFBO);
    unsigned int alchaoColorBuffer, alchaoColorBufferBlur;
    // ALCHAO color buffer
    glGenTextures(1, &alchaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, alchaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, alchaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ALCHAO Framebuffer not complete!" << std::endl;
    // ALCHAO blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, alchaoBlurFBO);
    glGenTextures(1, &alchaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, alchaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, alchaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ALCHAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // generate ssao sample kernel
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 16; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 16.0f;

        // scale samples s.t. they're more aligned to center of kernel
        scale = ourLerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // generate ssao noise texture
    // ----------------------
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
    unsigned int noiseTexture; glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Generate HBAO noise texture
    // ---------------------------
    std::vector<glm::vec3> hbaoNoise;
    size_t noiseSize = 16; // Number of noise values (4x4 texture)
    for (size_t i = 0; i < noiseSize; ++i) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f, // x-coordinate
            randomFloats(generator) * 2.0f - 1.0f, // y-coordinate
            0.0f // z-coordinate is zero since we operate in tangent space
        );
        // Normalize the noise vector to stay on the xy-plane
        hbaoNoise.push_back(glm::normalize(noise));
    }

    unsigned int hbaoNoiseTexture;
    glGenTextures(1, &hbaoNoiseTexture);
    glBindTexture(GL_TEXTURE_2D, hbaoNoiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, hbaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // lighting info
    // -------------
    const unsigned int NR_LIGHTS = 14;
    std::vector<glm::vec3> lightColors = {
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f),  
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3(1.0f, 1.0f, 1.0f), 
    };

    std::vector<glm::vec3> lightPositions = {
        glm::vec3(-100.0f, 30.0f, -5.0f),
        glm::vec3(80.0f, 30.0f, -5.0f),
        glm::vec3(-5.0f, 50.0f, -10.0f),
        glm::vec3(100.0f, 80.0f, 30.0f),
        glm::vec3(100.0f, 80.0f, -30.0f),
        glm::vec3(-110.0f, 80.0f, -30.0f),
        glm::vec3(-110.0f, 80.0f, 30.0f),
        glm::vec3(-10.0f, 30.0f, -60.0f),
        glm::vec3(-5.0f, 30.0f, 50.0f),
        glm::vec3(125.0f, 30.0f, -60.0f),
        glm::vec3(-138.0f, 30.0f, -600.0f),
        glm::vec3(-138.0f, 30.0f, 50.0f),
        glm::vec3(120.0f, 30.0f, 50.0f),
        glm::vec3(0.0f, 40.0f, 0.0f)
    };

    // shader configuration
    // --------------------
    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedo", 2);
    shaderLightingPass.setVec3("viewPos", camera.Position);
    shaderLightingPass.setInt("ssao", 3);
    shaderLightingPass.setVec3("dirLight.Direction", -0.2f, -1.0f, -0.3f);
    shaderLightingPass.setVec3("dirLight.Color", 0.5f, 0.5f, 0.5f); 
    shaderSSAO.use();
    shaderSSAO.setInt("gPosition", 0);
    shaderSSAO.setInt("gNormal", 1);
    shaderSSAO.setInt("texNoise", 2);
    shaderSSAOBlur.use();
    shaderSSAOBlur.setInt("ssaoInput", 0);
    shaderHBAO.use();
    shaderHBAO.setInt("gPosition", 0);
    shaderHBAO.setInt("gNormal", 1);
    shaderHBAO.setInt("texNoise", 2);
    shaderHBAOBlur.use();
    shaderHBAOBlur.setInt("hbaoInput", 0);
    shaderALCHAO.use();
    shaderALCHAO.setInt("gPosition", 0);
    shaderALCHAO.setInt("gNormal", 1);
    shaderALCHAO.setInt("texNoise", 2);
    shaderALCHAOBlur.use();
    shaderALCHAOBlur.setInt("ssaoInput", 0);


    // initialize imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // white texture
    GLuint whiteTexture;
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    float whitePixel[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // SSAO Parameters
    int ss_kernelSize = 16;
    float ss_radius = 2.9f;
    float ss_bias = 0.025f;

    // HBAO Parameters
    float hb_radius = 500000.f;
    float hb_bias = 0.f;
    int hb_samples = 4;

    // ALCHAO Parameters
    int al_kernelSize = 16;
    float al_radius = 1.0f;
    float al_bias = 0.025f;
    float al_sigma = 1.0f;
    int al_k = 1;
    float al_beta = 0.001f;
    float al_turns = 10.0f;

    // Initialize camera presets
    initializeCameraPresets();

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        updateTesting(deltaTime); // Update AO testing status

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // geometry pass
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 model = glm::mat4(1.0f);
            shaderGeometryPass.use();  // Use the arrow operator to access methods
            shaderGeometryPass.setBool("useTexture", enableTextures);
            shaderGeometryPass.setMat4("projection", projection);
            shaderGeometryPass.setMat4("view", view);
            
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0));
            model = glm::scale(model, glm::vec3(0.1f));
            shaderGeometryPass.setMat4("model", model);

            sponzaModel.Draw(shaderGeometryPass);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // SSAO-----------------------------------------------------------------------------------
        // generate SSAO texture
        if (enableSSAO) {
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
                glClear(GL_COLOR_BUFFER_BIT);
                shaderSSAO.use();
                // Send kernel + rotation 
                for (unsigned int i = 0; i < 16; ++i)
                    shaderSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
                shaderSSAO.setMat4("projection", projection);
                shaderSSAO.setInt("kernelSize", ss_kernelSize);
                shaderSSAO.setFloat("radius", ss_radius);
                shaderSSAO.setFloat("bias", ss_bias);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, gPosition);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, gNormal);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, noiseTexture);
                renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        }
        // blur SSAO texture to remove noise
        if (enableSSAO){
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
                glClear(GL_COLOR_BUFFER_BIT);
                shaderSSAOBlur.use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
                renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // HBAO-----------------------------------------------------------------------------------
        // generate HBAO texture
        if (enableHBAO) {
            glBindFramebuffer(GL_FRAMEBUFFER, hbaoFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderHBAO.use();
            shaderHBAO.setMat4("projection", projection);
            shaderHBAO.setMat4("view", view); 
            shaderHBAO.setFloat("radius", hb_radius);
            shaderHBAO.setFloat("bias", hb_bias);
            shaderHBAO.setInt("samples", hb_samples);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, hbaoNoiseTexture);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        //  blur HBAO texture to remove noise
        if (enableHBAO) {
            glBindFramebuffer(GL_FRAMEBUFFER, hbaoBlurFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderHBAOBlur.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hbaoColorBuffer);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        
        // ALCHAO---------------------------------------------------------------------------------
        // generate ALCHAO texture
        if (enableALCHAO) {
            glBindFramebuffer(GL_FRAMEBUFFER, alchaoFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderALCHAO.use();
            // Send kernel + rotation 
            for (unsigned int i = 0; i < 16; ++i)
                shaderALCHAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
            shaderALCHAO.setMat4("projection", projection);
            shaderALCHAO.setInt("kernelSize", al_kernelSize);
            shaderALCHAO.setFloat("radius", al_radius);
            shaderALCHAO.setFloat("bias", al_bias);
            shaderALCHAO.setFloat("sigma", al_sigma);
            shaderALCHAO.setInt("k", al_k);
            shaderALCHAO.setFloat("beta", al_beta);
            shaderALCHAO.setFloat("turns", al_turns);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, noiseTexture);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        // blur ALCHAO texture to remove noise
        if (enableALCHAO) {
            bool horizontal = true, first_iteration = true;
            int amount = 10; // Number of blur iterations
            shaderALCHAOBlur.use();
            for (unsigned int i = 0; i < amount; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, horizontal ? alchaoBlurFBO : alchaoFBO);
                shaderALCHAOBlur.setInt("horizontal", horizontal);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, first_iteration ? alchaoColorBuffer : alchaoColorBufferBlur);
                renderQuad();
                horizontal = !horizontal;
                if (first_iteration)
                    first_iteration = false;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        
        //  lighting pass
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderLightingPass.use();
        shaderLightingPass.setMat4("invView", glm::inverse(camera.GetViewMatrix()));
        glm::vec3 camPosition = camera.Position; 
        shaderLightingPass.setVec3("viewPos", camPosition);

        for (int i = 0; i < lightPositions.size(); ++i) {
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
            const float linear = 0.09f;
            const float quadratic = 0.01f;
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Linear", linear);       // Assuming all lights have the same
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic); // attenuation factors.
        }


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);

        glActiveTexture(GL_TEXTURE3);  
        
        if (enableSSAO) {
            glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur); 
        }
        else if (enableHBAO) {
            glBindTexture(GL_TEXTURE_2D, hbaoColorBufferBlur); 
        }
        else if (enableALCHAO) {
            glBindTexture(GL_TEXTURE_2D, alchaoColorBufferBlur); 
        }
        else {
            glBindTexture(GL_TEXTURE_2D, whiteTexture);
        }
        
        renderQuad();


        if (showGui)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));  // Width, Height


            // Calculate and display FPS
            static float fps = 0.0f;
            static float lastTime = 0.0f;

            float currentTime = glfwGetTime();
            fps++;
            if (currentTime - lastTime >= 1.0f) {
                lastTime = currentTime;
                static std::string title = "(FPS: " + std::to_string(static_cast<int>(fps)) + ")";
                glfwSetWindowTitle(window, title.c_str());
                fps = 0;
            }

            // RENDER IMGUI WINDOW
            ImGui::Begin("Performance and Settings"); 
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Checkbox("SSAO (1)", &enableSSAO); 
            ImGui::Checkbox("HBAO (2)", &enableHBAO); 
            ImGui::Checkbox("ALCHAO (3)", &enableALCHAO); 
            ImGui::Checkbox("Texture (T)", &enableTextures); 
            ImGui::Text("Cycle Through Preset Cameras (Z)");
            glm::vec3 camPos = camera.Position; 
            ImGui::Text("Camera Position:"); 
            ImGui::Text("X: %.2f", camPos.x); 
            ImGui::Text("Y: %.2f", camPos.y); 
            ImGui::Text("Z: %.2f", camPos.z); 
            
            // SLIDERS SSAO
            ImGui::Separator();
            ImGui::Text("SSAO Parameters");
            ImGui::SliderFloat("SSAO radius", &ss_radius, 0.0f, 100.f);
            ImGui::SliderFloat("SSAO bias", &ss_bias, 0.f, 1.f);

            // SLIDERS HBAO
            ImGui::Separator();
            ImGui::Text("HBAO Parameters");
            ImGui::SliderFloat("HBAO Radius", &hb_radius, 0.0f, 1000000.0f);
            ImGui::SliderFloat("HBAO Bias", &hb_bias, 0.0f, 40.0f);

            // SLIDERS ALCHAO
            ImGui::Separator();
            ImGui::Text("ALCHAO Parameters");
            ImGui::SliderFloat("ALCHAO radius", &al_radius, 0.0f, 20.f);
            ImGui::SliderFloat("ALCHAO bias", &al_bias, 0.f, 1.f);
            ImGui::SliderFloat("ALCHAO sigma", &al_sigma, 0.f, 20.f);
            ImGui::SliderInt("ALCHAO k", &al_k, 0, 10);
            ImGui::InputFloat("ALCHAO beta", &al_beta, 0.f, 0.001f, "%.6f");
            ImGui::SliderFloat("ALCHAO turns", &al_turns , 0.f, 30.f);

            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        

        // glfw swap buffers and poll IO events 
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // shutdown imgui

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}


// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // Camera Sprint [SHIFT]
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.MovementSpeed = SPEED * 2.0f; // Temporary speed-up while shift is held
    }
    else {
        camera.MovementSpeed = SPEED; // Default speed
    }
    
    // Toggle SSAO [1]
    static bool onePressed = false;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !onePressed)
    {
        enableSSAO = !enableSSAO;
        enableHBAO = false;
        enableALCHAO = false;
        onePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE)
    {
        onePressed = false;
    }

    // Toggle HBAO [2]
    static bool twoPressed = false;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !twoPressed)
    {
        enableHBAO = !enableHBAO;
        enableSSAO = false;
        enableALCHAO = false;
        twoPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE)
    {
        twoPressed = false;
    }

    // Toggle ALCHAO [3]
    static bool threePressed = false;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !threePressed)
    {
        enableALCHAO = !enableALCHAO;
        enableSSAO = false;
        enableHBAO = false;
        threePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE)
    {
        threePressed = false;
    }

    // Toggle Texture [T]
    static bool tPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tPressed)
    {
        enableTextures = !enableTextures;
        tPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
    {
        tPressed = false;
    }

    // Toggle GUI [G]
    static bool gPressed = false;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !gPressed)
    {
        showGui = !showGui;
        gPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
    {
        gPressed = false;
    }

    // Toggle Cursor [C]
    static bool cPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cPressed) {
        int cursorMode = glfwGetInputMode(window, GLFW_CURSOR);

        if (cursorMode == GLFW_CURSOR_NORMAL) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cameraEnabled = true;
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cameraEnabled = false;
        }

        cPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        cPressed = false; 
    }

    // Switch Between Camera Presets [Z]
    static bool zPressed = false;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && !zPressed) {
        switchCameraPreset(camera);
        zPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE) {
        zPressed = false; 
    }

    // Log Camera State [L]
    static bool lPressed = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        static bool lPressed = false;
        if (!lPressed) {
            logCameraState(camera);
            lPressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        lPressed = false;
    }

    // Start FPS Test [K]
    static bool kPressed = false;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && !kPressed) {
        isTesting = true; 
        currentPresetIndex = -1;
        currentAOSetting = 0;
        elapsedTime = 0.0f;
        fpsSum = 0.0f;
        frameCount = 0;
        kPressed = true; 

        switchCameraPreset(camera); 
        applyAOSetting(currentAOSetting); 
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE) {
        kPressed = false; 
    }

}




// glfw whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

}





// glfw whenever the mouse moves, this callback is called
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

    if (cameraEnabled) 
    {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; 

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
    else
    {
        lastX = xpos;
        lastY = ypos;
    }
}

// glfw whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

