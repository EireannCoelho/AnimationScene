#pragma once
#include <vector>
#include "vertex.h"

void GenerateWavyCurtainMesh(std::vector<Vertex>& verts,
                             std::vector<unsigned int>& idx,
                             int segmentsX,
                             int segmentsY);
