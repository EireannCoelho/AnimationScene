#pragma once
#include <vector>
#include "vertex.h"

// Grandfather clock: base (wider square), body (tall skinny rect), top (triangle)
void GenerateGrandfatherClockMesh(std::vector<Vertex>& verts,
                                  std::vector<unsigned int>& idx);
