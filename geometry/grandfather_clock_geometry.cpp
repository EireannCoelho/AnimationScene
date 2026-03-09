#include "grandfather_clock_geometry.h"
#include "vertex.h"
#include <glm/glm.hpp>

static void addQuad(std::vector<Vertex>& v, std::vector<unsigned int>& i,
    glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 norm)
{
    size_t base = v.size();
    v.push_back({a, norm, {0,0}});
    v.push_back({b, norm, {1,0}});
    v.push_back({c, norm, {1,1}});
    v.push_back({d, norm, {0,1}});
    i.insert(i.end(), {
        static_cast<unsigned int>(base), static_cast<unsigned int>(base+1),
        static_cast<unsigned int>(base+2), static_cast<unsigned int>(base),
        static_cast<unsigned int>(base+2), static_cast<unsigned int>(base+3)
    });
}

void GenerateGrandfatherClockMesh(std::vector<Vertex>& verts,
                                  std::vector<unsigned int>& idx)
{
    verts.clear();
    idx.clear();

    float depth = 0.08f;  // 3D depth from wall
    glm::vec3 nF(0, 0, 1), nR(1, 0, 0), nL(-1, 0, 0), nT(0, 1, 0), nB(0, -1, 0);

    // Order: body (skinny) at bottom, square on top, triangle on top
    // Body: skinny tall rectangle (y 0 to 2.5, x -0.15 to 0.15)
    float bodyW = 0.15f;
    float bodyH = 2.5f;
    addQuad(verts, idx, {-bodyW,0,0}, {bodyW,0,0}, {bodyW,bodyH,0}, {-bodyW,bodyH,0}, nF);
    addQuad(verts, idx, {bodyW,0,0}, {bodyW,0,-depth}, {bodyW,bodyH,-depth}, {bodyW,bodyH,0}, nR);
    addQuad(verts, idx, {-bodyW,0,-depth}, {-bodyW,0,0}, {-bodyW,bodyH,0}, {-bodyW,bodyH,-depth}, nL);

    // Square: wider (y 2.5 to 2.9, x -0.35 to 0.35)
    float sqW = 0.35f;
    float sqBottom = bodyH;
    float sqTop = 2.9f;
    addQuad(verts, idx, {-sqW,sqBottom,0}, {sqW,sqBottom,0}, {sqW,sqTop,0}, {-sqW,sqTop,0}, nF);
    addQuad(verts, idx, {sqW,sqBottom,0}, {sqW,sqBottom,-depth}, {sqW,sqTop,-depth}, {sqW,sqTop,0}, nR);
    addQuad(verts, idx, {-sqW,sqBottom,-depth}, {-sqW,sqBottom,0}, {-sqW,sqTop,0}, {-sqW,sqTop,-depth}, nL);

    // Triangle (y 2.9 to 3.5) - front face only
    float triBottom = sqTop;
    float triTip = 3.5f;
    float triW = 0.3f;
    size_t baseIdx = verts.size();
    verts.push_back({{-triW, triBottom, 0}, nF, {0,0}});
    verts.push_back({{triW, triBottom, 0}, nF, {1,0}});
    verts.push_back({{0, triTip, 0}, nF, {0.5f,1}});
    idx.insert(idx.end(), {
        static_cast<unsigned int>(baseIdx), static_cast<unsigned int>(baseIdx+1),
        static_cast<unsigned int>(baseIdx+2)
    });
}
