#version 450

layout(vertices = 4) out;

in vec3 v_position[];
in vec2 v_uv[];

out vec2 tc_uv[];

uniform mat4 u_view_proj;
uniform mat4 u_model;
uniform vec3 u_camera_pos;
uniform float u_tess_level_factor; // Global multiplier
uniform float u_max_dist;

// Calculate tessellation level based on distance
float get_tess_level(float distance, float distance_min, float distance_max) {
    float t = clamp((distance_max - distance) / (distance_max - distance_min), 0.0, 1.0);
    return mix(1.0, 64.0, t) * u_tess_level_factor;
}

bool is_patch_visible(vec3 min_pos, vec3 max_pos) {
    // Simple/Naïve frustum check could go here
    // For now, assuming always visible as CPU culling isn't fed here yet
    // GL clipping will handle fine-grained culling
    return true;
}

void main() {
    tc_uv[gl_InvocationID] = v_uv[gl_InvocationID];

    if (gl_InvocationID == 0) {
        // Calculate center of patch
        vec3 center_pos = (v_position[0] + v_position[1] + v_position[2] + v_position[3]) * 0.25;
        vec3 world_pos = (u_model * vec4(center_pos, 1.0)).xyz;

        float dist = distance(world_pos, u_camera_pos);
        
        // Simple distance-based LOD
        float level = get_tess_level(dist, 10.0, u_max_dist);

        // Frustum cull optimization:
        // vec4 clip_pos = u_view_proj * vec4(world_pos, 1.0);
        // if (clip_pos.z < -clip_pos.w) level = 0.0; // Behind camera

        gl_TessLevelOuter[0] = level;
        gl_TessLevelOuter[1] = level;
        gl_TessLevelOuter[2] = level;
        gl_TessLevelOuter[3] = level;

        gl_TessLevelInner[0] = level;
        gl_TessLevelInner[1] = level;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
