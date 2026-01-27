#version 450

in vec3 te_position;
in vec3 te_normal;
in vec2 te_uv;

// Terrain material textures
uniform sampler2D u_grass_albedo;
uniform sampler2D u_forest_albedo;
uniform sampler2D u_rocks_albedo;
uniform sampler2D u_snow_albedo;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal_metal;

void main() {

    vec3 out_color;
    vec2 uv = te_position.xz * 0.8;
    float h = te_position.y;
    // vec3 normal = normalize(out_normal);
    if (h > 28.0) {
        out_color = texture(u_snow_albedo, uv).xyz;
        // normal = texture(snow_nm_text, uv).xyz;
    }
    else if (h > 24.0) {
        float t = (h - 24.0) / 4.0;
        out_color = mix(texture(u_rocks_albedo, uv).xyz, texture(u_snow_albedo, uv).xyz, t);
        // normal = mix(texture(rock_nm_text, uv).xyz, texture(snow_nm_text, uv).xyz, t);
    }
    else if (h > 20.0) {
        float t = (h - 20.0) / 4.0;
        out_color = mix(texture(u_forest_albedo, uv).xyz, texture(u_rocks_albedo, uv).xyz, t);
        // normal = mix(texture(forest_nm_text, uv).xyz, texture(rock_nm_text, uv).xyz, t);
    }
    else if (h > 16.0) {
        float t = (h - 16.0) / 4.0;
        out_color = mix(texture(u_grass_albedo, uv).xyz, texture(u_forest_albedo, uv).xyz, t);
        // normal = mix(texture(grass_nm_text, uv).xyz, texture(forest_nm_text, uv).xyz, t);
    }
    else {
        out_color = texture(u_grass_albedo, uv).xyz;
    }
    
    out_albedo = vec4(out_color, 1.0); // Roughness in alpha (1.0 = rough)
    out_normal_metal = vec4(normalize(te_normal) * 0.5 + 0.5, 0.0); // Metal in alpha
}
