#version 450

#include "utils.glsl"
#include "structs.glsl"
#include "lighting.glsl"

layout(binding = 0) uniform sampler2D in_albedo_roughness;
layout(binding = 1) uniform sampler2D in_normal_metal;
layout(binding = 2) uniform sampler2D in_depth;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(std430, binding = 1) readonly buffer Lights {
    PointLight point_lights[];
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    vec3 base_color = texelFetch(in_albedo_roughness, coord, 0).rgb;
    float roughness = texelFetch(in_albedo_roughness, coord, 0).a;
    float depth = texelFetch(in_depth, coord, 0).r;
    vec3 normal = texelFetch(in_normal_metal, coord, 0).rgb;
    float metallic = texelFetch(in_normal_metal, coord, 0).a;

    normal = normal * 2.0 - 1.0;
    
    vec3 position = unproject(in_uv, depth, frame.camera.inv_view_proj);
    
    const vec3 to_view = (frame.camera.position - position);
    const vec3 view_dir = normalize(to_view);
    
    vec3 hdr = vec3(0.0);
    
    for (uint i = 0; i < point_lights.length(); ++i) {
        const PointLight light = point_lights[i];
        
        const vec3 to_light = light.position - position;
        const float dist = length(to_light);
        const vec3 light_dir = to_light / dist;
        
        const float attenuation = 1.0 / (dist * dist);
        
        if (dist <= light.radius) {
            hdr += light.color * attenuation * eval_brdf(normal, view_dir, light_dir, base_color, metallic, roughness);
        }
    }
    
    out_color = vec4(hdr, 1.0);
}
