#include "snow_room_tree_geometry.h"
#include "vertex.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

void GenerateSnowRoomTreeTrunk(std::vector<Vertex>& verts,
                               std::vector<unsigned int>& idx,
                               float trunkRadius, float trunkHeight)
{
    verts.clear();
    idx.clear();
    const int seg = 12;
    glm::vec3 n;
    float y0 = 0, y1 = trunkHeight;

    for (int i = 0; i <= seg; i++) {
        float t = (float)i / seg * glm::two_pi<float>();
        float x = cosf(t), z = sinf(t);
        n = glm::normalize(glm::vec3(x, 0, z));
        verts.push_back({{trunkRadius*x, y0, trunkRadius*z}, n, {0, 0}});
        verts.push_back({{trunkRadius*x, y1, trunkRadius*z}, n, {0, 1}});
    }

    for (int i = 0; i < seg; i++) {
        unsigned int a = 2 * i, b = 2 * i + 1, c = 2 * (i + 1) + 1, d = 2 * (i + 1);
        idx.insert(idx.end(), {a, b, c, a, c, d});
    }
}

void GenerateSnowRoomTreeFoliage(std::vector<Vertex>& verts,
                                std::vector<unsigned int>& idx,
                                float baseRadius, float height, float yBase)
{
    verts.clear();
    idx.clear();

    const int seg = 12;
    glm::vec3 tip(0, yBase + height, 0);

    // Base ring vertices - UV.y = 0 (dark green)
    for (int i = 0; i <= seg; i++) {
        float t = (float)i / seg * glm::two_pi<float>();
        float x = cosf(t), z = sinf(t);
        glm::vec3 pos(baseRadius * x, yBase, baseRadius * z);
        glm::vec3 normal = glm::normalize(glm::vec3(x, baseRadius / height, z));
        verts.push_back({pos, normal, {t / glm::two_pi<float>(), 0}});
    }

    // Tip vertex - UV.y = 1 (white snow)
    unsigned int tipIdx = static_cast<unsigned int>(verts.size());
    verts.push_back({tip, glm::vec3(0, 1, 0), {0.5f, 1}});

    // Triangles: base ring to tip
    for (int i = 0; i < seg; i++) {
        idx.push_back(i);
        idx.push_back(i + 1);
        idx.push_back(tipIdx);
    }
}

static void addCone(std::vector<Vertex>& verts, std::vector<unsigned int>& idx,
                    float baseRadius, float height, float yBase, int seg)
{
    size_t baseV = verts.size();
    glm::vec3 tip(0, yBase + height, 0);
    for (int i = 0; i <= seg; i++) {
        float t = (float)i / seg * glm::two_pi<float>();
        float x = cosf(t), z = sinf(t);
        glm::vec3 pos(baseRadius * x, yBase, baseRadius * z);
        glm::vec3 normal = glm::normalize(glm::vec3(x, baseRadius / height, z));
        verts.push_back({pos, normal, {t / glm::two_pi<float>(), 0}});
    }
    unsigned int tipIdx = static_cast<unsigned int>(verts.size());
    verts.push_back({tip, glm::vec3(0, 1, 0), {0.5f, 1}});
    for (int i = 0; i < seg; i++) {
        idx.push_back(static_cast<unsigned int>(baseV + i));
        idx.push_back(static_cast<unsigned int>(baseV + i + 1));
        idx.push_back(tipIdx);
    }
}

void GenerateSnowRoomTreeFoliage2Level(std::vector<Vertex>& verts,
                                      std::vector<unsigned int>& idx,
                                      float baseR1, float h1, float baseR2, float h2, float yBase)
{
    verts.clear();
    idx.clear();
    const int seg = 12;
    addCone(verts, idx, baseR1, h1, yBase, seg);
    // Top cone starts lower (overlap) so it sits on the lower cone, not speared on the tip
    float overlap = 0.55f;  // top cone base at 55% up the bottom cone
    addCone(verts, idx, baseR2, h2, yBase + h1 * overlap, seg);
}
