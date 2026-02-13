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
#include "geometry/tree_geometry.h"
#include "geometry/mesh_helpers.h"
#include "geometry/curtain_geometry.h"


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

    //left mouse cursor to lock again
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        growingTree = true;
        targetTreeGrowth = 2.0f;
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

    // tree cone geometry parameters (match what you used in GenerateTreeGeometry)
    float trunkHeight = 0.0f;
    float coneH = 3.5f;
    float coneR = 2.0f;
    float coneBaseY = 0.0f;

    // deterministic random
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> ud_h(0.05f, 0.95f); // height along cone (bottom=0)
    std::uniform_real_distribution<float> ud_ang(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> ud_color(0.0f, 1.0f);

    // sample positions on cone surface
    for (int i = 0; i < N_ORNAMENTS; ++i) {
        float t = ud_h(rng); // 0..1 up the cone
        // radius at that height (linear taper)
        float localR = coneR * (1.0f - t);
        float ang = ud_ang(rng);
        float x = cos(ang) * localR;
        float z = sin(ang) * localR;
        float y = coneBaseY + t * coneH;
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

    // lights (emissive) — sample near outer cone, fewer
    for (int i = 0; i < N_LIGHTS; ++i) {
        float t = ud_h(rng);
        float localR = coneR * (1.0f - t);
        float ang = ud_ang(rng);
        float x = cos(ang) * localR;
        float z = sin(ang) * localR;
        float y = coneBaseY + t * coneH;
        lightPositions.push_back(glm::vec3(x, y, z));
        // colored lights
        glm::vec3 lc;
        float pick = ud_color(rng);
        if (pick < 0.4f) lc = glm::vec3(1.0f, 0.6f, 0.2f); // warm
        else if (pick < 0.7f) lc = glm::vec3(1.0f, 0.0f, 0.0f);
        else lc = glm::vec3(0.0f, 0.6f, 1.0f);
        lightColors.push_back(lc);
    }

    // star model transform parameters
    glm::vec3 starPos = glm::vec3(0.0f, coneBaseY + coneH + 0.05f, 0.0f);
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


    // --- Main Loop ---
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        if (growingTree) {
            treeGrowth += (targetTreeGrowth - treeGrowth) * deltaTime * 2.0f; // growth speed
            if (fabs(treeGrowth - targetTreeGrowth) < 0.01f) {
                treeGrowth = targetTreeGrowth;
                growingTree = false;
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

        // Draw room
        glUniform1i(glGetUniformLocation(roomShader, "isTree"), 0);
        glUniform3f(glGetUniformLocation(roomShader, "wallColor"),
                    0.55f, 0.35f, 0.20f); 
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, roomIndices.size(), GL_UNSIGNED_INT, 0);

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
 