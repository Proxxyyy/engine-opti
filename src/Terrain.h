#ifndef TERRAIN_H
#define TERRAIN_H

#include <Program.h>
#include <Texture.h>
#include <TypedBuffer.h>
#include <Vertex.h>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <memory>

namespace OM3D
{

    class Terrain
    {
    public:
        Terrain();

        // Called once at startup
        void init(std::shared_ptr<Program> compute_program, std::shared_ptr<Texture> heightmap);

        // Regenerates heightmap (call once or when needed)
        void update();

        // Single render method - expects program to already be bound
        // Sets terrain-specific uniforms and draws
        void render(Program& program) const;

        // Getters for terrain resources (needed for external program setup)
        const Texture& heightmap() const { return *_heightmap; }
        float size() const { return _size; }
        float height_scale() const { return _height_scale; }

    private:
        void generate_grid_mesh(u32 grid_size);
        void set_common_uniforms(Program& program) const;

        std::shared_ptr<Texture> _heightmap;
        std::shared_ptr<Program> _compute_program;

        // Raw OpenGL handles
        GLuint _vao = 0;
        GLuint _vbo = 0;
        GLuint _ibo = 0;
        u32 _index_count = 0;

        // Terrain settings
        float _size = 5.0f;
        float _height_scale = 5.0f;
    };

} // namespace OM3D

#endif // TERRAIN_H