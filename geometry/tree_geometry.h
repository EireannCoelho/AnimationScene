#pragma once
#include <vector>
#include <glm/glm.hpp>

void GenerateTreeGeometry(std::vector<Vertex>& outVerts,
                          std::vector<unsigned int>& outIdx);

void GenerateBranchLayer(std::vector<Vertex>& verts,
                         std::vector<unsigned int>& indices,
                         float radius,
                         float height,
                         int segments,
                         float yOffset);

void GenerateCylinder(std::vector<Vertex>& verts,
                      std::vector<unsigned int>& indices,
                      float radius,
                      float height,
                      int segments,
                      float yOffset);