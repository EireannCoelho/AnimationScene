#include "picture_frame_geometry.h"
#include "vertex.h"
#include <glm/glm.hpp>

static void addV(std::vector<Vertex>& v, const glm::vec3& pos,
                 const glm::vec3& normal, const glm::vec2& uv)
{
    v.push_back({pos, normal, uv});
}

void GeneratePictureFrameMesh(std::vector<Vertex>& verts,
                              std::vector<unsigned int>& idx,
                              size_t& frameIndexCount,
                              float frameWidth, float frameHeight,
                              float frameDepth)
{
    verts.clear();
    idx.clear();

    float hw = frameWidth * 0.5f;
    float hh = frameHeight * 0.5f;
    float border = 0.08f;  // frame border width
    float innerW = frameWidth - 2 * border;
    float innerH = frameHeight - 2 * border;

    // Frame: front face (facing into room) - outer rect
    // Local: X right, Y up, Z forward (into room)
    int base = 0;

    // Front face - frame border (4 rectangles around the canvas)
    // Top border
    addV(verts, {-hw, hh - border, 0}, {0, 0, 1}, {0, 0});
    addV(verts, {hw, hh - border, 0}, {0, 0, 1}, {1, 0});
    addV(verts, {hw, hh, 0}, {0, 0, 1}, {1, 1});
    addV(verts, {-hw, hh, 0}, {0, 0, 1}, {0, 1});
    idx.insert(idx.end(), {0, 1, 2, 0, 2, 3});

    // Bottom border
    addV(verts, {-hw, -hh, 0}, {0, 0, 1}, {0, 0});
    addV(verts, {hw, -hh, 0}, {0, 0, 1}, {1, 0});
    addV(verts, {hw, -hh + border, 0}, {0, 0, 1}, {1, 1});
    addV(verts, {-hw, -hh + border, 0}, {0, 0, 1}, {0, 1});
    idx.insert(idx.end(), {4, 5, 6, 4, 6, 7});

    // Left border
    addV(verts, {-hw, -hh + border, 0}, {0, 0, 1}, {0, 0});
    addV(verts, {-hw + border, -hh + border, 0}, {0, 0, 1}, {1, 0});
    addV(verts, {-hw + border, hh - border, 0}, {0, 0, 1}, {1, 1});
    addV(verts, {-hw, hh - border, 0}, {0, 0, 1}, {0, 1});
    idx.insert(idx.end(), {8, 9, 10, 8, 10, 11});

    // Right border
    addV(verts, {hw - border, -hh + border, 0}, {0, 0, 1}, {0, 0});
    addV(verts, {hw, -hh + border, 0}, {0, 0, 1}, {1, 0});
    addV(verts, {hw, hh - border, 0}, {0, 0, 1}, {1, 1});
    addV(verts, {hw - border, hh - border, 0}, {0, 0, 1}, {0, 1});
    idx.insert(idx.end(), {12, 13, 14, 12, 14, 15});

    // Canvas (painting surface) - inset, slightly recessed
    float canvasZ = -frameDepth * 0.3f;
    int cBase = verts.size();
    addV(verts, {-hw + border, -hh + border, canvasZ}, {0, 0, 1}, {0, 0});
    addV(verts, {hw - border, -hh + border, canvasZ}, {0, 0, 1}, {1, 0});
    addV(verts, {hw - border, hh - border, canvasZ}, {0, 0, 1}, {1, 1});
    addV(verts, {-hw + border, hh - border, canvasZ}, {0, 0, 1}, {0, 1});
    idx.insert(idx.end(), {
        static_cast<unsigned int>(cBase), static_cast<unsigned int>(cBase + 1),
        static_cast<unsigned int>(cBase + 2), static_cast<unsigned int>(cBase),
        static_cast<unsigned int>(cBase + 2), static_cast<unsigned int>(cBase + 3)
    });

    frameIndexCount = 24;  // 4 border quads * 6 indices
}
