#include <vector>
#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <unordered_map>

struct Vertex {
    float x, y, z;

    bool operator==(const Vertex& other) const {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }
};

namespace std {
    template <>
    struct hash<Vertex> {
        size_t operator()(const Vertex& v) const {
            size_t hx = std::hash<float>()(v.x);
            size_t hy = std::hash<float>()(v.y);
            size_t hz = std::hash<float>()(v.z);
            return hx ^ (hy << 1) ^ (hz << 2);
        }
    };
}

struct Triangle {
    std::array<int, 3> vertices;
};

class Sphere {
public:
    Sphere(Vertex origin, float radius, int resolution)
        : origin(origin), radius(radius), resolution(resolution) {
        triangulate();
    }

    void write_to_vtk(const std::string& filename) const {
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

    int get_total_vertices() const {
        return vertices.size();
    }

    int get_total_cells() const {
        return triangles.size();
    }
    Vertex get_origin() const {
        return origin;
    }

    float get_radius() const {
        return radius;
    }

    int get_resolution() const {
        return resolution;
    }

    const std::vector<Vertex>& get_vertices() const {
        return vertices;
    }

    const std::vector<Triangle>& get_triangles() const {
        return triangles;
    }

    std::vector<float> get_triangle_areas() const {
        std::vector<float> areas;
        areas.reserve(triangles.size());

        for (const auto& tri : triangles) {
            const Vertex& v0 = vertices[tri.vertices[0]];
            const Vertex& v1 = vertices[tri.vertices[1]];
            const Vertex& v2 = vertices[tri.vertices[2]];

            // Compute area using cross product
            Vertex edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
            Vertex edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

            Vertex cross_product = {
                edge1.y * edge2.z - edge1.z * edge2.y,
                edge1.z * edge2.x - edge1.x * edge2.z,
                edge1.x * edge2.y - edge1.y * edge2.x
            };

            float area = 0.5f * std::sqrt(cross_product.x * cross_product.x +
                                          cross_product.y * cross_product.y +
                                          cross_product.z * cross_product.z);

            areas.push_back(area);
        }

        return areas;
    }

    std::vector<Vertex> get_triangle_centroids() const {
        std::vector<Vertex> centroids;
        centroids.reserve(triangles.size());

        for (const auto& tri : triangles) {
            const Vertex& v0 = vertices[tri.vertices[0]];
            const Vertex& v1 = vertices[tri.vertices[1]];
            const Vertex& v2 = vertices[tri.vertices[2]];

            Vertex centroid = {
                (v0.x + v1.x + v2.x) / 3.0f,
                (v0.y + v1.y + v2.y) / 3.0f,
                (v0.z + v1.z + v2.z) / 3.0f
            };

            centroids.push_back(centroid);
        }

        return centroids;
    }

private:
    Vertex origin;
    float radius;
    int resolution;
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;

    void triangulate() {
        std::vector<Triangle> octahedron = {
        {{0, 1, 2}}, {{0, 3, 1}}, {{0, 4, 3}}, {{0, 2, 4}}, 
        {{5, 1, 2}}, {{5, 3, 1}}, {{5, 4, 3}}, {{5, 2, 4}}
        };

        std::vector<Vertex> initial_vertices = {
            {0, 1, 0}, {1, 0, 0},  {0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {0, -1, 0} 
        };

        std::unordered_map<Vertex, int> vertex_map;
        for (const auto& v : initial_vertices) {
            Vertex normalized_vertex = normalize(v, radius);
            vertex_map[normalized_vertex] = vertices.size();
            vertices.push_back(normalized_vertex);
        }

        for (const auto& t : octahedron) {
            std::vector<Triangle> face_triangles = get_sub_triangles(t, resolution, vertex_map, vertices);
            triangles.insert(triangles.end(), face_triangles.begin(), face_triangles.end());
        }

        for (auto& vert : vertices) {
            vert = normalize(vert, radius);
            vert.x += origin.x;
            vert.y += origin.y;
            vert.z += origin.z;
        }
    }

    Vertex interpolate(Vertex a, Vertex b, Vertex c, float row, float column) const {
        Vertex result;
        result.x = a.x + row * (b.x - a.x) + column * (c.x - b.x);
        result.y = a.y + row * (b.y - a.y) + column * (c.y - b.y);
        result.z = a.z + row * (b.z - a.z) + column * (c.z - b.z);
        return result;
    }

    Vertex normalize(Vertex a, float radius) const {
        float length = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
        a.x /= length;
        a.y /= length;
        a.z /= length;
        a.x *= radius;
        a.y *= radius;
        a.z *= radius;
        return a;
    }

    std::vector<Triangle> get_sub_triangles(
        Triangle face, 
        int resolution, 
        std::unordered_map<Vertex, int>& vertex_map, 
        std::vector<Vertex>& vertices) 
    {
        Vertex a = vertices[face.vertices[0]];
        Vertex b = vertices[face.vertices[1]];
        Vertex c = vertices[face.vertices[2]];
        std::vector<Triangle> triangles;

        auto get_or_add_vertex = [&](Vertex v) -> int {
            auto it = vertex_map.find(v);
            if (it != vertex_map.end()) {
                return it->second;
            }
            int index = vertices.size();
            vertices.push_back(v);
            vertex_map[v] = index;
            return index;
        };

        for (int row = 0; row < resolution; ++row) {
            for (int column = 0; column <= row; ++column) {
                float invResolution = 1.0f / resolution;

                Vertex v0 = interpolate(a, b, c, row * invResolution, column * invResolution);
                Vertex v1 = interpolate(a, b, c, (row + 1) * invResolution, column * invResolution);
                Vertex v2 = interpolate(a, b, c, (row + 1) * invResolution, (column + 1) * invResolution);
                Vertex v3 = interpolate(a, b, c, row * invResolution, (column + 1) * invResolution);

                int idx0 = get_or_add_vertex(v0);
                int idx1 = get_or_add_vertex(v1);
                int idx2 = get_or_add_vertex(v2);

                if (column != row) {
                    int idx3 = get_or_add_vertex(v3);                
                    triangles.push_back({{idx0, idx1, idx2}});
                    triangles.push_back({{idx0, idx2, idx3}});
                } else {
                    triangles.push_back({{idx0, idx1, idx2}});
                }
            }
        }

        return triangles;
    }
};

int main() {
    Sphere sphere({0, 0, 0}, 0.5f, 3);
    std::cout << "Generated sphere mesh." << std::endl;
    std::cout << "Total vertices: " << sphere.get_total_vertices() << std::endl;
    std::cout << "Total cells: " << sphere.get_total_cells() << std::endl;
    std::cout << "Origin: (" << sphere.get_origin().x << ", " << sphere.get_origin().y << ", " << sphere.get_origin().z << ")" << std::endl;
    std::cout << "Radius: " << sphere.get_radius() << std::endl;
    std::cout << "Resolution: " << sphere.get_resolution() << std::endl;    
    std::vector<float> areas = sphere.get_triangle_areas();
    for (size_t i = 0; i < areas.size(); ++i) {
        std::cout << "Area of triangle " << i << ": " << areas[i] << std::endl;
    }

    std::vector<Vertex> centroids = sphere.get_triangle_centroids();
    for (size_t i = 0; i < centroids.size(); ++i) {
        const auto& c = centroids[i];
        std::cout << "Centroid of triangle " << i << ": (" << c.x << ", " << c.y << ", " << c.z << ")" << std::endl;
    }
        
    sphere.write_to_vtk("sphere.vtk");
    std::cout << "Mesh written to sphere.vtk" << std::endl;

    return 0;
}
