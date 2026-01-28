struct CameraData {
    mat4 view_proj;
    mat4 inv_view_proj;
    vec3 position;
    float padding;
};

struct FrameData {
    CameraData camera;

    mat4 light_view_proj;

    vec3 sun_dir;
    uint point_light_count;

    vec3 sun_color;
    float ibl_intensity;
};

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float padding;
};


struct Frustum {
    vec3 near_normal;
    float padding0; // to align to 16 bytes
    vec3 top_normal;
    float padding1;
    vec3 bottom_normal;
    float padding2;
    vec3 right_normal;
    float padding3;
    vec3 left_normal;
    float padding4;
};
