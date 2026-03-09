#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "geometry/room_geometry.h"
#include "geometry/room2_geometry.h"
#include "geometry/tree_geometry.h"
#include "geometry/mesh_helpers.h"
#include "geometry/curtain_geometry.h"
#include "geometry/window_geometry.h"
#include "geometry/mouse_geometry.h"
#include "geometry/picture_frame_geometry.h"
#include "geometry/forest_tree_geometry.h"
#include "geometry/grandfather_clock_geometry.h"


// ===== CAMERA GLOBALS =====
glm::vec3 cameraPos   = glm::vec3(0.0f, 1.6f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw   = -90.0f;
float pitch = 0.0f;
float lastX = 800 / 2.0f;
float lastY = 600 / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float treeGrowth = 0.7f;
float targetTreeGrowth = 0.7f;
bool growingTree = false;

// Mice: run after tree stops growing
bool treeJustFinished = false;
float miceStartTime = -1.0f;
const float MICE_RUN_DURATION = 3.0f;

// Door: opened by clicking cactus painting
float doorOpenAmount = 0.0f;  // 0=closed, 1=fully open
bool doorOpening = false;
const float DOOR_WIDTH = 2.5f;
const float DOOR_HEIGHT = 3.0f;  // door from floor (y=0) to y=3

// Snow room particles
const int N_SNOW = 900;


// --- Utility: load shader from file ---
std::string loadFile(const char* path) {
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// --- Utility: compile shader ---
GLuint compileShader(const char* path, GLenum type) {
    std::string src = loadFile(path);
    const char* csrc = src.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &csrc, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        std::cout << "Shader compile error in " << path << "\n" << log << std::endl;
    }
    return shader;
}

// --- Utility: link shaders ---
GLuint makeProgram(const char* vsPath, const char* fsPath) {
    GLuint vs = compileShader(vsPath, GL_VERTEX_SHADER);
    GLuint fs = compileShader(fsPath, GL_FRAGMENT_SHADER);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, NULL, log);
        std::cout << "Program link error:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
        return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* window)
{
    float speed = 3.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += right * speed;

    //esc to unlock cursor
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }

    // Left mouse to lock cursor again
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    }

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        growingTree = true;
        targetTreeGrowth = 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        growingTree = true;
        targetTreeGrowth = 0.7f;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        doorOpening = true;
    }
}

// Door area on right wall (x=5): y 1-4, z -1.25 to 1.25
bool rayIntersectsDoor(glm::vec3 rayOrigin, glm::vec3 rayDir) {
    float wallX = 5.0f;
    if (fabs(rayDir.x) < 0.001f) return false;
    float t = (wallX - rayOrigin.x) / rayDir.x;
    if (t <= 0.0f) return false;
    glm::vec3 hit = rayOrigin + t * rayDir;
    return hit.y >= 0.0f && hit.y <= 3.0f && hit.z >= -1.25f && hit.z <= 1.25f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    // Use center of screen when cursor locked (where player is looking), else use cursor pos
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float ndcX, ndcY;
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        ndcX = 0.0f;
        ndcY = 0.0f;
    } else {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        ndcX = (2.0f * mx / width) - 1.0f;
        ndcY = 1.0f - (2.0f * my / height);
    }

    float aspect = (float)width / height;
    float fov = glm::radians(60.0f);

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
    glm::vec3 rayDir = glm::normalize(
        cameraFront +
        right * (ndcX * aspect * tanf(fov / 2.0f)) +
        cameraUp * (ndcY * tanf(fov / 2.0f))
    );

    if (rayIntersectsDoor(cameraPos, rayDir)) {
        doorOpening = true;
    }
}


int main() {
    // --- Initialize GLFW ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 800, "Nutcracker Room", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        return -1;
    }

    glfwMakeContextCurrent(window);

    // --- Initialize GLEW (instead of glad) ---
    glewExperimental = true; 
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW\n";
        return -1;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);


    glEnable(GL_DEPTH_TEST);

    // --- Shader program ---
    GLuint roomShader = makeProgram("shaders/room.vert", "shaders/room.frag");

    // --- Room VAO ---
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, roomVertices.size() * sizeof(Vertex),
                 roomVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, roomIndices.size() * sizeof(unsigned int),
                 roomIndices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // --- Generate tree geometry ---
    std::vector<Vertex> treeVerts;
    std::vector<unsigned int> treeIdx;

    GenerateTreeGeometry(treeVerts, treeIdx);

    GLuint treeVAO, treeVBO, treeEBO;
    glGenVertexArrays(1, &treeVAO);
    glGenBuffers(1, &treeVBO);
    glGenBuffers(1, &treeEBO);

    glBindVertexArray(treeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, treeVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 treeVerts.size() * sizeof(Vertex),
                 treeVerts.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 treeIdx.size() * sizeof(unsigned int),
                 treeIdx.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // --- Texture placeholder (white pixel) ---
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    unsigned char white[3] = {255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // ----------------- ORNAMENTS / LIGHTS / STAR SETUP -----------------

    // create sphere mesh (ornaments & light bulbs)
    std::vector<Vertex> sphereVerts;
    std::vector<unsigned int> sphereIdx;
    //float ornamentRadius = 0.04f + 0.02f * sin(i * 12.3f);
    MakeSphereMesh(12, 24, sphereVerts, sphereIdx, 0.08f); // small sphere radius 0.08

    GLuint sphereVAO=0, sphereVBO=0, sphereEBO=0;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVerts.size()*sizeof(Vertex), sphereVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIdx.size()*sizeof(unsigned int), sphereIdx.data(), GL_STATIC_DRAW);

    // attribs (match Vertex layout)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    // create star mesh
    std::vector<Vertex> starVerts;
    std::vector<unsigned int> starIdx;
    MakeStarMesh(starVerts, starIdx, 0.25f);

    GLuint starVAO=0, starVBO=0, starEBO=0;
    glGenVertexArrays(1, &starVAO);
    glGenBuffers(1, &starVBO);
    glGenBuffers(1, &starEBO);

    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, starVerts.size()*sizeof(Vertex), starVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, starEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, starIdx.size()*sizeof(unsigned int), starIdx.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // Create ornament positions on cone surface
    const int N_ORNAMENTS = 150;
    const int N_LIGHTS = 200;
    std::vector<glm::vec3> ornamentPositions;
    std::vector<glm::vec3> ornamentColors;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;

    // tree geometry parameters (match GenerateTreeGeometry: trunk + 4 foliage layers)
    struct Layer { float yBase, height, radius; };
    Layer layers[] = {
        {0.6f, 1.4f, 1.8f},   // layer0
        {1.58f, 1.2f, 1.3f},  // layer1
        {2.38f, 1.0f, 0.9f},  // layer2
        {2.88f, 0.8f, 0.5f}   // layer3
    };

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> ud_layer(0, 3);
    std::uniform_real_distribution<float> ud_t(0.1f, 0.95f);
    std::uniform_real_distribution<float> ud_ang(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> ud_color(0.0f, 1.0f);

    // sample positions on layered tree surface
    for (int i = 0; i < N_ORNAMENTS; ++i) {
        int li = ud_layer(rng);
        float t = ud_t(rng);
        float localR = layers[li].radius * (1.0f - t);
        float ang = ud_ang(rng);
        float x = cos(ang) * localR;
        float z = sin(ang) * localR;
        float y = layers[li].yBase + t * layers[li].height;
        ornamentPositions.push_back(glm::vec3(x, y, z));
        // random ornament color (red/blue/gold/white)
        glm::vec3 c;
        float pick = ud_color(rng);
        if (pick < 0.25f) c = glm::vec3(0.9f, 0.1f, 0.1f);
        else if (pick < 0.55f) c = glm::vec3(1.0f, 0.85f, 0.2f);
        else if (pick < 0.8f) c = glm::vec3(0.05f, 0.55f, 0.9f);
        else c = glm::vec3(0.95f, 0.95f, 0.95f);
        ornamentColors.push_back(c);
    }

    // lights (emissive) — sample on layered tree surface
    for (int i = 0; i < N_LIGHTS; ++i) {
        int li = ud_layer(rng);
        float t = ud_t(rng);
        float localR = layers[li].radius * (1.0f - t);
        float ang = ud_ang(rng);
        float x = cos(ang) * localR;
        float z = sin(ang) * localR;
        float y = layers[li].yBase + t * layers[li].height;
        lightPositions.push_back(glm::vec3(x, y, z));
        // colored lights
        glm::vec3 lc;
        float pick = ud_color(rng);
        if (pick < 0.4f) lc = glm::vec3(1.0f, 0.6f, 0.2f); // warm
        else if (pick < 0.7f) lc = glm::vec3(1.0f, 0.0f, 0.0f);
        else lc = glm::vec3(0.0f, 0.6f, 1.0f);
        lightColors.push_back(lc);
    }

    // star model transform parameters (top of tree)
    glm::vec3 starPos = glm::vec3(0.0f, 3.68f + 0.05f, 0.0f);
    float starScale = 0.6f;

    // === PRESENTS AROUND THE TREE ===
    std::vector<glm::vec3> presentPositions;
    std::vector<glm::vec3> presentColors;
    std::vector<glm::vec3> presentScales;
    std::vector<int> presentTypes;

    int N_PRESENTS = 24;
    std::uniform_real_distribution<float> pdist(0.2f, 1.0f);
    std::uniform_real_distribution<float> cdist(-0.3f, 0.3f);

    float ringRadius = 1.9;
    float ringThickness = 0.4f;

    for (int i = 0; i < N_PRESENTS; i++) {
        // base angle
        float angle = (float)i / N_PRESENTS * 2.0f * 3.1415926f;

        // --- jitter (simple random variation) ---
        float jr = ((rand() % 1000) / 1000.0f) * 0.5f - 0.25f;  // radius jitter = -0.25 → +0.25
        float ja = ((rand() % 1000) / 1000.0f) * 6.0f - 3.0f;   // angle jitter = -3° → +3°

        float angleJ = angle + glm::radians(ja);
        float r = ringRadius + jr;

        // final position
        float x = cos(angleJ) * r;
        float z = sin(angleJ) * r;

        presentPositions.push_back(glm::vec3(x, 0.0f, z));


        presentColors.push_back(glm::vec3(
            0.3f + 0.7f*((rand()%1000)/1000.0f),
            0.3f + 0.7f*((rand()%1000)/1000.0f),
            0.3f + 0.7f*((rand()%1000)/1000.0f)
        ));

        presentScales.push_back(glm::vec3(
            0.4f + 0.15f*((rand()%1000)/1000.0f),
            0.4f + 0.15f*((rand()%1000)/1000.0f),
            0.4f + 0.15f*((rand()%1000)/1000.0f)
        ));

        int typeRand = rand() % 3;
        presentTypes.push_back(typeRand);
    }

    // === PRESENT BOX MESH ===
    std::vector<Vertex> cubeVerts;
    std::vector<unsigned int> cubeIdx;
    MakeCubeMesh(cubeVerts, cubeIdx, 1.0f);

    GLuint cubeVAO, cubeVBO, cubeEBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, cubeVerts.size()*sizeof(Vertex), cubeVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIdx.size()*sizeof(unsigned int), cubeIdx.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);



    // === CURTAIN MESH ===
    std::vector<Vertex> curtainVerts;
    std::vector<unsigned int> curtainIdx;

    GenerateWavyCurtainMesh(curtainVerts, curtainIdx, 40, 20); // high-detail pleats

    GLuint curtainVAO, curtainVBO, curtainEBO;
    glGenVertexArrays(1, &curtainVAO);
    glGenBuffers(1, &curtainVBO);
    glGenBuffers(1, &curtainEBO);

    glBindVertexArray(curtainVAO);

    glBindBuffer(GL_ARRAY_BUFFER, curtainVBO);
    glBufferData(GL_ARRAY_BUFFER, curtainVerts.size()*sizeof(Vertex),
                curtainVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curtainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, curtainIdx.size()*sizeof(unsigned int),
                curtainIdx.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    // === WINDOW MESH (back wall, between curtains) ===
    std::vector<Vertex> windowVerts;
    std::vector<unsigned int> windowIdx;
    GenerateWindowMesh(windowVerts, windowIdx, 0.9f, 1.2f);
    GLuint windowVAO, windowVBO, windowEBO;
    glGenVertexArrays(1, &windowVAO);
    glGenBuffers(1, &windowVBO);
    glGenBuffers(1, &windowEBO);
    glBindVertexArray(windowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, windowVBO);
    glBufferData(GL_ARRAY_BUFFER, windowVerts.size() * sizeof(Vertex), windowVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, windowEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, windowIdx.size() * sizeof(unsigned int), windowIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // === PICTURE FRAMES ===
    std::vector<Vertex> frameVerts;
    std::vector<unsigned int> frameIdx;
    size_t frameBorderCount;
    GeneratePictureFrameMesh(frameVerts, frameIdx, frameBorderCount, 0.8f, 1.0f, 0.05f);
    GLuint frameVAO, frameVBO, frameEBO;
    glGenVertexArrays(1, &frameVAO);
    glGenBuffers(1, &frameVBO);
    glGenBuffers(1, &frameEBO);
    glBindVertexArray(frameVAO);
    glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
    glBufferData(GL_ARRAY_BUFFER, frameVerts.size() * sizeof(Vertex), frameVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, frameEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, frameIdx.size() * sizeof(unsigned int), frameIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // === MOUSE MESH ===
    std::vector<Vertex> mouseVerts;
    std::vector<unsigned int> mouseIdx;
    GenerateMouseMesh(mouseVerts, mouseIdx);
    GLuint mouseVAO, mouseVBO, mouseEBO;
    glGenVertexArrays(1, &mouseVAO);
    glGenBuffers(1, &mouseVBO);
    glGenBuffers(1, &mouseEBO);
    glBindVertexArray(mouseVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mouseVBO);
    glBufferData(GL_ARRAY_BUFFER, mouseVerts.size() * sizeof(Vertex), mouseVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mouseEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mouseIdx.size() * sizeof(unsigned int), mouseIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // === DOOR PANELS (right wall - slide open when cactus clicked) ===
    float dhw = DOOR_WIDTH * 0.25f;
    float dhh = DOOR_HEIGHT * 0.5f;
    std::vector<Vertex> doorVerts = {
        {{-dhw, -dhh, 0}, {0, 0, 1}, {0, 0}},
        {{dhw, -dhh, 0}, {0, 0, 1}, {1, 0}},
        {{dhw, dhh, 0}, {0, 0, 1}, {1, 1}},
        {{-dhw, dhh, 0}, {0, 0, 1}, {0, 1}}
    };
    std::vector<unsigned int> doorIdx = {0, 1, 2, 0, 2, 3};
    GLuint doorVAO, doorVBO, doorEBO;
    glGenVertexArrays(1, &doorVAO);
    glGenBuffers(1, &doorVBO);
    glGenBuffers(1, &doorEBO);
    glBindVertexArray(doorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
    glBufferData(GL_ARRAY_BUFFER, doorVerts.size() * sizeof(Vertex), doorVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, doorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, doorIdx.size() * sizeof(unsigned int), doorIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // === RIGHT WALL FRAME (around door opening) ===
    GLuint rightWallFrameVAO, rightWallFrameVBO, rightWallFrameEBO;
    glGenVertexArrays(1, &rightWallFrameVAO);
    glGenBuffers(1, &rightWallFrameVBO);
    glGenBuffers(1, &rightWallFrameEBO);
    glBindVertexArray(rightWallFrameVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rightWallFrameVBO);
    glBufferData(GL_ARRAY_BUFFER, rightWallFrameVertices.size() * sizeof(Vertex), rightWallFrameVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rightWallFrameEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rightWallFrameIndices.size() * sizeof(unsigned int), rightWallFrameIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // === SECOND ROOM VAO (for door destination) ===
    GLuint room2VAO, room2VBO, room2EBO;
    glGenVertexArrays(1, &room2VAO);
    glGenBuffers(1, &room2VBO);
    glGenBuffers(1, &room2EBO);
    glBindVertexArray(room2VAO);
    glBindBuffer(GL_ARRAY_BUFFER, room2VBO);
    glBufferData(GL_ARRAY_BUFFER, room2Vertices.size() * sizeof(Vertex), room2Vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, room2EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, room2Indices.size() * sizeof(unsigned int), room2Indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // === FOREST TREE SILHOUETTES (grey 2D trees for snow room walls) ===
    std::vector<Vertex> forestTreeVerts;
    std::vector<unsigned int> forestTreeIdx;
    GenerateForestTreeMesh(forestTreeVerts, forestTreeIdx, 0.8f, 1.5f);
    GLuint forestTreeVAO, forestTreeVBO, forestTreeEBO;
    glGenVertexArrays(1, &forestTreeVAO);
    glGenBuffers(1, &forestTreeVBO);
    glGenBuffers(1, &forestTreeEBO);
    glBindVertexArray(forestTreeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, forestTreeVBO);
    glBufferData(GL_ARRAY_BUFFER, forestTreeVerts.size() * sizeof(Vertex), forestTreeVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, forestTreeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, forestTreeIdx.size() * sizeof(unsigned int), forestTreeIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // === GRANDFATHER CLOCK (back wall, between curtains) ===
    std::vector<Vertex> clockBodyVerts, clockFaceVerts, clockHandsVerts;
    std::vector<unsigned int> clockBodyIdx, clockFaceIdx, clockHandsIdx;
    GenerateGrandfatherClockBody(clockBodyVerts, clockBodyIdx);
    GenerateClockFace(clockFaceVerts, clockFaceIdx);
    GenerateClockHands(clockHandsVerts, clockHandsIdx);

    GLuint clockBodyVAO, clockBodyVBO, clockBodyEBO;
    glGenVertexArrays(1, &clockBodyVAO);
    glGenBuffers(1, &clockBodyVBO);
    glGenBuffers(1, &clockBodyEBO);
    glBindVertexArray(clockBodyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, clockBodyVBO);
    glBufferData(GL_ARRAY_BUFFER, clockBodyVerts.size() * sizeof(Vertex), clockBodyVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, clockBodyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, clockBodyIdx.size() * sizeof(unsigned int), clockBodyIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    GLuint clockFaceVAO, clockFaceVBO, clockFaceEBO;
    glGenVertexArrays(1, &clockFaceVAO);
    glGenBuffers(1, &clockFaceVBO);
    glGenBuffers(1, &clockFaceEBO);
    glBindVertexArray(clockFaceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, clockFaceVBO);
    glBufferData(GL_ARRAY_BUFFER, clockFaceVerts.size() * sizeof(Vertex), clockFaceVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, clockFaceEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, clockFaceIdx.size() * sizeof(unsigned int), clockFaceIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    GLuint clockHandsVAO, clockHandsVBO, clockHandsEBO;
    glGenVertexArrays(1, &clockHandsVAO);
    glGenBuffers(1, &clockHandsVBO);
    glGenBuffers(1, &clockHandsEBO);
    glBindVertexArray(clockHandsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, clockHandsVBO);
    glBufferData(GL_ARRAY_BUFFER, clockHandsVerts.size() * sizeof(Vertex), clockHandsVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, clockHandsEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, clockHandsIdx.size() * sizeof(unsigned int), clockHandsIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // Snow particles (falling in snow room x 5-25, z -5 to 5)
    std::vector<glm::vec3> snowPositions(N_SNOW);
    std::mt19937 snowRng(42);
    std::uniform_real_distribution<float> snowX(5.5f, 24.5f);
    std::uniform_real_distribution<float> snowZ(-4.5f, 4.5f);
    std::uniform_real_distribution<float> snowY(0.1f, 4.9f);
    std::uniform_real_distribution<float> snowSpeed(0.3f, 0.8f);
    for (int i = 0; i < N_SNOW; i++) {
        snowPositions[i] = glm::vec3(snowX(snowRng), snowY(snowRng), snowZ(snowRng));
    }
    std::vector<float> snowSpeeds(N_SNOW);
    for (int i = 0; i < N_SNOW; i++) snowSpeeds[i] = snowSpeed(snowRng);

    // --- Main Loop ---
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        if (growingTree) {
            treeGrowth += (targetTreeGrowth - treeGrowth) * deltaTime * 2.0f;
            if (fabs(treeGrowth - targetTreeGrowth) < 0.01f) {
                treeGrowth = targetTreeGrowth;
                growingTree = false;
                treeJustFinished = true;
            }
        }
        if (treeJustFinished && miceStartTime < 0.0f) {
            miceStartTime = currentFrame;
            treeJustFinished = false;
        }
        if (doorOpening) {
            doorOpenAmount += deltaTime * 1.5f;
            if (doorOpenAmount >= 1.0f) {
                doorOpenAmount = 1.0f;
                doorOpening = false;
            }
        }

        // --- Projection & View ---
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        //glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f/800.0f, 0.1f, 100.0f);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(roomShader);

        glUniformMatrix4fv(glGetUniformLocation(roomShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(roomShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(roomShader, "timeVal"), glfwGetTime());

        // Model = identity
        glm::mat4 model(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // === LIGHTING UNIFORMS ===
        glUniform3f(glGetUniformLocation(roomShader, "lightPos"), 0.0f, 3.0f, 3.0f);
        glUniform3f(glGetUniformLocation(roomShader, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(roomShader, "viewPos"),
                    cameraPos.x, cameraPos.y, cameraPos.z);

        // Lights
        // glUniform3f(glGetUniformLocation(roomShader, "lightPos"), 0, 4, 4);
        // glUniform3f(glGetUniformLocation(roomShader, "lightColor"), 1, 1, 1);
        // glUniform3f(glGetUniformLocation(roomShader, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(glGetUniformLocation(roomShader, "diffuseTex"), 0);

        glUniform1i(glGetUniformLocation(roomShader, "isPresent"), 0);

        // Draw room (always omit solid right wall - we use frame + door panels)
        glUniform1i(glGetUniformLocation(roomShader, "isTree"), 0);
        glUniform3f(glGetUniformLocation(roomShader, "wallColor"),
                    0.55f, 0.35f, 0.20f); 
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, roomIndicesWithoutRightWall.size(), GL_UNSIGNED_INT, 0);
        // Right wall frame (around door)
        glBindVertexArray(rightWallFrameVAO);
        glDrawElements(GL_TRIANGLES, rightWallFrameIndices.size(), GL_UNSIGNED_INT, 0);

        // === Draw Curtains ===
        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);

        // deep red velvet color
        glUniform3f(glGetUniformLocation(roomShader, "overrideColor"),
                    0.25f, 0.10f, 0.05f);
                    //0.55f, 0.05f, 0.05f);

        glUniform1i(glGetUniformLocation(roomShader, "isTree"), 0);
        glUniform1i(glGetUniformLocation(roomShader, "isCurtain"), 1);
        glUniform1i(glGetUniformLocation(roomShader, "isPresent"), 0);


        float curtainOffsetX = 3.25f;
        float curtainHeightOffset = 0.0f;

        glm::mat4 curtainLeft = glm::mat4(1.0f);
        curtainLeft = glm::translate(curtainLeft, glm::vec3(-curtainOffsetX, curtainHeightOffset, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE,
                        glm::value_ptr(curtainLeft));
            
        glBindVertexArray(curtainVAO);
        glDrawElements(GL_TRIANGLES, curtainIdx.size(), GL_UNSIGNED_INT, 0); 

        glm::mat4 curtainRight = glm::mat4(1.0f);
        curtainRight = glm::translate(curtainRight, glm::vec3(curtainOffsetX, curtainHeightOffset, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE,
                        glm::value_ptr(curtainRight));
        
        glDrawElements(GL_TRIANGLES, curtainIdx.size(), GL_UNSIGNED_INT, 0);
        glUniform1i(glGetUniformLocation(roomShader, "isCurtain"), 0);
        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);

        // === Draw Grandfather Clock (back wall, between curtains, 3D boxes) ===
        glm::mat4 clockM = glm::mat4(1.0f);
        clockM = glm::translate(clockM, glm::vec3(0.0f, 0.0f, -4.7f));  // extends from wall

        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);
        glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 0.25f, 0.18f, 0.12f);  // dark wood body
        glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(clockM));
        glBindVertexArray(clockBodyVAO);
        glDrawElements(GL_TRIANGLES, clockBodyIdx.size(), GL_UNSIGNED_INT, 0);

        glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 1.0f, 1.0f, 1.0f);  // white clock face
        glBindVertexArray(clockFaceVAO);
        glDrawElements(GL_TRIANGLES, clockFaceIdx.size(), GL_UNSIGNED_INT, 0);

        glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 0.15f, 0.12f, 0.1f);  // dark hands
        glBindVertexArray(clockHandsVAO);
        glDrawElements(GL_TRIANGLES, clockHandsIdx.size(), GL_UNSIGNED_INT, 0);

        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);

        // === Draw Windows (left wall x=-5, equally spaced, dark blue night sky) ===
        glUniform1i(glGetUniformLocation(roomShader, "isWindow"), 1);
        glUniform3f(glGetUniformLocation(roomShader, "windowColor"), 0.05f, 0.08f, 0.2f);  // dark blue night
        float windowSpacing = 1.2f;
        glm::mat4 winLeft = glm::mat4(1.0f);
        winLeft = glm::translate(winLeft, glm::vec3(-4.99f, 2.1f, -windowSpacing));
        winLeft = glm::rotate(winLeft, glm::radians(-90.0f), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(winLeft));
        glBindVertexArray(windowVAO);
        glDrawElements(GL_TRIANGLES, windowIdx.size(), GL_UNSIGNED_INT, 0);
        glm::mat4 winRight = glm::mat4(1.0f);
        winRight = glm::translate(winRight, glm::vec3(-4.99f, 2.1f, windowSpacing));
        winRight = glm::rotate(winRight, glm::radians(-90.0f), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(winRight));
        glDrawElements(GL_TRIANGLES, windowIdx.size(), GL_UNSIGNED_INT, 0);
        glUniform1i(glGetUniformLocation(roomShader, "isWindow"), 0);

        // === Draw Picture Frame (right wall, same blue as snow room - only when door closed) ===
        if (doorOpenAmount < 0.01f) {
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);
            glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 0.4f, 0.25f, 0.15f);  // wood frame
            glUniform1i(glGetUniformLocation(roomShader, "isPainting"), 0);
            glBindVertexArray(frameVAO);
            glm::mat4 frameRight = glm::mat4(1.0f);
            frameRight = glm::translate(frameRight, glm::vec3(4.99f, 2.5f, 0.0f));
            frameRight = glm::rotate(frameRight, glm::radians(90.0f), glm::vec3(0, 1, 0));
            glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(frameRight));
            glDrawElements(GL_TRIANGLES, frameBorderCount, GL_UNSIGNED_INT, 0);
            glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 0.68f, 0.85f, 0.95f);  // same as snow room walls
            glDrawElements(GL_TRIANGLES, frameIdx.size() - frameBorderCount, GL_UNSIGNED_INT, (void*)(frameBorderCount * sizeof(unsigned int)));
            glUniform1i(glGetUniformLocation(roomShader, "isPainting"), 0);
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);
        }

        // === Draw Mice (after tree stops growing, run left to right) ===
        if (miceStartTime >= 0.0f) {
            float miceT = (currentFrame - miceStartTime) / MICE_RUN_DURATION;
            if (miceT < 1.0f) {
                float runX = -4.5f + miceT * 9.0f;  // -4.5 to 4.5
                glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);
                glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 0.35f, 0.3f, 0.28f);
                for (int m = 0; m < 6; m++) {
                    float offset = m * 0.15f;
                    float mx = runX + offset * (m % 2 == 0 ? 1.0f : -1.0f);
                    glm::mat4 mouseM = glm::mat4(1.0f);
                    mouseM = glm::translate(mouseM, glm::vec3(mx, 0.06f, 0.2f + m * 0.2f));
                    mouseM = glm::scale(mouseM, glm::vec3(1.2f));
                    glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(mouseM));
                    glBindVertexArray(mouseVAO);
                    glDrawElements(GL_TRIANGLES, mouseIdx.size(), GL_UNSIGNED_INT, 0);
                }
                glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);
            }
        }

        // === Draw Snow Room FIRST (behind door - white floor/ceiling, light blue walls) ===
        if (doorOpenAmount > 0.01f) {
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);
            glUniform1i(glGetUniformLocation(roomShader, "isSnowSurface"), 1);
            glBindVertexArray(room2VAO);
            glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
            glUniform3f(glGetUniformLocation(roomShader, "wallColor"), 1.0f, 1.0f, 1.0f);  // floor white
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glUniform3f(glGetUniformLocation(roomShader, "wallColor"), 1.0f, 1.0f, 1.0f);  // ceiling white
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(unsigned int)));
            glUniform1i(glGetUniformLocation(roomShader, "isSnowSurface"), 0);
            glUniform1i(glGetUniformLocation(roomShader, "isSnowSurface"), 1);
            glUniform3f(glGetUniformLocation(roomShader, "wallColor"), 0.68f, 0.85f, 0.95f);  // light blue all walls
            glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, (void*)(12 * sizeof(unsigned int)));
            glUniform1i(glGetUniformLocation(roomShader, "isSnowSurface"), 0);

            // Grey forest trees - trunk base at floor, overlapping like back wall on all walls
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);
            glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 0.4f, 0.4f, 0.45f);
            glBindVertexArray(forestTreeVAO);
            // Back wall - trees with overlap, trunk base at floor (y=0)
            float backZ[] = {-4.2f,-3.8f,-2.9f,-2.1f,-1.8f,-0.5f,0.2f,0.8f,1.5f,2.0f,2.8f,3.8f,4.1f};
            float backY[] = {0.0f,0.5f,0.9f,1.3f,1.8f,2.2f,2.6f,3.0f,3.5f};
            for (int i = 0; i < 13; i++) {
                for (int j = 0; j < 9; j++) {
                    glm::mat4 T = glm::mat4(1.0f);
                    T = glm::translate(T, glm::vec3(24.99f, backY[j], backZ[i]));
                    T = glm::rotate(T, glm::radians(-90.0f), glm::vec3(0, 1, 0));
                    T = glm::scale(T, glm::vec3(0.6f + (i + j) % 4 * 0.25f));
                    glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(T));
                    glDrawElements(GL_TRIANGLES, forestTreeIdx.size(), GL_UNSIGNED_INT, 0);
                }
            }
            // Left wall - same overlap logic, trunk at floor
            float leftX[] = {6.2f,7.5f,8.8f,10.2f,11.5f,12.9f,14.2f,15.6f,16.9f,18.3f,19.6f,21.0f,22.3f,23.7f};
            float leftY[] = {0.0f,0.5f,0.9f,1.3f,1.8f,2.2f,2.6f,3.0f,3.5f};
            for (int i = 0; i < 14; i++) {
                for (int j = 0; j < 9; j++) {
                    glm::mat4 T = glm::mat4(1.0f);
                    T = glm::translate(T, glm::vec3(leftX[i], leftY[j], -4.99f));
                    T = glm::scale(T, glm::vec3(0.6f + (i + j) % 4 * 0.25f));
                    glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(T));
                    glDrawElements(GL_TRIANGLES, forestTreeIdx.size(), GL_UNSIGNED_INT, 0);
                }
            }
            // Right wall - same overlap logic
            for (int i = 0; i < 14; i++) {
                for (int j = 0; j < 9; j++) {
                    glm::mat4 T = glm::mat4(1.0f);
                    T = glm::translate(T, glm::vec3(leftX[i], leftY[j], 4.99f));
                    T = glm::rotate(T, glm::radians(180.0f), glm::vec3(0, 1, 0));
                    T = glm::scale(T, glm::vec3(0.6f + (i + j) % 4 * 0.25f));
                    glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(T));
                    glDrawElements(GL_TRIANGLES, forestTreeIdx.size(), GL_UNSIGNED_INT, 0);
                }
            }
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);

            // Falling snow - white circles
            for (int i = 0; i < N_SNOW; i++) {
                snowPositions[i].y -= snowSpeeds[i] * deltaTime;
                if (snowPositions[i].y < 0.0f) {
                    snowPositions[i].y = 4.9f;
                    snowPositions[i].x = snowX(snowRng);
                    snowPositions[i].z = snowZ(snowRng);
                }
            }
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);
            glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 1.0f, 1.0f, 1.0f);
            glBindVertexArray(sphereVAO);
            for (int i = 0; i < N_SNOW; i++) {
                glm::mat4 snowM = glm::mat4(1.0f);
                snowM = glm::translate(snowM, snowPositions[i]);
                snowM = glm::scale(snowM, glm::vec3(0.04f));
                glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(snowM));
                glDrawElements(GL_TRIANGLES, sphereIdx.size(), GL_UNSIGNED_INT, 0);
            }
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);

            glUniform3f(glGetUniformLocation(roomShader, "wallColor"), 0.55f, 0.35f, 0.20f);
        }

        // === Draw Door Panels (fill door opening; slide open when door area clicked or E pressed) ===
        {
            float slideDist = doorOpenAmount * 1.5f;
            float leftPanelZ = -0.625f - slideDist;
            float rightPanelZ = 0.625f + slideDist;
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);
            glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 0.55f, 0.35f, 0.20f);  // match wall
            glBindVertexArray(doorVAO);
            glm::mat4 doorLeft = glm::mat4(1.0f);
            doorLeft = glm::translate(doorLeft, glm::vec3(5.0f, 1.5f, leftPanelZ));
            doorLeft = glm::rotate(doorLeft, glm::radians(-90.0f), glm::vec3(0, 1, 0));
            glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(doorLeft));
            glDrawElements(GL_TRIANGLES, doorIdx.size(), GL_UNSIGNED_INT, 0);
            glm::mat4 doorRight = glm::mat4(1.0f);
            doorRight = glm::translate(doorRight, glm::vec3(5.0f, 1.5f, rightPanelZ));
            doorRight = glm::rotate(doorRight, glm::radians(-90.0f), glm::vec3(0, 1, 0));
            glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(doorRight));
            glDrawElements(GL_TRIANGLES, doorIdx.size(), GL_UNSIGNED_INT, 0);
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);
        }

        // Draw tree
        glUniform1i(glGetUniformLocation(roomShader, "isTree"), 1);

        glm::mat4 treeModel = glm::mat4(1.0f);
        treeModel = glm::translate(treeModel, glm::vec3(0.0f, 0.0f, 0.0f));
        //treeModel = glm::translate(treeModel, glm::vec3(0.0f, coneH *(treeGrowth - 1.0f), 0.0f)); // grow from ground up
        //treeModel = glm::translate(treeModel, glm::vec3(treeGrowth));
        treeModel = glm::scale(treeModel, glm::vec3(treeGrowth));                 // optional

        glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(treeModel));

        glBindVertexArray(treeVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)treeIdx.size(), GL_UNSIGNED_INT, 0);

        // ===================== DRAW ORNAMENTS =====================
        glBindVertexArray(sphereVAO);

        for (int i = 0; i < N_ORNAMENTS; i++) {
            float ornamentRadius = 0.05f + 0.02f + sin(i*12.3f);
            glm::mat4 M = treeModel;
            M = glm::translate(M, ornamentPositions[i]);
            M = glm::scale(M, glm::vec3(ornamentRadius));   // ornament size

            glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(M));
            glUniform3fv(glGetUniformLocation(roomShader, "overrideColor"), 1, glm::value_ptr(ornamentColors[i]));
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);

            glDrawElements(GL_TRIANGLES, sphereIdx.size(), GL_UNSIGNED_INT, 0);
        }

        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);

        // ===================== DRAW LIGHT BULBS =====================
        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);
        glUniform1i(glGetUniformLocation(roomShader, "isEmissive"), 1);
        glUniform1i(glGetUniformLocation(roomShader, "isTree"), 0);
        glUniform1f(glGetUniformLocation(roomShader, "emissiveStrength"), 6.0f);
        glUniform3fv(glGetUniformLocation(roomShader, "overrideColor"), 1, glm::value_ptr(glm::vec3(1.0f))); // white if no color  
        
        

        for (int i = 0; i < N_LIGHTS; i++) {
            
            glm::mat4 M = treeModel;
            M = glm::translate(M, lightPositions[i]);
            M = glm::scale(M, glm::vec3(0.09f));   // light bulb size

            glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(M));
            glUniform3fv(glGetUniformLocation(roomShader, "overrideColor"), 1, glm::value_ptr(lightColors[i]));
            //glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);

            glDrawElements(GL_TRIANGLES, sphereIdx.size(), GL_UNSIGNED_INT, 0);
        }

        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);
        glUniform1i(glGetUniformLocation(roomShader, "isEmissive"), 0);
        glUniform1f(glGetUniformLocation(roomShader, "emissiveStrength"), 1.0f);

        // ===================== DRAW PRESENTS =====================
        glBindVertexArray(cubeVAO);

        for (int i = 0; i < N_PRESENTS; i++)
        {
            glUniform1i(glGetUniformLocation(roomShader, "isPresent"), 1);
            glUniform1i(glGetUniformLocation(roomShader, "presentType"), presentTypes[i]);

            // --- MODEL TRANSFORM ---
            glm::mat4 P = glm::mat4(1.0f);
            P = glm::translate(P, presentPositions[i]);
            P = glm::scale(P, presentScales[i]);

            glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"),
                            1, GL_FALSE, glm::value_ptr(P));

            // --- SEND PRESENT TYPE (0 = solid, 1 = candy cane, 2 = bow) ---
            glUniform1i(glGetUniformLocation(roomShader, "presentType"),
                        presentTypes[i]);

            // --- SOLID WRAPPING COLOR (used for type 0 & type 2 bow boxes) ---
            glUniform3fv(glGetUniformLocation(roomShader, "solidColor"), 1,
                        glm::value_ptr(presentColors[i]));

            // --- CANDY CANE STRIPE COLORS ---
            glUniform3f(glGetUniformLocation(roomShader, "stripe1"), 1.0f, 1.0f, 1.0f); // white stripe
            glUniform3f(glGetUniformLocation(roomShader, "stripe2"), 1.0f, 0.1f, 0.1f); // red stripe

            // --- PRESENTS DO NOT USE overrideColor ---
            glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);

            glDrawElements(GL_TRIANGLES, cubeIdx.size(), GL_UNSIGNED_INT, 0);
            glUniform1i(glGetUniformLocation(roomShader, "isPresent"), 0);
        }


        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);


        // ===================== DRAW STAR =====================
        glBindVertexArray(starVAO);

        glm::mat4 starM = treeModel;
        starM = glm::translate(starM, starPos);
        starM = glm::scale(starM, glm::vec3(starScale));

        glUniformMatrix4fv(glGetUniformLocation(roomShader, "model"), 1, GL_FALSE, glm::value_ptr(starM));
        glUniform3f(glGetUniformLocation(roomShader, "overrideColor"), 1.0f, 0.9f, 0.3f);
        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 1);
        glDrawElements(GL_TRIANGLES, starIdx.size(), GL_UNSIGNED_INT, 0);

        glUniform1i(glGetUniformLocation(roomShader, "useOverrideColor"), 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
 