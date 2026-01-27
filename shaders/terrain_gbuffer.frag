#version 450

in vec3 te_position;
in vec3 te_normal;
in vec2 te_uv;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal_metal;

void main() {
    // Terrain coloring based on height
    vec3 color = vec3(0.2, 0.6, 0.15); // Grass green
    
    out_albedo = vec4(color, 1.0); // Roughness in alpha (1.0 = rough)
    out_normal_metal = vec4(normalize(te_normal) * 0.5 + 0.5, 0.0); // Metal in alpha
}
