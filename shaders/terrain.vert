#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

out vec3 v_position;
out vec2 v_uv;

void main() {
    v_position = in_position;
    v_uv = in_uv;
    
    // Pass position through for tessellation stages
    // TCS reads gl_in[].gl_Position
    gl_Position = vec4(in_position, 1.0);
}
