#pragma once

#include <glm/glm.hpp>

#include <vector>

class Mesh
{
public:
    Mesh() = default;

    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> triangles;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec2> uvs;

    static Mesh CreateCube(
        glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
    )
    {
        Mesh mesh;

        // --------------------------------------------------
        // Vertices
        // 4 vertices per face × 6 faces = 24 vertices
        // --------------------------------------------------

        mesh.vertices = {
            // Front (+Z)
            glm::vec3(-0.5f, -0.5f,  0.5f),
            glm::vec3(0.5f, -0.5f,  0.5f),
            glm::vec3(0.5f,  0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f,  0.5f),

            // Back (-Z)
            glm::vec3(0.5f, -0.5f, -0.5f),
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),
            glm::vec3(0.5f,  0.5f, -0.5f),

            // Right (+X)
            glm::vec3(0.5f, -0.5f,  0.5f),
            glm::vec3(0.5f, -0.5f, -0.5f),
            glm::vec3(0.5f,  0.5f, -0.5f),
            glm::vec3(0.5f,  0.5f,  0.5f),

            // Left (-X)
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3(-0.5f, -0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),

            // Top (+Y)
            glm::vec3(-0.5f,  0.5f,  0.5f),
            glm::vec3(0.5f,  0.5f,  0.5f),
            glm::vec3(0.5f,  0.5f, -0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),

            // Bottom (-Y)
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3(0.5f, -0.5f, -0.5f),
            glm::vec3(0.5f, -0.5f,  0.5f),
            glm::vec3(-0.5f, -0.5f,  0.5f)
        };

        // --------------------------------------------------
        // Triangles
        // --------------------------------------------------

        mesh.triangles = {
            // Front
            0,  1,  2,
            2,  3,  0,

            // Back
            4,  5,  6,
            6,  7,  4,

            // Right
            8,  9, 10,
            10, 11,  8,

            // Left
            12, 13, 14,
            14, 15, 12,

            // Top
            16, 17, 18,
            18, 19, 16,

            // Bottom
            20, 21, 22,
            22, 23, 20
        };

        // --------------------------------------------------
		// Colors
        // --------------------------------------------------

        mesh.colors = {
            color, color, color, color,
            color, color, color, color,
            color, color, color, color,
            color, color, color, color,
            color, color, color, color,
            color, color, color, color
        };

        // --------------------------------------------------
        // Normals
        // --------------------------------------------------

        mesh.normals = {
            // Front (+Z)
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),

            // Back (-Z)
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),

            // Right (+X)
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),

            // Left (-X)
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),

            // Top (+Y)
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),

            // Bottom (-Y)
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f)
        };

        // --------------------------------------------------
        // UVs
        // --------------------------------------------------

        mesh.uvs = {
            // Front
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),

            // Back
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),

            // Right
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),

            // Left
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),

            // Top
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),

            // Bottom
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f)
        };

        return mesh;
    }

    static Mesh CreateSphere(
        float radius = 0.5f,
        unsigned int latitudeBands = 20,
        unsigned int longitudeBands = 20,
        glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
    )
    {
        Mesh mesh;

        // --------------------------------------------------
        // Vertices, Normals, UVs, and Colors
        // --------------------------------------------------
        for (unsigned int lat = 0; lat <= latitudeBands; ++lat)
        {
            float theta = lat * glm::pi<float>() / latitudeBands;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            for (unsigned int lon = 0; lon <= longitudeBands; ++lon)
            {
                float phi = lon * 2.0f * glm::pi<float>() / longitudeBands;
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);

                // Outward-facing unit normal (same convention as the cube)
                glm::vec3 normal(
                    cosPhi * sinTheta,
                    cosTheta,
                    sinPhi * sinTheta
                );

                glm::vec2 uv(
                    (float)lon / (float)longitudeBands,
                    1.0f - (float)lat / (float)latitudeBands // v increases upward, like the cube
                );

                mesh.vertices.push_back(normal * radius);
                mesh.normals.push_back(normal);
                mesh.uvs.push_back(uv);
                mesh.colors.push_back(color);
            }
        }

        // --------------------------------------------------
        // Triangles (counter-clockwise when viewed from outside)
        // --------------------------------------------------
        for (unsigned int lat = 0; lat < latitudeBands; ++lat)
        {
            for (unsigned int lon = 0; lon < longitudeBands; ++lon)
            {
                unsigned int first = (lat * (longitudeBands + 1)) + lon;
                unsigned int second = first + longitudeBands + 1;

                // First triangle of quad
                mesh.triangles.push_back(first);
                mesh.triangles.push_back(first + 1);
                mesh.triangles.push_back(second);

                // Second triangle of quad
                mesh.triangles.push_back(second);
                mesh.triangles.push_back(first + 1);
                mesh.triangles.push_back(second + 1);
            }
        }

        return mesh;
    }
};