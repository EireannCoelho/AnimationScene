#include "mouse_geometry.h"
#include "vertex.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

static void addV(std::vector<Vertex>& v, const glm::vec3& pos,
                 const glm::vec3& normal, const glm::vec2& uv)
{
    v.push_back({pos, normal, uv});
}

void GenerateMouseMesh(std::vector<Vertex>& verts,
                       std::vector<unsigned int>& idx)
{
    verts.clear();
    idx.clear();

    // Mouse: ellipsoid body (elongated) + two ear spheres
    int latDiv = 8;
    int lonDiv = 12;
    float bodyRx = 0.08f;  // length (along run direction)
    float bodyRy = 0.06f;  // height
    float bodyRz = 0.05f;  // width

    int base = 0;
    for (int i = 0; i <= latDiv; i++) {
        float v = float(i) / latDiv;
        float theta = v * glm::pi<float>();
        for (int j = 0; j <= lonDiv; j++) {
            float u = float(j) / lonDiv;
            float phi = u * glm::two_pi<float>();

            float x = sin(theta) * cos(phi) * bodyRx;
            float y = cos(theta) * bodyRy;
            float z = sin(theta) * sin(phi) * bodyRz;

            glm::vec3 pos(x, y, z);
            glm::vec3 normal = glm::normalize(glm::vec3(
                x / (bodyRx * bodyRx),
                y / (bodyRy * bodyRy),
                z / (bodyRz * bodyRz)
            ));
            addV(verts, pos, normal, glm::vec2(u, v));
        }
    }

    for (int i = 0; i < latDiv; i++) {
        for (int j = 0; j < lonDiv; j++) {
            int row1 = i * (lonDiv + 1);
            int row2 = (i + 1) * (lonDiv + 1);
            idx.push_back(row1 + j);
            idx.push_back(row2 + j);
            idx.push_back(row1 + j + 1);
            idx.push_back(row1 + j + 1);
            idx.push_back(row2 + j);
            idx.push_back(row2 + j + 1);
        }
    }

    // Two simple ear quads (flat ovals) on top
    float earY = bodyRy;
    float earSize = 0.04f;
    int earBase = verts.size();
    glm::vec3 earNorm(0, 1, 0);

    addV(verts, glm::vec3(-bodyRx * 0.5f - earSize, earY, -bodyRz * 0.5f), earNorm, {0, 0});
    addV(verts, glm::vec3(-bodyRx * 0.5f, earY, -bodyRz * 0.5f - earSize), earNorm, {1, 0});
    addV(verts, glm::vec3(-bodyRx * 0.5f + earSize, earY, -bodyRz * 0.5f), earNorm, {1, 1});
    addV(verts, glm::vec3(-bodyRx * 0.5f, earY, -bodyRz * 0.5f + earSize), earNorm, {0, 1});
    idx.push_back(earBase); idx.push_back(earBase + 1); idx.push_back(earBase + 2);
    idx.push_back(earBase); idx.push_back(earBase + 2); idx.push_back(earBase + 3);

    addV(verts, glm::vec3(bodyRx * 0.5f - earSize, earY, -bodyRz * 0.5f), earNorm, {0, 0});
    addV(verts, glm::vec3(bodyRx * 0.5f, earY, -bodyRz * 0.5f - earSize), earNorm, {1, 0});
    addV(verts, glm::vec3(bodyRx * 0.5f + earSize, earY, -bodyRz * 0.5f), earNorm, {1, 1});
    addV(verts, glm::vec3(bodyRx * 0.5f, earY, -bodyRz * 0.5f + earSize), earNorm, {0, 1});
    idx.push_back(earBase + 4); idx.push_back(earBase + 5); idx.push_back(earBase + 6);
    idx.push_back(earBase + 4); idx.push_back(earBase + 6); idx.push_back(earBase + 7);
}
