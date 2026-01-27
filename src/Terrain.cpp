#include "Terrain.h"
#include <glad/gl.h>

namespace OM3D
{

    Terrain::Terrain() {}

    void Terrain::init(std::shared_ptr<Program> compute_program, std::shared_ptr<Texture> heightmap)
    {
        _compute_program = std::move(compute_program);
        _heightmap = std::move(heightmap);

        generate_grid_mesh(64);
        update();
    }

    void Terrain::update()
    {
        if (!_compute_program || !_heightmap)
        {
            return;
        }

        // Dispatch Compute - generate heightmap centered at origin
        _compute_program->bind();
        _compute_program->set_uniform(HASH("u_camera_pos"), glm::vec2(0.0f, 0.0f));
        _compute_program->set_uniform(HASH("u_scale"), _size);
        _compute_program->set_uniform(HASH("u_noise_freq_scale"), _noise_frequency);
        _compute_program->set_uniform(HASH("u_height_scale"), _height_scale);

        // Bind image
        glBindImageTexture(0, _heightmap->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

        glm::uvec2 size = _heightmap->size();
        glDispatchCompute((size.x + 15) / 16, (size.y + 15) / 16, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void Terrain::set_common_uniforms(Program& program, const Camera& camera) const
    {
        // Keep terrain fixed at world origin instead of following camera
        program.set_uniform(HASH("u_camera_pos_xz"), glm::vec2(0.0f, 0.0f));
        program.set_uniform(HASH("u_view_proj"), camera.view_proj_matrix());
        program.set_uniform(HASH("u_model_view"), camera.view_matrix() * glm::mat4(1.0f));
        program.set_uniform(HASH("u_terrain_size"), _size);
        program.set_uniform(HASH("u_height_scale"), _height_scale);
        program.set_uniform(HASH("u_max_dist"), _size * 2.0f);
        program.set_uniform(HASH("u_tess_level_factor"), 1.0f);
        program.set_uniform(HASH("u_model"), glm::mat4(1.0f));
    }

    void Terrain::render(Program& program, const Camera& camera) const
    {
        if (!_heightmap || _vao == 0 || _index_count == 0)
        {
            return;
        }

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL); // Reverse-Z
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Bind heightmap texture
        _heightmap->bind(0);

        // Set terrain-specific uniforms
        set_common_uniforms(program, camera);

        // Save previous VAO state
        GLint prev_vao = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);

        // Draw terrain patches
        glBindVertexArray(_vao);
        glPatchParameteri(GL_PATCH_VERTICES, 4);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_PATCHES, _index_count, GL_UNSIGNED_INT, nullptr);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // restore to normal

        // Restore previous VAO
        glBindVertexArray(prev_vao);
    }

    void Terrain::generate_grid_mesh(u32 grid_size)
    {
        if (_vao)
            glDeleteVertexArrays(1, &_vao);
        if (_vbo)
            glDeleteBuffers(1, &_vbo);
        if (_ibo)
            glDeleteBuffers(1, &_ibo);

        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        float step = 1.0f / float(grid_size);

        for (u32 z = 0; z <= grid_size; ++z)
        {
            for (u32 x = 0; x <= grid_size; ++x)
            {
                Vertex v;
                v.uv = glm::vec2(float(x) / grid_size, float(z) / grid_size);
                v.position = glm::vec3((float(x) * step - 0.5f) * _size, 0.0f, (float(z) * step - 0.5f) * _size);
                v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vertices.push_back(v);
            }
        }

        for (u32 z = 0; z < grid_size; ++z)
        {
            for (u32 x = 0; x < grid_size; ++x)
            {
                u32 start = z * (grid_size + 1) + x;
                indices.push_back(start);
                indices.push_back(start + 1);
                indices.push_back(start + (grid_size + 1) + 1);
                indices.push_back(start + (grid_size + 1));
            }
        }

        _index_count = u32(indices.size());

        glCreateBuffers(1, &_vbo);
        glNamedBufferData(_vbo, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glCreateBuffers(1, &_ibo);
        glNamedBufferData(_ibo, indices.size() * sizeof(u32), indices.data(), GL_STATIC_DRAW);

        glCreateVertexArrays(1, &_vao);

        glVertexArrayVertexBuffer(_vao, 0, _vbo, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(_vao, _ibo);

        glEnableVertexArrayAttrib(_vao, 0);
        glVertexArrayAttribFormat(_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
        glVertexArrayAttribBinding(_vao, 0, 0);

        glEnableVertexArrayAttrib(_vao, 1);
        glVertexArrayAttribFormat(_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
        glVertexArrayAttribBinding(_vao, 1, 0);

        glEnableVertexArrayAttrib(_vao, 2);
        glVertexArrayAttribFormat(_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
        glVertexArrayAttribBinding(_vao, 2, 0);
    }

} // namespace OM3D
