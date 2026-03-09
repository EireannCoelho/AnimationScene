#include "forest_tree_geometry.h"
#include "vertex.h"
#include <glm/glm.hpp>

void GenerateForestTreeMesh(std::vector<Vertex>& verts,
                            std::vector<unsigned int>& idx,
                            float width, float height)
{
    verts.clear();
    idx.clear();

    float trunkW = width * 0.35f;  // wider trunk
    float trunkH = height * 0.35f;

    // Trunk (rectangle) - centered, bottom
    float left = -trunkW * 0.5f;
    float right = trunkW * 0.5f;
    verts.push_back({{left, 0, 0}, {0, 0, 1}, {0, 0}});
    verts.push_back({{right, 0, 0}, {0, 0, 1}, {1, 0}});
    verts.push_back({{right, trunkH, 0}, {0, 0, 1}, {1, 1}});
    verts.push_back({{left, trunkH, 0}, {0, 0, 1}, {0, 1}});
    idx.insert(idx.end(), {0, 1, 2, 0, 2, 3});

    // Foliage (triangle) - on top of trunk, wider
    float tipY = height;
    float baseY = trunkH;
    float halfW = width * 0.65f;  // wider foliage
    verts.push_back({{-halfW, baseY, 0}, {0, 0, 1}, {0, 0}});
    verts.push_back({{halfW, baseY, 0}, {0, 0, 1}, {1, 0}});
    verts.push_back({{0, tipY, 0}, {0, 0, 1}, {0.5f, 1}});
    idx.insert(idx.end(), {4, 5, 6});
}
