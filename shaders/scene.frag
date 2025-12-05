#version 450

#include "utils.glsl"
#include "lighting.glsl"

layout(binding = 0) uniform sampler2D in_albedo_roughness;
layout(binding = 1) uniform sampler2D in_normal_metal;
layout(binding = 4) uniform samplerCube in_envmap;
layout(binding = 5) uniform sampler2D brdf_lut;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    vec3 base_color = texelFetch(in_albedo_roughness, coord, 0).rgb;
    vec3 hdr = vec3(0.0); // TODO from emissive texture in G-buffer
    float roughness = texelFetch(in_albedo_roughness, coord, 0).a;

    vec3 normal = texelFetch(in_normal_metal, coord, 0).rgb;
    float metallic = texelFetch(in_normal_metal, coord, 0).a;

    const vec3 to_view = (frame.camera.position - in_position);
    const vec3 view_dir = normalize(to_view);

    hdr += eval_ibl(in_envmap, brdf_lut, normal, view_dir, base_color, metallic, roughness) * frame.ibl_intensity;
    hdr += frame.sun_color * eval_brdf(normal, view_dir, frame.sun_dir, base_color, metallic, roughness);

    out_color = vec4(hdr, 1.0);
}
