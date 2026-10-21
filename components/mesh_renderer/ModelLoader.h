#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "../../Config.h"
#include "../../core/rendering/texture/Texture.h"
#include "Mesh.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class ModelLoader
{
public:
    // =========================================================================
    // Static Storage & Model Management (mirrors Shader & Texture)
    // =========================================================================

    static Mesh* Create(
        const std::string& path,
        const std::string& name = "",
        glm::vec4 defaultColor = glm::vec4(1.0f)
    )
    {
        std::string name_t = name.empty() ? path : name;
        auto existing = models.find(name_t);
        if (existing != models.end())
        {
            std::cerr << "ERROR::MODEL_LOADER::ALREADY_EXISTS: " << name_t << std::endl;
            return &existing->second;
        }

        Mesh mesh;
        if (!LoadModelInternal(path, defaultColor, mesh))
        {
            std::cerr << "ERROR::MODEL_LOADER::CREATION_FAILED: " << name_t << std::endl;
            return nullptr;
        }

        auto [it, inserted] = models.insert_or_assign(name_t, std::move(mesh));
        return &it->second;
    }

    // Look up a loaded model mesh by name. Returns nullptr and logs error if missing.
    static Mesh* Get(const std::string& name)
    {
        auto it = models.find(name);
        if (it == models.end())
        {
            std::cerr << "ERROR::MODEL_LOADER::NOT_FOUND: " << name << std::endl;
            return nullptr;
        }
        return &it->second;
    }

    static bool Has(const std::string& name)
    {
        return models.find(name) != models.end();
    }

    // Loads a Mesh from file (or returns the cached copy if already loaded under `path`).
    static Mesh LoadMesh(
        const std::string& path,
        glm::vec4 defaultColor = glm::vec4(1.0f)
    )
    {
        auto it = models.find(path);
        if (it != models.end())
        {
            return it->second;
        }

        Mesh* created = Create(path, path, defaultColor);
        if (!created)
        {
            return Mesh{};
        }

        return *created;
    }

    static void Clear()
    {
        models.clear();
    }

private:
    inline static std::unordered_map<std::string, Mesh> models;

    static glm::mat4 ConvertMatrix(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        // Assimp matrices are row-major; GLM matrices are column-major
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    static std::string GetDirectory(const std::string& filepath)
    {
        std::string normalized = filepath;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        const size_t lastSlash = normalized.find_last_of('/');
        if (lastSlash == std::string::npos)
            return "";
        return normalized.substr(0, lastSlash + 1);
    }

    static std::string NormalizeTexturePath(const std::string& modelDirectory, const char* rawTexPath)
    {
        std::string texPath(rawTexPath ? rawTexPath : "");
        std::replace(texPath.begin(), texPath.end(), '\\', '/');

        // Strip leading "./" if present
        if (texPath.rfind("./", 0) == 0)
            texPath = texPath.substr(2);

        return modelDirectory + texPath;
    }

    static void LoadMaterialBaseTextures(
        const aiMaterial* mat,
        aiTextureType type,
        const std::string& modelDirectory,
        Mesh& outMesh
    )
    {
        if (!mat)
            return;

        const unsigned int count = mat->GetTextureCount(type);
        for (unsigned int i = 0; i < count; ++i)
        {
            aiString str;
            if (mat->GetTexture(type, i, &str) == AI_SUCCESS)
            {
                // Skip embedded textures ("*0") since only file-based base textures are used for now
                if (str.length == 0 || str.C_Str()[0] == '*')
                    continue;

                const std::string texRelativePath = NormalizeTexturePath(modelDirectory, str.C_Str());

                if (!Texture::Has(texRelativePath))
                {
                    Texture::Create(texRelativePath, texRelativePath);
                }

                if (outMesh.textureName.empty())
                {
                    outMesh.textureName = texRelativePath;
                }

                if (std::find(outMesh.textureNames.begin(), outMesh.textureNames.end(), texRelativePath) == outMesh.textureNames.end())
                {
                    outMesh.textureNames.push_back(texRelativePath);
                }
            }
        }
    }

    static void ProcessMesh(
        const aiMesh* mesh,
        const aiScene* scene,
        const glm::mat4& transform,
        const std::string& modelDirectory,
        const glm::vec4& defaultColor,
        Mesh& outMesh
    )
    {
        if (!mesh)
            return;

        const unsigned int baseVertex = static_cast<unsigned int>(outMesh.vertices.size());
        const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(transform));

        // Check if material specifies a diffuse color fallback when vertex colors are absent
        glm::vec4 materialColor = defaultColor;
        if (scene && mesh->mMaterialIndex < scene->mNumMaterials)
        {
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            if (material)
            {
                aiColor4D diffuseColor;
                if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor) == AI_SUCCESS)
                {
                    materialColor = glm::vec4(
                        diffuseColor.r,
                        diffuseColor.g,
                        diffuseColor.b,
                        diffuseColor.a
                    ) * defaultColor;
                }

                // Load base/diffuse textures (ignoring normal maps, specular maps, etc.)
                LoadMaterialBaseTextures(material, aiTextureType_DIFFUSE, modelDirectory, outMesh);
                LoadMaterialBaseTextures(material, aiTextureType_BASE_COLOR, modelDirectory, outMesh);
            }
        }

        outMesh.vertices.reserve(outMesh.vertices.size() + mesh->mNumVertices);
        outMesh.normals.reserve(outMesh.normals.size() + mesh->mNumVertices);
        outMesh.uvs.reserve(outMesh.uvs.size() + mesh->mNumVertices);
        outMesh.colors.reserve(outMesh.colors.size() + mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            // 1. Vertex Position
            const aiVector3D& pos = mesh->mVertices[i];
            const glm::vec4 worldPos = transform * glm::vec4(pos.x, pos.y, pos.z, 1.0f);
            outMesh.vertices.emplace_back(worldPos.x, worldPos.y, worldPos.z);

            // 2. Vertex Normal
            if (mesh->HasNormals())
            {
                const aiVector3D& norm = mesh->mNormals[i];
                glm::vec3 transformedNormal = normalMatrix * glm::vec3(norm.x, norm.y, norm.z);
                const float len = glm::length(transformedNormal);
                if (len > 0.0001f)
                    transformedNormal /= len;
                outMesh.normals.push_back(transformedNormal);
            }
            else
            {
                outMesh.normals.emplace_back(0.0f, 1.0f, 0.0f);
            }

            // 3. Vertex UVs (channel 0)
            if (mesh->mTextureCoords[0])
            {
                const aiVector3D& uv = mesh->mTextureCoords[0][i];
                outMesh.uvs.emplace_back(uv.x, uv.y);
            }
            else
            {
                outMesh.uvs.emplace_back(0.0f, 0.0f);
            }

            // 4. Vertex Colors (channel 0, or fallback to material/default color)
            if (mesh->HasVertexColors(0) && mesh->mColors[0])
            {
                const aiColor4D& col = mesh->mColors[0][i];
                outMesh.colors.emplace_back(col.r, col.g, col.b, col.a);
            }
            else
            {
                outMesh.colors.push_back(materialColor);
            }
        }

        // 5. Triangles (Indices)
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace& face = mesh->mFaces[i];
            if (face.mNumIndices != 3)
                continue;

            outMesh.triangles.push_back(baseVertex + face.mIndices[0]);
            outMesh.triangles.push_back(baseVertex + face.mIndices[1]);
            outMesh.triangles.push_back(baseVertex + face.mIndices[2]);
        }
    }

    static void ProcessNode(
        const aiNode* node,
        const aiScene* scene,
        const glm::mat4& parentTransform,
        const std::string& modelDirectory,
        const glm::vec4& defaultColor,
        Mesh& outMesh
    )
    {
        if (!node || !scene)
            return;

        const glm::mat4 nodeTransform = parentTransform * ConvertMatrix(node->mTransformation);

        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessMesh(mesh, scene, nodeTransform, modelDirectory, defaultColor, outMesh);
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(node->mChildren[i], scene, nodeTransform, modelDirectory, defaultColor, outMesh);
        }
    }

    static bool LoadModelInternal(
        const std::string& path,
        const glm::vec4& defaultColor,
        Mesh& outMesh
    )
    {
        const std::string fullPath = CONFIG::DIRECTORY::MODELS + path;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            fullPath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices
        );

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
        {
            std::cerr << "ERROR::MODEL_LOADER::ASSIMP_LOAD_FAILED: "
                      << fullPath << " (" << importer.GetErrorString() << ")" << std::endl;
            return false;
        }

        const std::string modelDirectory = GetDirectory(path);
        ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f), modelDirectory, defaultColor, outMesh);

        if (outMesh.vertices.empty() || outMesh.triangles.empty())
        {
            std::cerr << "ERROR::MODEL_LOADER::EMPTY_MESH: " << fullPath << std::endl;
            return false;
        }

        return true;
    }
};

