#pragma once
#include <vector>
#include "vertex.h"

// Grandfather clock: foot (wide), body (skinny), square (wider), triangle (same width as square)
// All parts are proper 3D boxes (cubes)
void GenerateGrandfatherClockBody(std::vector<Vertex>& verts,
                                  std::vector<unsigned int>& idx);

// White clock face circle (drawn on front of square)
void GenerateClockFace(std::vector<Vertex>& verts,
                      std::vector<unsigned int>& idx);

// Clock hands (two lines from center)
void GenerateClockHands(std::vector<Vertex>& verts,
                       std::vector<unsigned int>& idx);
