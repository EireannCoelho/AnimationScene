# Nutcracker Room — Final Project
Author: Eireann Coelho
Course: UO CS441
Date: Fall 2024


Overview

This project is an interactive 3D “Nutcracker Room” scene built using Modern OpenGL (3.3 Core). 
The scene features:

- A full 3D room with warm brown walls
- Procedural wavy curtains with custom shading
- A Christmas tree that grows when pressing the G key
- Ornaments,light bulbs, and a gold star
- Randomly placed presents with different wrapping styles
- Camera movement and mouse look controls

The project demonstrates:
- Custom geometry (tree, curtains, windows, presents)
- Vertex/fragment shaders
- Lighting (ambient, diffuse, specular)
- Emissive flickering lights
- Procedural shading (tree needles, curtain folds)
- Transformations and scene composition


Controls


W / A / S / D  — Move the camera  
Mouse Look     — Look around  
ESC            — Unlock mouse  
Left Click     — Relock mouse  
G              — Grow the tree  


Project Structure


finalproject/
│
├── main.cpp
├── Makefile
├── README.md
│
├── geometry/
│   ├── room_geometry.cpp / .h
│   ├── tree_geometry.cpp / .h
│   ├── curtain_geometry.cpp / .h
│   ├── mesh_helpers.h
│   └── vertex.h
│
└── shaders/
    ├── room.vert
    └── room.frag


Building the Project


I spent most of my time working on the tree elements of the project. I struggled with
aligning the tree to the floor while also getting it to grow. at first I thought I had
to have the tree sconcealed underneath the floor to then grow out. I was able to 
find a way to grow the tree vertically up and horizontally out so that the tree
was not conceled under the floor but on top of it. I wishj i could have put more detail into 
the decorations and the room to amke the whole scene come to life. 


Geometry Implemented


ROOM  
- Floor, ceiling, walls  
- Optional window areas  
- Correct normals for lighting

TREE  
- Generated from stacked cone layers  
- Procedural shading simulates needles  
- Star on top, ornaments placed randomly

CURTAINS  
- High-resolution wavy mesh  
- Custom shadow darkening based on lighting  
- Chocolate brown color controlled via overrideColor

PRESENTS  
Each present has:  
- Random position around the tree  
- Random color  
- Random scale  
- A wrapping type:
  - 0 = solid color
  - 1 = candy cane stripes
  - 2 = bow cross wrap

LIGHTS  
- Emissive, pulsing tree bulbs  
- Star with gold override color


Shader Features


Fragment Shader:
- Ambient + diffuse + specular lighting
- Emissive flickering lights
- Curtain shadow and ambient occlusion effect
- Tree procedural texture
- Present wrapping logic

Vertex Shader:
- Standard MVP transform
- Pass-through normals and UVs


Known Limitations


- No shadow mapping (curtains do not cast shadows onto the room).  
- Lighting is simplified and not PBR.  
- Graphical details on tree is limited 
- Decorations of room are simple 


