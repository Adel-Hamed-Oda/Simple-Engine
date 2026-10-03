#pragma once

#include <glad/gl.h>

#include "../Component.h"
#include "Mesh.h"

#include <memory>
#include <string>

class Material;

class MeshRenderer : public Component
{
public:
    MeshRenderer(const Mesh& mesh, std::shared_ptr<Material> material)
        : mesh(mesh), material(std::move(material))
    {
        InitializeBuffers();
    }
    ~MeshRenderer()
    {
        CleanupBuffers();
    }

    const Mesh* GetMesh() const
    {
        return &mesh;
    }
    void SetMesh(const Mesh& mesh)
    {
        CleanupBuffers();
        this->mesh = mesh;
        InitializeBuffers();
    }

    Material* GetMaterial() const
    {
        return material.get();
    }
    // Shared ownership: the material stays alive as long as any renderer uses it,
    // and one material can be shared by many renderers.
    void SetMaterial(std::shared_ptr<Material> material)
    {
        this->material = std::move(material);
    }

    void Draw() const
    {
        if (VAO == 0 || indicesCount == 0)
            return;

        glBindVertexArray(VAO);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(indicesCount),
            GL_UNSIGNED_INT,
            nullptr
        );

        glBindVertexArray(0);
    }

    void DrawLighting() const
    {
        if (lightingVAO == 0 || indicesCount == 0)
            return;

        glBindVertexArray(lightingVAO);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(indicesCount),
            GL_UNSIGNED_INT,
            nullptr
        );

        glBindVertexArray(0);
    }

private:
    Mesh mesh;
    std::shared_ptr<Material> material;

    unsigned int VAO = 0;
    unsigned int lightingVAO = 0;

    unsigned int triangleEBO = 0;

    unsigned int posVBO = 0;
    unsigned int colorVBO = 0;
    unsigned int uvVBO = 0;
    unsigned int normalVBO = 0;

    size_t indicesCount = 0;

    void InitializeBuffers()
    {
        if (!CheckMeshValidity()) return;

        indicesCount = mesh.triangles.size();

        // ==================================================
        // Position VBO
        // ==================================================

        glGenBuffers(1, &posVBO);

        glBindBuffer(GL_ARRAY_BUFFER, posVBO);

        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3), mesh.vertices.data(), GL_STATIC_DRAW);

        // ==================================================
        // Triangle EBO
        // ==================================================

        glGenBuffers(1, &triangleEBO);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleEBO);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.triangles.size() * sizeof(unsigned int), mesh.triangles.data(), GL_STATIC_DRAW);

        // ==================================================
        // Main VAO
        // ==================================================

        glGenVertexArrays(1, &VAO);

        glBindVertexArray(VAO);

        // --------------------------------------------------
        // Position - location 0
        // --------------------------------------------------

        glBindBuffer(GL_ARRAY_BUFFER, posVBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

        glEnableVertexAttribArray(0);

        // --------------------------------------------------
        // Normal - location 1
        // --------------------------------------------------

        if (!mesh.normals.empty() &&
            mesh.normals.size() == mesh.vertices.size())
        {
            glGenBuffers(1, &normalVBO);

            glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
            glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(glm::vec3), mesh.normals.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

            glEnableVertexAttribArray(1);
        }

        // --------------------------------------------------
        // Color - location 2
        // --------------------------------------------------

        if (!mesh.colors.empty() &&
            mesh.colors.size() == mesh.vertices.size())
        {
            glGenBuffers(1, &colorVBO);

            glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
            glBufferData(GL_ARRAY_BUFFER, mesh.colors.size() * sizeof(glm::vec4), mesh.colors.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);

            glEnableVertexAttribArray(2);
        }

        // --------------------------------------------------
        // UV - location 3
        // --------------------------------------------------

        if (!mesh.uvs.empty() &&
            mesh.uvs.size() == mesh.vertices.size())
        {
            glGenBuffers(1, &uvVBO);

            glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
            glBufferData(GL_ARRAY_BUFFER, mesh.uvs.size() * sizeof(glm::vec2), mesh.uvs.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

            glEnableVertexAttribArray(3);
        }

        // --------------------------------------------------
        // SharedEBO
        // --------------------------------------------------

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleEBO);

        glBindVertexArray(0);

        // ==================================================
        // Lighting VAO
        // ==================================================

        glGenVertexArrays(1, &lightingVAO);

        glBindVertexArray(lightingVAO);

        // --------------------------------------------------
        // Position - location 0
        // --------------------------------------------------

        glBindBuffer(GL_ARRAY_BUFFER, posVBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

        glEnableVertexAttribArray(0);

        // --------------------------------------------------
        // Shared EBO
        // --------------------------------------------------

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleEBO);

        glBindVertexArray(0);

        // ==================================================
        // Cleanup global bindings
        // ==================================================

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void CleanupBuffers()
    {
        if (triangleEBO != 0)
        {
            glDeleteBuffers(1, &triangleEBO);
            triangleEBO = 0;
        }

        if (normalVBO != 0)
        {
            glDeleteBuffers(1, &normalVBO);
            normalVBO = 0;
        }

        if (uvVBO != 0)
        {
            glDeleteBuffers(1, &uvVBO);
            uvVBO = 0;
        }

        if (colorVBO != 0)
        {
            glDeleteBuffers(1, &colorVBO);
            colorVBO = 0;
        }

        if (posVBO != 0)
        {
            glDeleteBuffers(1, &posVBO);
            posVBO = 0;
        }

        if (lightingVAO != 0)
        {
            glDeleteVertexArrays(1, &lightingVAO);
            lightingVAO = 0;
        }

        if (VAO != 0)
        {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }

        indicesCount = 0;
    }

    bool CheckMeshValidity() const
    {
        if (mesh.vertices.empty() ||
            mesh.triangles.empty())
        {
            return false;
        }

        return true;
    }
};