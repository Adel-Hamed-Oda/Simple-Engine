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
#include <utility>
#include <vector>

class ModelLoader
{
public:
    static Mesh* Create(
        const std::string& path,
        glm::vec4 defaultColor = glm::vec4(1.0f)
    )
    {
        return LoadMesh(path, defaultColor);
    }

    static Mesh* LoadMesh(
        const std::string& path,
        glm::vec4 defaultColor = glm::vec4(1.0f)
    )
    {
        Mesh* mesh = new Mesh();
        if (!LoadModelInternal(path, defaultColor, *mesh))
        {
            std::cerr << "ERROR::MODEL_LOADER::CREATION_FAILED: " << path << std::endl;
        }

        return mesh;
    }

private:
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
        std::vector<std::pair<std::string, Texture*>>& loadedTextures,
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

                const auto existing = std::find_if(
                    loadedTextures.begin(),
                    loadedTextures.end(),
                    [&texRelativePath](const std::pair<std::string, Texture*>& entry)
                    {
                        return entry.first == texRelativePath;
                    }
                );

                if (existing == loadedTextures.end())
                {
                    Texture* tex = Texture::Create(texRelativePath);
                    if (tex && tex->IsValid())
                    {
                        loadedTextures.emplace_back(texRelativePath, tex);
                        outMesh.textures.push_back(tex);
                    }
                    else
                    {
                        delete tex;
                    }
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
        std::vector<std::pair<std::string, Texture*>>& loadedTextures,
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

                // Load base/diffuse textures into outMesh.textures (std::vector<Texture*>)
                LoadMaterialBaseTextures(material, aiTextureType_DIFFUSE, modelDirectory, loadedTextures, outMesh);
                LoadMaterialBaseTextures(material, aiTextureType_BASE_COLOR, modelDirectory, loadedTextures, outMesh);
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
        std::vector<std::pair<std::string, Texture*>>& loadedTextures,
        Mesh& outMesh
    )
    {
        if (!node || !scene)
            return;

        const glm::mat4 nodeTransform = parentTransform * ConvertMatrix(node->mTransformation);

        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessMesh(mesh, scene, nodeTransform, modelDirectory, defaultColor, loadedTextures, outMesh);
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(node->mChildren[i], scene, nodeTransform, modelDirectory, defaultColor, loadedTextures, outMesh);
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
        std::vector<std::pair<std::string, Texture*>> loadedTextures;
        ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f), modelDirectory, defaultColor, loadedTextures, outMesh);

        if (outMesh.vertices.empty() || outMesh.triangles.empty())
        {
            std::cerr << "ERROR::MODEL_LOADER::EMPTY_MESH: " << fullPath << std::endl;
            return false;
        }

        return true;
    }
};
