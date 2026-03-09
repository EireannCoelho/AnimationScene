#pragma once
#include <vector>
#include "vertex.h"

// 3D forest tree (trunk + cone foliage) for snow room - not Christmas tree style
void GenerateSnowRoomTreeTrunk(std::vector<Vertex>& verts,
                               std::vector<unsigned int>& idx,
                               float trunkRadius, float trunkHeight);

void GenerateSnowRoomTreeFoliage(std::vector<Vertex>& verts,
                                std::vector<unsigned int>& idx,
                                float baseRadius, float height, float yBase);

// 2-level foliage: larger base cone + smaller top cone (like wall tree style)
void GenerateSnowRoomTreeFoliage2Level(std::vector<Vertex>& verts,
                                      std::vector<unsigned int>& idx,
                                      float baseR1, float h1, float baseR2, float h2, float yBase);
