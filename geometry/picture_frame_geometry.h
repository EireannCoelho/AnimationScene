#pragma once
#include <vector>
#include "vertex.h"

// Frame + canvas (painting surface). Canvas is inset from frame.
// Returns total mesh; frameIndexCount = indices for wooden frame, rest is canvas
void GeneratePictureFrameMesh(std::vector<Vertex>& verts,
                              std::vector<unsigned int>& idx,
                              size_t& frameIndexCount,
                              float frameWidth, float frameHeight,
                              float frameDepth);
