#ifndef STATICMESH_H
#define STATICMESH_H

#include <TypedBuffer.h>
#include <Vertex.h>
#include <graphics.h>

#include <vector>

namespace OM3D
{

    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
    };

    struct BoundingSphere
    {
        glm::vec3 center;
        float radius;
    };

    class StaticMesh : NonCopyable
    {

    public:
        StaticMesh() = default;
        StaticMesh(StaticMesh&&) = default;
        StaticMesh& operator=(StaticMesh&&) = default;

        StaticMesh(const MeshData& data);

        void draw() const;

        const BoundingSphere& bounding_sphere() const;

    private:
        TypedBuffer<Vertex> _vertex_buffer;
        TypedBuffer<u32> _index_buffer;
        BoundingSphere _bounding_sphere;
    };

} // namespace OM3D

#endif // STATICMESH_H
