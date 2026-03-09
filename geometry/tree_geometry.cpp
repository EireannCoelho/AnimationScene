#include <vector>
#include "vertex.h"
#include "tree_geometry.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

// Simple noise function for branch needles
static float noise(float x, float y)
{
    return 0.5f * (sin(x * 12.9898f + y * 78.233f) * 43758.5453f -
                   floor(sin(x * 12.9898f + y * 78.233f) * 43758.5453f));
}

// Add a vertex
static void addV(std::vector<Vertex>& v,
                 const glm::vec3& pos,
                 const glm::vec3& normal,
                 const glm::vec2& uv)
{
    Vertex vert;
    vert.pos = pos;
    vert.normal   = normal;
    vert.uv       = uv;
    v.push_back(vert);

}

//Build trunk -------------------------------------------------------------------
static void buildCylinder(float R, float H, int seg, float y0,
                          std::vector<Vertex>& verts,
                          std::vector<unsigned>& idx)
{
    int base = verts.size();

    for (int i = 0; i <= seg; i++) {
        float t = float(i) / seg;
        float a = t * glm::two_pi<float>();

        float x = cos(a), z = sin(a);
        glm::vec3 n = glm::normalize(glm::vec3(x, 0, z));

        addV(verts, glm::vec3(R*x, y0,       R*z), n, glm::vec2(t, 0));
        addV(verts, glm::vec3(R*x, y0 + H,   R*z), n, glm::vec2(t, 1));
    }

    for (int i = 0; i < seg; i++) {
        int i0 = base + 2*i;
        int i1 = i0 + 1;
        int i2 = i0 + 2;
        int i3 = i0 + 3;
        idx.push_back(i0); idx.push_back(i2); idx.push_back(i1);
        idx.push_back(i1); idx.push_back(i2); idx.push_back(i3);
    }
}

// Build big cone with noisy needle normals --------------------------------------
static void buildNeedleCone(float radius, float height, int segments, float yOffset,
                            std::vector<Vertex>& verts,
                            std::vector<unsigned>& idx)
{
    int base = verts.size();
    glm::vec3 tip = glm::vec3(0, yOffset + height, 0);

    // ring vertices
    for (int i = 0; i <= segments; i++) {
        float t = float(i) / segments;
        float ang = t * glm::two_pi<float>();

        float x = cos(ang);
        float z = sin(ang);
        glm::vec3 pos = glm::vec3(radius*x, yOffset, radius*z);

        // Basic cone normal
        glm::vec3 normal = glm::normalize(glm::vec3(x, radius/height, z));

        // Add needle noise (perturb normal)
        float n = noise(x * 3.0f, z * 3.0f);
        normal += glm::vec3(x * n * 0.7f, n * 0.3f, z * n * 0.7f);
        normal = glm::normalize(normal);

        addV(verts, pos, normal, glm::vec2(t, 0));
    }

    // tip vertex
    int tipIndex = verts.size();
    addV(verts, tip, glm::vec3(0, 1, 0), glm::vec2(0.5f, 1));

    for (int i = 0; i < segments; i++) {
        int i0 = base + i;
        int i1 = base + i + 1;
        idx.push_back(i0);
        idx.push_back(i1);
        idx.push_back(tipIndex);
    }
}

// Build tapered trunk with bark-like texture
static void buildTrunk(float baseR, float topR, float H, int seg, float y0,
                       std::vector<Vertex>& verts,
                       std::vector<unsigned>& idx)
{
    int base = verts.size();

    for (int i = 0; i <= seg; i++) {
        float t = float(i) / seg;
        float a = t * glm::two_pi<float>();

        float x = cos(a), z = sin(a);
        float R_bot = baseR * (1.0f + noise(x * 5.0f, z * 5.0f) * 0.15f);
        float R_top = topR * (1.0f + noise(x * 5.0f + 1.0f, z * 5.0f + 1.0f) * 0.1f);

        glm::vec3 n = glm::normalize(glm::vec3(x, 0, z));
        addV(verts, glm::vec3(R_bot*x, y0, R_bot*z), n, glm::vec2(t, 0));
        addV(verts, glm::vec3(R_top*x, y0 + H, R_top*z), n, glm::vec2(t, 1));
    }

    for (int i = 0; i < seg; i++) {
        int i0 = base + 2*i;
        int i1 = i0 + 1;
        int i2 = i0 + 2;
        int i3 = i0 + 3;
        idx.push_back(i0); idx.push_back(i2); idx.push_back(i1);
        idx.push_back(i1); idx.push_back(i2); idx.push_back(i3);
    }
}

// Main entry ---------------------------------------------------------------------
void GenerateTreeGeometry(std::vector<Vertex>& verts,
                          std::vector<unsigned>& idx)
{
    verts.clear();
    idx.clear();

    int seg = 48;

    // Realistic tree: trunk + layered foliage cones
    float trunkH = 0.6f;
    float trunkBaseR = 0.35f;
    float trunkTopR = 0.28f;

    buildTrunk(trunkBaseR, trunkTopR, trunkH, seg, 0.0f, verts, idx);

    // Bottom foliage layer (widest)
    float layer0Y = trunkH;
    float layer0H = 1.4f;
    float layer0R = 1.8f;
    buildNeedleCone(layer0R, layer0H, seg, layer0Y, verts, idx);

    // Middle foliage layer
    float layer1Y = layer0Y + layer0H * 0.7f;  // overlap slightly
    float layer1H = 1.2f;
    float layer1R = 1.3f;
    buildNeedleCone(layer1R, layer1H, seg, layer1Y, verts, idx);

    // Upper foliage layer
    float layer2Y = layer1Y + layer1H * 0.6f;
    float layer2H = 1.0f;
    float layer2R = 0.9f;
    buildNeedleCone(layer2R, layer2H, seg, layer2Y, verts, idx);

    // Top cone (tree tip)
    float layer3Y = layer2Y + layer2H * 0.5f;
    float layer3H = 0.8f;
    float layer3R = 0.5f;
    buildNeedleCone(layer3R, layer3H, seg, layer3Y, verts, idx);
}
