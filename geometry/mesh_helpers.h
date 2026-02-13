#pragma once
#include <vector>
#include "vertex.h"
#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>



// Generate a UV sphere (latitude-longitude)
inline void MakeSphereMesh(int latDiv, int lonDiv,
                           std::vector<Vertex>& outVerts,
                           std::vector<unsigned int>& outIdx,
                           float radius = 1.0f)
{
    outVerts.clear();
    outIdx.clear();

    for (int i = 0; i <= latDiv; ++i) {
        float v = float(i) / latDiv;
        float theta = v * glm::pi<float>();
        for (int j = 0; j <= lonDiv; ++j) {
            float u = float(j) / lonDiv;
            float phi = u * glm::two_pi<float>();

            float x = sin(theta) * cos(phi);
            float y = cos(theta);
            float z = sin(theta) * sin(phi);
            glm::vec3 pos = glm::vec3(x, y, z) * radius;
            glm::vec3 normal = glm::normalize(pos);
            glm::vec2 uv = glm::vec2(u, v);
            Vertex vert; vert.pos = pos; vert.normal = normal; vert.uv = uv;
            outVerts.push_back(vert);
        }
    }

    for (int i = 0; i < latDiv; ++i) {
        for (int j = 0; j < lonDiv; ++j) {
            int row1 = i * (lonDiv + 1);
            int row2 = (i + 1) * (lonDiv + 1);
            outIdx.push_back(row1 + j);
            outIdx.push_back(row2 + j);
            outIdx.push_back(row1 + j + 1);

            outIdx.push_back(row1 + j + 1);
            outIdx.push_back(row2 + j);
            outIdx.push_back(row2 + j + 1);
        }
    }
}

// Simple 3D star (octahedron-like) centered at origin
inline void MakeStarMesh(std::vector<Vertex>& outVerts,
                         std::vector<unsigned int>& outIdx,
                         float radius = 0.25f)
{
    outVerts.clear();
    outIdx.clear();
    // 6 points of octahedron
    glm::vec3 p[] = {
        {0, radius, 0},   // top
        {0, -radius, 0},  // bottom
        {radius, 0, 0},
        {-radius, 0, 0},
        {0, 0, radius},
        {0, 0, -radius}
    };
    // vertices with dummy uv
    for (int i = 0; i < 6; ++i) {
        Vertex v; v.pos = p[i]; v.normal = glm::normalize(p[i]); v.uv = glm::vec2(0.5f, 0.5f);
        outVerts.push_back(v);
    }
    // 8 triangles connecting top and bottom
    unsigned idxs[] = {
        0,2,4, 0,4,3, 0,3,5, 0,5,2,
        1,4,2, 1,3,4, 1,5,3, 1,2,5
    };
    outIdx.insert(outIdx.end(), std::begin(idxs), std::end(idxs));
}

void MakeCubeMesh(std::vector<Vertex>& verts,
                  std::vector<unsigned int>& idx,
                  float size = 1.0f)
{
    float s = size * 0.5f;

    glm::vec3 positions[] = {
        {-s,-s,-s}, {s,-s,-s}, {s, s,-s}, {-s, s,-s}, // back
        {-s,-s, s}, {s,-s, s}, {s, s, s}, {-s, s, s}  // front
    };

    unsigned int faces[] = {
        0,1,2, 2,3,0,   // back
        4,5,6, 6,7,4,   // front
        0,4,7, 7,3,0,   // left
        1,5,6, 6,2,1,   // right
        3,2,6, 6,7,3,   // top
        0,1,5, 5,4,0    // bottom
    };

    verts.clear();
    idx.clear();

    for (int i = 0; i < 8; i++) {
        Vertex v{};
        v.pos = positions[i];
        v.normal = glm::vec3(0,1,0); // temporary normal
        v.uv = glm::vec2(0);         // unused
        verts.push_back(v);
    }

    for (int i = 0; i < 36; i++)
        idx.push_back(faces[i]);
}
