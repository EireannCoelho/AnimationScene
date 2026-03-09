#pragma once
#include <vector>
#include "vertex.h"

// Simple 2D tree silhouette (trunk + triangle foliage) for forest wall decoration
void GenerateForestTreeMesh(std::vector<Vertex>& verts,
                            std::vector<unsigned int>& idx,
                            float width, float height);
