#include "grandfather_clock_geometry.h"
#include "vertex.h"
#include <glm/glm.hpp>
#include <cmath>

static void addQuadIdx(std::vector<unsigned int>& i, size_t base) {
    unsigned int b = static_cast<unsigned int>(base);
    i.insert(i.end(), {b, b+1, b+2, b, b+2, b+3});
}

static void addTriIdx(std::vector<unsigned int>& i, size_t base) {
    unsigned int b = static_cast<unsigned int>(base);
    i.insert(i.end(), {b, b+1, b+2});
}

static void addBox(std::vector<Vertex>& v, std::vector<unsigned int>& i,
    float x0, float y0, float z0, float x1, float y1, float z1)
{
    size_t base = v.size();
    glm::vec3 n;
    // Front (z1)
    n = {0, 0, 1};
    v.push_back({{x0,y0,z1}, n, {0,0}});
    v.push_back({{x1,y0,z1}, n, {1,0}});
    v.push_back({{x1,y1,z1}, n, {1,1}});
    v.push_back({{x0,y1,z1}, n, {0,1}});
    addQuadIdx(i, base);
    base = v.size();
    // Back (z0)
    n = {0, 0, -1};
    v.push_back({{x1,y0,z0}, n, {0,0}});
    v.push_back({{x0,y0,z0}, n, {1,0}});
    v.push_back({{x0,y1,z0}, n, {1,1}});
    v.push_back({{x1,y1,z0}, n, {0,1}});
    addQuadIdx(i, base);
    base = v.size();
    // Right (x1)
    n = {1, 0, 0};
    v.push_back({{x1,y0,z1}, n, {0,0}});
    v.push_back({{x1,y0,z0}, n, {1,0}});
    v.push_back({{x1,y1,z0}, n, {1,1}});
    v.push_back({{x1,y1,z1}, n, {0,1}});
    addQuadIdx(i, base);
    base = v.size();
    // Left (x0)
    n = {-1, 0, 0};
    v.push_back({{x0,y0,z0}, n, {0,0}});
    v.push_back({{x0,y0,z1}, n, {1,0}});
    v.push_back({{x0,y1,z1}, n, {1,1}});
    v.push_back({{x0,y1,z0}, n, {0,1}});
    addQuadIdx(i, base);
    base = v.size();
    // Top (y1)
    n = {0, 1, 0};
    v.push_back({{x0,y1,z1}, n, {0,0}});
    v.push_back({{x1,y1,z1}, n, {1,0}});
    v.push_back({{x1,y1,z0}, n, {1,1}});
    v.push_back({{x0,y1,z0}, n, {0,1}});
    addQuadIdx(i, base);
    base = v.size();
    // Bottom (y0)
    n = {0, -1, 0};
    v.push_back({{x0,y0,z0}, n, {0,0}});
    v.push_back({{x1,y0,z0}, n, {1,0}});
    v.push_back({{x1,y0,z1}, n, {1,1}});
    v.push_back({{x0,y0,z1}, n, {0,1}});
    addQuadIdx(i, base);
}

void GenerateGrandfatherClockBody(std::vector<Vertex>& verts,
                                  std::vector<unsigned int>& idx)
{
    verts.clear();
    idx.clear();

    float depth = 0.3f;   // how far clock extends from wall
    float zBack = -depth;
    float zFront = 0.02f;

    // Foot: wide short at bottom (y 0 to 0.35)
    float footW = 0.7f;
    float footH = 0.35f;
    addBox(verts, idx, -footW/2, 0, zBack, footW/2, footH, zFront);

    // Body: skinny tall (y 0.35 to 2.4)
    float bodyW = 0.18f;
    float bodyH = 2.05f;
    addBox(verts, idx, -bodyW/2, footH, zBack, bodyW/2, footH + bodyH, zFront);

    // Square: wider (y 2.4 to 2.9), same width as triangle
    float sqW = 0.55f;
    float sqH = 0.5f;
    addBox(verts, idx, -sqW/2, footH + bodyH, zBack, sqW/2, footH + bodyH + sqH, zFront);

    // Triangle: prism, same width as square (y 2.9 to 3.45)
    float triW = sqW;
    float triBottom = footH + bodyH + sqH;
    float triTop = triBottom + 0.55f;
    glm::vec3 nF(0, 0, 1);
    size_t baseIdx = verts.size();
    // Front triangle
    verts.push_back({{-triW/2, triBottom, zFront}, nF, {0,0}});
    verts.push_back({{triW/2, triBottom, zFront}, nF, {1,0}});
    verts.push_back({{0, triTop, zFront}, nF, {0.5f,1}});
    addTriIdx(idx, baseIdx);
    baseIdx = verts.size();
    // Back triangle
    glm::vec3 nB(0, 0, -1);
    verts.push_back({{triW/2, triBottom, zBack}, nB, {0,0}});
    verts.push_back({{-triW/2, triBottom, zBack}, nB, {1,0}});
    verts.push_back({{0, triTop, zBack}, nB, {0.5f,1}});
    addTriIdx(idx, baseIdx);
    // 3 side quads of prism
    baseIdx = verts.size();
    glm::vec3 nR(1,0,0), nL(-1,0,0), nT(0,1,0);
    verts.push_back({{-triW/2, triBottom, zFront}, nL, {0,0}});
    verts.push_back({{-triW/2, triBottom, zBack}, nL, {1,0}});
    verts.push_back({{0, triTop, zBack}, nL, {0.5f,1}});
    verts.push_back({{0, triTop, zFront}, nL, {0.5f,1}});
    addQuadIdx(idx, baseIdx);
    baseIdx = verts.size();
    verts.push_back({{triW/2, triBottom, zBack}, nR, {0,0}});
    verts.push_back({{triW/2, triBottom, zFront}, nR, {1,0}});
    verts.push_back({{0, triTop, zFront}, nR, {0.5f,1}});
    verts.push_back({{0, triTop, zBack}, nR, {0.5f,1}});
    addQuadIdx(idx, baseIdx);
    baseIdx = verts.size();
    verts.push_back({{-triW/2, triBottom, zBack}, nT, {0,0}});
    verts.push_back({{triW/2, triBottom, zBack}, nT, {1,0}});
    verts.push_back({{triW/2, triBottom, zFront}, nT, {1,1}});
    verts.push_back({{-triW/2, triBottom, zFront}, nT, {0,1}});
    addQuadIdx(idx, baseIdx);
}

void GenerateClockFace(std::vector<Vertex>& verts,
                      std::vector<unsigned int>& idx)
{
    verts.clear();
    idx.clear();

    // White circle on front of square - center at (0, 2.65, 0.03), radius 0.2
    float cx = 0, cy = 2.65f, cz = 0.03f;
    float radius = 0.2f;
    int segments = 32;
    glm::vec3 n(0, 0, 1);

    size_t centerIdx = verts.size();
    verts.push_back({{cx, cy, cz}, n, {0.5f, 0.5f}});

    for (int i = 0; i < segments; i++) {
        float t = (float)i / segments * 2.0f * 3.14159265f;
        float x = cx + cosf(t) * radius;
        float y = cy + sinf(t) * radius;
        verts.push_back({{x, y, cz}, n, {0.5f + 0.5f*cosf(t), 0.5f + 0.5f*sinf(t)}});
    }

    for (int i = 0; i < segments; i++) {
        idx.push_back(static_cast<unsigned int>(centerIdx));
        idx.push_back(static_cast<unsigned int>(centerIdx + 1 + i));
        idx.push_back(static_cast<unsigned int>(centerIdx + 1 + (i + 1) % segments));
    }
}

void GenerateClockHands(std::vector<Vertex>& verts,
                       std::vector<unsigned int>& idx)
{
    verts.clear();
    idx.clear();

    float cx = 0, cy = 2.65f, cz = 0.035f;  // slightly in front of face
    float hw = 0.008f;  // half width of hand
    glm::vec3 n(0, 0, 1);

    // Hour hand: shorter, pointing up-left (~10 o'clock, -120 deg from right)
    float hourLen = 0.12f;
    float hourAng = -2.0f;  // radians, roughly 10 o'clock
    float hx = cx + cosf(hourAng) * hourLen;
    float hy = cy + sinf(hourAng) * hourLen;
    float perpX = -sinf(hourAng) * hw;
    float perpY = cosf(hourAng) * hw;
    size_t base = verts.size();
    verts.push_back({{cx - perpX, cy - perpY, cz}, n, {0,0}});
    verts.push_back({{cx + perpX, cy + perpY, cz}, n, {1,0}});
    verts.push_back({{hx + perpX, hy + perpY, cz}, n, {1,1}});
    verts.push_back({{hx - perpX, hy - perpY, cz}, n, {0,1}});
    addQuadIdx(idx, base);

    // Minute hand: longer, pointing right (3 o'clock)
    float minLen = 0.18f;
    base = verts.size();
    verts.push_back({{cx, cy - hw, cz}, n, {0,0}});
    verts.push_back({{cx, cy + hw, cz}, n, {1,0}});
    verts.push_back({{cx + minLen, cy + hw, cz}, n, {1,1}});
    verts.push_back({{cx + minLen, cy - hw, cz}, n, {0,1}});
    addQuadIdx(idx, base);
}
