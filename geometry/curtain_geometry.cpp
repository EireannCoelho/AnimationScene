#include "vertex.h"
#include <glm/glm.hpp>
#include <vector>
#include <cmath>

void GenerateWavyCurtainMesh(std::vector<Vertex>& verts,
                             std::vector<unsigned int>& idx,
                             int segmentsX,
                             int segmentsY)
{
    verts.clear();
    idx.clear();

    float width = 3.5f;      // total curtain width
    float height = 5.0f;     // curtain height
    float zBase = -4.5f;    // slightly in front of wall

    float dx = width / segmentsX;
    float dy = height / segmentsY;

    for (int y = 0; y <= segmentsY; y++) {
        for (int x = 0; x <= segmentsX; x++) {

            float px = -width * 0.5f + x * dx;
            float py = y * dy;

            // WAVE (pleats)
            float wave = sin(px * 6.0f) * 0.12f;  // amplitude of wave
            float pz = zBase + wave;

            glm::vec3 pos(px, py, pz);

            // Normal is roughly facing forward
            glm::vec3 normal(0, 0, 1);

            glm::vec2 uv(
                float(x) / segmentsX,
                float(y) / segmentsY
            );

            verts.push_back({ pos, normal, uv });
        }
    }

    // Create indices
    for (int y = 0; y < segmentsY; y++) {
        for (int x = 0; x < segmentsX; x++) {
            int i0 = y * (segmentsX + 1) + x;
            int i1 = i0 + 1;
            int i2 = i0 + (segmentsX + 1);
            int i3 = i2 + 1;

            idx.push_back(i0); idx.push_back(i2); idx.push_back(i1);
            idx.push_back(i1); idx.push_back(i2); idx.push_back(i3);
        }
    }
}
