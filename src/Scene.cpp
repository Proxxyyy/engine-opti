#include "Scene.h"

#include <TypedBuffer.h>
#include <iostream>

namespace OM3D
{

    Scene::Scene()
    {
        _sky_material.set_program(Program::from_files("sky.frag", "screen.vert"));
        _sky_material.set_depth_test_mode(DepthTestMode::None);

        _envmap = std::make_shared<Texture>(Texture::empty_cubemap(4, ImageFormat::RGBA8_UNORM));
    }

    void Scene::add_object(SceneObject obj) { _objects.emplace_back(std::move(obj)); }

    void Scene::add_light(PointLight obj) { _point_lights.emplace_back(std::move(obj)); }

    Span<const SceneObject> Scene::objects() const { return _objects; }

    Span<const PointLight> Scene::point_lights() const { return _point_lights; }

    Camera& Scene::camera() { return _camera; }

    const Camera& Scene::camera() const { return _camera; }

    void Scene::set_envmap(std::shared_ptr<Texture> env) { _envmap = std::move(env); }

    void Scene::set_ibl_intensity(float intensity) { _ibl_intensity = intensity; }

    void Scene::set_sun(float altitude, float azimuth, glm::vec3 color)
    {
        // Convert from degrees to radians
        const float alt = glm::radians(altitude);
        const float azi = glm::radians(azimuth);
        // Convert from polar to cartesian
        _sun_direction = glm::vec3(sin(azi) * cos(alt), sin(alt), cos(azi) * cos(alt));
        _sun_color = color;
    }

    void Scene::set_light_view_proj(const glm::mat4& m) { _light_view_proj = m; }

    void Scene::bind_buffer() const
    {
        if (!_frame_data_buffer)
        {
            _frame_data_buffer = std::make_unique<TypedBuffer<shader::FrameData>>(nullptr, 1);
        }

        {
            auto mapping = _frame_data_buffer->map(AccessType::WriteOnly);
            mapping[0].camera.view_proj = _camera.view_proj_matrix();
            mapping[0].camera.inv_view_proj = glm::inverse(_camera.view_proj_matrix());
            mapping[0].camera.position = _camera.position();
            mapping[0].light_view_proj = _light_view_proj;
            mapping[0].point_light_count = u32(_point_lights.size());
            mapping[0].sun_color = _sun_color;
            mapping[0].sun_dir = glm::normalize(_sun_direction);
            mapping[0].ibl_intensity = _ibl_intensity;
        }

        _frame_data_buffer->bind(BufferUsage::Uniform, 0);

        // Bind envmap
        DEBUG_ASSERT(_envmap && !_envmap->is_null());
        _envmap->bind(4);

        // Bind brdf lut needed for lighting to scene rendering shaders
        brdf_lut().bind(5);
    }

    void Scene::bind_buffer_pl() const
    {
        if (!_point_light_buffer)
        {
            // std::max with 1 to prevent error if 0 point light in the scene
            _point_light_buffer = std::make_unique<TypedBuffer<shader::PointLight>>(
                    nullptr, std::max(_point_lights.size(), size_t(1)));
        }

        {
            auto mapping = _point_light_buffer->map(AccessType::WriteOnly);
            for (size_t i = 0; i != _point_lights.size(); ++i)
            {
                const auto& light = _point_lights[i];
                mapping[i] = {light.position(), light.radius(), light.color(), 0.0f};
            }
        }

        _point_light_buffer->bind(BufferUsage::Storage, 1);
    }

    void Scene::render() const
    {
        bind_buffer();

        bind_buffer_pl();

        // Render the sky
        _sky_material.bind();
        _sky_material.set_uniform(HASH("intensity"), _ibl_intensity);
        draw_full_screen_triangle();

        // Render every object
        {
            // Opaque first
            for (const SceneObject& obj: _objects)
            {
                if (obj.material().is_opaque())
                {
                    // Check frustum culling
                    Frustum frustum = _camera.build_frustum();

                    const BoundingSphere bs = obj.mesh()->bounding_sphere();
                    glm::vec3 bsWS = obj.transform() * glm::vec4(bs.center, 1.0f);

                    bool is_culled = false;

                    if (glm::dot(frustum._near_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }
                    else if (glm::dot(frustum._top_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }
                    else if (glm::dot(frustum._bottom_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }
                    else if (glm::dot(frustum._right_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }
                    else if (glm::dot(frustum._left_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }

                    if (is_culled)
                    {
                        continue;
                    }
                    obj.render();
                }
            }

            // Transparent after
            for (const SceneObject& obj: _objects)
            {

                if (!obj.material().is_opaque())
                {
                    // Check frustum culling
                    Frustum frustum = _camera.build_frustum();

                    const BoundingSphere bs = obj.mesh()->bounding_sphere();
                    glm::vec3 bsWS = obj.transform() * glm::vec4(bs.center, 1.0f);

                    bool is_culled = false;

                    if (glm::dot(frustum._near_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }
                    else if (glm::dot(frustum._top_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }
                    else if (glm::dot(frustum._bottom_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }
                    else if (glm::dot(frustum._right_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }
                    else if (glm::dot(frustum._left_normal, bsWS - _camera.position()) < -bs.radius)
                    {
                        is_culled = true;
                    }

                    if (is_culled)
                    {
                        continue;
                    }
                    obj.render();
                }
            }
        }
    }

} // namespace OM3D
