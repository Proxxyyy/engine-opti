#version 450

layout(location = 0) in vec3 in_pos;

uniform mat4 lightSpaceMatrix;
uniform mat4 transform;

void main() {
    gl_Position = lightSpaceMatrix * transform * vec4(in_pos, 1.0);
}
