#ifndef SCENE_H
#define SCENE_H

#include <SceneObject.h>
#include <PointLight.h>
#include <Camera.h>
#include <shader_structs.h>
#include <TypedBuffer.h>

#include <vector>
#include <memory>

namespace OM3D {

class Scene : NonMovable {

    public:
        Scene();

        static Result<std::unique_ptr<Scene>> from_gltf(const std::string& file_name);

        void bind_buffer() const;
        void bind_buffer_pl() const;
        void render() const;

        void add_object(SceneObject obj);
        void add_light(PointLight obj);

        Span<const SceneObject> objects() const;
        Span<const PointLight> point_lights() const;

        Camera& camera();
        const Camera& camera() const;

        void set_envmap(std::shared_ptr<Texture> env);
        void set_ibl_intensity(float intensity);

        void set_light_view_proj(const glm::mat4& m);

        void set_sun(float altitude, float azimuth, glm::vec3 color = glm::vec3(1.0f));

    private:
        std::vector<SceneObject> _objects;
        std::vector<PointLight> _point_lights;

        mutable std::unique_ptr<TypedBuffer<shader::FrameData>> _frame_data_buffer;
        mutable std::unique_ptr<TypedBuffer<shader::PointLight>> _point_light_buffer;

        glm::vec3 _sun_direction = glm::vec3(0.2f, 1.0f, 0.1f);
        glm::vec3 _sun_color = glm::vec3(1.0f);

        std::shared_ptr<Texture> _envmap;
        float _ibl_intensity = 1.0f;
        Material _sky_material;

        Camera _camera;
        glm::mat4 _light_view_proj = glm::mat4(1.0f);
};

}

#endif // SCENE_H
