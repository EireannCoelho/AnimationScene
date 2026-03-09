#include "window_geometry.h"
#include "vertex.h"
#include <glm/glm.hpp>

void GenerateWindowMesh(std::vector<Vertex>& verts,
                        std::vector<unsigned int>& idx,
                        float width, float height)
{
    verts.clear();
    idx.clear();

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    // Window quad: centered at origin in XY, facing +Z (toward room)
    // Vertices in local space: x left-right, y up-down, z forward
    glm::vec3 positions[] = {
        {-hw, -hh, 0}, {hw, -hh, 0}, {hw, hh, 0}, {-hw, hh, 0}
    };
    glm::vec3 normal(0, 0, 1);
    glm::vec2 uvs[] = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    };

    for (int i = 0; i < 4; i++) {
        verts.push_back({positions[i], normal, uvs[i]});
    }
    idx = {0, 1, 2, 0, 2, 3};
}
