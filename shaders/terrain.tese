#version 450

layout(quads, fractional_odd_spacing, cw) in;

in vec2 tc_uv[];

out vec3 te_position;
out vec3 te_normal;
out vec2 te_uv;

layout(binding = 0) uniform sampler2D u_heightmap;
uniform mat4 u_view_proj;
uniform mat4 u_model;

uniform vec2 u_camera_pos_xz;
uniform float u_terrain_size; // World size
uniform float u_height_scale;

void main() {
    // Interpolate UV coordinates
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 uv = mix(mix(tc_uv[0], tc_uv[1], u), mix(tc_uv[3], tc_uv[2], u), v);
    te_uv = uv;

    // Interpolate Local Position (flat grid)
    vec3 p0 = mix(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, u);
    vec3 p1 = mix(gl_in[3].gl_Position.xyz, gl_in[2].gl_Position.xyz, u);
    vec3 local_pos = mix(p0, p1, v);

    // Initial World Position (before displacement)
    vec3 world_pos = (u_model * vec4(local_pos, 1.0)).xyz;

    // --- Heightmap Lookup ---
    
    // We need to map world_pos to texture UVs.
    // The heightmap is centered around the camera.
    // Texture UV [0,1] corresponds to [cam - size/2, cam + size/2]
    
    vec2 map_uv = (world_pos.xz - u_camera_pos_xz) / u_terrain_size + 0.5;

    // Sample height
    float h = texture(u_heightmap, map_uv).r;

    // Apply displacement
    world_pos.y += h;
    te_position = world_pos;

    // --- Normal Calculation ---
    // Finite difference for normal
    float tex_step = 1.0 / textureSize(u_heightmap, 0).x;
    float h_r = texture(u_heightmap, map_uv + vec2(tex_step, 0.0)).r;
    float h_u = texture(u_heightmap, map_uv + vec2(0.0, tex_step)).r;
    
    // Gradient vectors
    vec3 tan_x = normalize(vec3(u_terrain_size * tex_step, h_r - h, 0.0));
    vec3 tan_z = normalize(vec3(0.0, h_u - h, u_terrain_size * tex_step));
    
    te_normal = normalize(cross(tan_z, tan_x));

    gl_Position = u_view_proj * vec4(world_pos, 1.0);
}
