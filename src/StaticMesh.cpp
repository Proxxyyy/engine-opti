#include "StaticMesh.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

namespace OM3D {

extern bool audit_bindings_before_draw;

const BoundingSphere& StaticMesh::bounding_sphere() const {
    return _bounding_sphere;
}

StaticMesh::StaticMesh(const MeshData& data) :
    _vertex_buffer(data.vertices),
    _index_buffer(data.indices) {
    // Ritter's algorithm

    // Random point
    OM3D::Vertex p0 = data.vertices[0];

    // Farthest point from p0
    OM3D::Vertex p1 = p0;
    float dist2_p01 = 0;
    for(const auto& p : data.vertices) {
        float curDist = glm::dot(p.position - p0.position, p.position - p0.position);
        if(curDist > dist2_p01) {
            p1 = p;
            dist2_p01 = curDist;
        }
    }

    // Farthest point from p1
    OM3D::Vertex p2 = p1;
    float dist2_p12 = 0;
    for(const auto& p : data.vertices) {
        float curDist = glm::dot(p.position - p1.position, p.position - p1.position);
        if(curDist > dist2_p12) {
            p2 = p;
            dist2_p12 = curDist;
        }
    }

    // Initial sphere
    glm::vec3 center;
    center = (p1.position + p2.position) * 0.5f;
    float radius = glm::length(p1.position - p2.position) * 0.5f;

    // Adjust sphere
    for(const auto& p : data.vertices) {
        const float dist = glm::length(p.position - center);
        if(dist > radius) {
            const float new_radius = (radius + dist) * 0.5f;
            const float k = (new_radius - radius) / dist;
            radius = new_radius;
            center += (p.position - center) * k;
        }
    }

    _bounding_sphere.center = center;
    _bounding_sphere.radius = radius;
}

void StaticMesh::draw() const {
    _vertex_buffer.bind(BufferUsage::Attribute);
    _index_buffer.bind(BufferUsage::Index);

    // Vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
    // Vertex normal
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    // Vertex uv
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
    // Tangent / bitangent sign
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(8 * sizeof(float)));
    // Vertex color
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(12 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    if(audit_bindings_before_draw) {
        audit_bindings();
    }

    glDrawElements(GL_TRIANGLES, int(_index_buffer.element_count()), GL_UNSIGNED_INT, nullptr);
}

}
