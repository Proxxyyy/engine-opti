#version 450

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D gbuffer_albedo_roughness;  // RGBA8_sRGB
layout(binding = 1) uniform sampler2D gbuffer_normal_metal;      // RGBA8_UNORM
layout(binding = 2) uniform sampler2D gbuffer_depth;             // Depth32F

in vec2 v_uv;

uniform uint debug_mode; // 0=depth, 1=normal, 2=albedo, 3=metallic, 4=roughness

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(gbuffer_albedo_roughness, 0));
    
    vec4 albedo_roughness = texture(gbuffer_albedo_roughness, uv);
    vec4 normal_metal = texture(gbuffer_normal_metal, uv);
    float depth = texture(gbuffer_depth, uv).r;
    
    vec3 albedo = albedo_roughness.rgb;
    float roughness = albedo_roughness.a;
    vec3 encoded_normal = normal_metal.rgb;
    float metalness = normal_metal.a;
    
    vec3 normal = encoded_normal * 2.0 - 1.0;
    
    switch(debug_mode) {
        case 0u:
            out_color = vec4(vec3(pow(depth, 0.35)), 1.0);
            break;
        case 1u:
            out_color = vec4(normal * 0.5 + 0.5, 1.0);
            break;
        case 2u:
            out_color = vec4(albedo, 1.0);
            break;
        case 3u:
            out_color = vec4(vec3(metalness), 1.0);
            break;
        case 4u:
            out_color = vec4(vec3(roughness), 1.0);
            break;
        default:
            out_color = vec4(albedo, 1.0);
            break;
    }
}
