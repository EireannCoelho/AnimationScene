#pragma once
#include <vector>
#include "vertex.h"

void GenerateWindowMesh(std::vector<Vertex>& verts,
                        std::vector<unsigned int>& idx,
                        float width, float height);
