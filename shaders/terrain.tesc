#version 450

layout(vertices = 4) out;

in vec3 v_position[];
in vec2 v_uv[];

out vec2 tc_uv[];

uniform mat4 u_view_proj;
uniform mat4 u_model;
uniform mat4 u_model_view;
uniform vec3 u_camera_pos;
uniform float u_tess_level_factor; // Global multiplier
uniform float u_max_dist;

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tc_uv[gl_InvocationID] = v_uv[gl_InvocationID];

    if (gl_InvocationID == 0)
    {
        // Adaptive tessellation based on camera distance
        const int MIN_TESS_LEVEL = 4;
        const int MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE = 50.0;
        const float MAX_DISTANCE = 300.0;
        const float CULL_DISTANCE = 400.0;  // Distance beyond which patches are discarded

        // Transform vertices to eye space
        vec4 eyePos00 = u_model_view * gl_in[0].gl_Position;
        vec4 eyePos01 = u_model_view * gl_in[1].gl_Position;
        vec4 eyePos10 = u_model_view * gl_in[2].gl_Position;
        vec4 eyePos11 = u_model_view * gl_in[3].gl_Position;

        // Calculate true 3D distance from camera in eye space
        float dist00 = length(eyePos00.xyz);
        float dist01 = length(eyePos01.xyz);
        float dist10 = length(eyePos10.xyz);
        float dist11 = length(eyePos11.xyz);

        // Distance culling: if ALL vertices are beyond cull distance, discard the patch
        float minDist = min(min(dist00, dist01), min(dist10, dist11));
        if (minDist > CULL_DISTANCE) {
            // Discard patch entirely by setting tessellation to 0
            gl_TessLevelOuter[0] = 0.0;
            gl_TessLevelOuter[1] = 0.0;
            gl_TessLevelOuter[2] = 0.0;
            gl_TessLevelOuter[3] = 0.0;
            gl_TessLevelInner[0] = 0.0;
            gl_TessLevelInner[1] = 0.0;
            return;
        }

        // Calculate smooth tessellation falloff using smoothstep
        // smoothstep(edge0, edge1, x) smoothly interpolates from 0 to 1 as x goes from edge0 to edge1
        float smoothDist00 = smoothstep(MAX_DISTANCE, MIN_DISTANCE, dist00);
        float smoothDist01 = smoothstep(MAX_DISTANCE, MIN_DISTANCE, dist01);
        float smoothDist10 = smoothstep(MAX_DISTANCE, MIN_DISTANCE, dist10);
        float smoothDist11 = smoothstep(MAX_DISTANCE, MIN_DISTANCE, dist11);

        // Calculate tessellation levels per edge (each edge connects 2 vertices)
        // Outer[0]: edge 0-1 (bottom)
        // Outer[1]: edge 1-3 (right)
        // Outer[2]: edge 3-2 (top)
        // Outer[3]: edge 2-0 (left)
        float tessLevel0 = mix(MIN_TESS_LEVEL, MAX_TESS_LEVEL, min(smoothDist00, smoothDist01));
        float tessLevel1 = mix(MIN_TESS_LEVEL, MAX_TESS_LEVEL, min(smoothDist01, smoothDist11));
        float tessLevel2 = mix(MIN_TESS_LEVEL, MAX_TESS_LEVEL, min(smoothDist11, smoothDist10));
        float tessLevel3 = mix(MIN_TESS_LEVEL, MAX_TESS_LEVEL, min(smoothDist10, smoothDist00));

        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        // Inner levels: average of opposing edges for smooth transitions
        gl_TessLevelInner[0] = (tessLevel0 + tessLevel2) * 0.5;
        gl_TessLevelInner[1] = (tessLevel1 + tessLevel3) * 0.5;
    }
}
