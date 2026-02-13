#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    fragPos = vec3(model * vec4(inPos, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * inNormal;
    fragUV = inUV;

    gl_Position = projection * view * vec4(fragPos, 1.0);
}
