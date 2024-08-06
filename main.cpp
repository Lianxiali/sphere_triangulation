#include <vector>
#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <unordered_map>

struct vertex 
{
    float x, y, z;

    bool operator==(const vertex& other) const {
        return std::fabs(x - other.x) < 1e-6 &&
               std::fabs(y - other.y) < 1e-6 &&
               std::fabs(z - other.z) < 1e-6;
    }
};

namespace std 
{
    template <>
    struct hash<vertex> {
        size_t operator()(const vertex& v) const {
            return ((std::hash<float>()(v.x)
                     ^ (std::hash<float>()(v.y) << 1)) >> 1)
                     ^ (std::hash<float>()(v.z) << 1);
        }
    };
}

struct triangle {
    std::array<int, 3> vertices;
};

vertex interpolate(vertex a, vertex b, vertex c, float row, float column) {
    vertex result;
    result.x = a.x + row * (b.x - a.x) + column * (c.x - b.x);
    result.y = a.y + row * (b.y - a.y) + column * (c.y - b.y);
    result.z = a.z + row * (b.z - a.z) + column * (c.z - b.z);
    return result;
}

vertex normalize(vertex a, float radius) {
    float length = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    a.x /= length;
    a.y /= length;
    a.z /= length;
    a.x *= radius;
    a.y *= radius;
    a.z *= radius;
    return a;
}

std::vector<triangle> get_sub_triangles(
    triangle face, 
    int resolution, 
    std::unordered_map<vertex, int>& vertex_map, 
    std::vector<vertex>& vertices) 
{
    vertex a = vertices[face.vertices[0]];
    vertex b = vertices[face.vertices[1]];
    vertex c = vertices[face.vertices[2]];
    std::vector<triangle> triangles;

    auto get_or_add_vertex = [&](vertex v) -> int {
        auto it = vertex_map.find(v);
        if (it != vertex_map.end()) {
            return it->second;
        }
        int index = vertices.size();
        vertices.push_back(v);
        vertex_map[v] = index;
        return index;
    };

std::cout << face.vertices[0] << " " << face.vertices[1]<< " " << face.vertices[2] << std::endl;
    for (int row = 0; row < resolution; ++row) {
        for (int column = 0; column <= row; ++column) {
            float invResolution = 1.0f / resolution;


            vertex v0 = interpolate(a, b, c, row * invResolution, column * invResolution);
            vertex v1 = interpolate(a, b, c, (row + 1) * invResolution, column * invResolution);
            vertex v2 = interpolate(a, b, c, (row + 1) * invResolution, (column + 1) * invResolution);
            vertex v3 = interpolate(a, b, c, row * invResolution, (column + 1) * invResolution);

std::cout << " row = " << row << " col =" << column << " ";

            int idx0 = get_or_add_vertex(v0);
            int idx1 = get_or_add_vertex(v1);
            int idx2 = get_or_add_vertex(v2);
            int idx3 = -1;

            if (column != row) 
            {
                idx3 = get_or_add_vertex(v3);                
                triangles.push_back({{idx0, idx1, idx2}});
                triangles.push_back({{idx0, idx2, idx3}});
            } 
            else 
            {
                triangles.push_back({{idx0, idx1, idx2}});
            }
std::cout << "  " << idx0 <<" " << idx1 <<" " << idx2 <<" " << idx3 <<std::endl;

        }
    }

    return triangles;
}

std::vector<triangle> get_sphere(vertex origin, float radius, int resolution, std::vector<vertex>& vertices) {
    // Note: The vertex index order matters
    // It produces duplicates (when the resolution is odd)
    // even if the vertex comparsion and hash map 
    // aim to remove the duplicates
    std::vector<triangle> octahedron = {
        {{0, 1, 2}}, {{0, 3, 1}}, {{0, 4, 3}}, {{0, 2, 4}}, 
        {{5, 1, 2}}, {{5, 3, 1}}, {{5, 4, 3}}, {{5, 2, 4}}
    };

    std::vector<vertex> initial_vertices = {
        {0, 1, 0}, {1, 0, 0},  {0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {0, -1, 0} 
    };

    std::unordered_map<vertex, int> vertex_map;
    for (const auto& v : initial_vertices) {
        vertex normalized_vertex = normalize(v, radius);
        vertex_map[normalized_vertex] = vertices.size();
        vertices.push_back(normalized_vertex);
    }

    std::vector<triangle> sub_triangles;
    for (const auto& t : octahedron) {
        std::vector<triangle> face_triangles = get_sub_triangles(t, resolution, vertex_map, vertices);
        sub_triangles.insert(sub_triangles.end(), face_triangles.begin(), face_triangles.end());
    }

    // Normalize all vertices after creating them
    for (auto& vert : vertices) {
        vert = normalize(vert, radius);
        vert.x += origin.x;
        vert.y += origin.y;
        vert.z += origin.z;
    }

    return sub_triangles;
}

void write_to_vtk(const std::vector<vertex>& vertices, const std::vector<triangle>& triangles, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open file " << filename << std::endl;
        return;
    }

    file << "# vtk DataFile Version 2.0\n";
    file << "Sphere Mesh\n";
    file << "ASCII\n";
    file << "DATASET UNSTRUCTURED_GRID\n";

    // Write points
    file << "POINTS " << vertices.size() << " float\n";
    for (const auto& vert : vertices) {
        file << vert.x << " " << vert.y << " " << vert.z << "\n";
    }

    // Write cells
    file << "CELLS " << triangles.size() << " " << triangles.size() * 4 << "\n";
    for (const auto& tri : triangles) {
        file << "3 " << tri.vertices[0] << " " << tri.vertices[1] << " " << tri.vertices[2] << "\n";
    }

    // Write cell types
    file << "CELL_TYPES " << triangles.size() << "\n";
    for (int i = 0; i < triangles.size(); ++i) {
        file << "5\n";  // VTK_TRIANGLE
    }

    file.close();
}

int main() {
    std::vector<vertex> vertices;
    std::vector<triangle> triangles = get_sphere({0, 0, 0}, 0.5f, 3, vertices);
    std::cout << "Generated " << triangles.size() << " triangles for the sphere." << std::endl;

    write_to_vtk(vertices, triangles, "sphere.vtk");
    std::cout << "Mesh written to sphere.vtk" << std::endl;

    return 0;
}
