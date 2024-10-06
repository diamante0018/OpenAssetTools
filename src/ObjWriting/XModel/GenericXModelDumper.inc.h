#pragma once

#ifndef GAME_NAMESPACE
#error Must define GAME_NAMESPACE
#endif

#include "Game/T6/CommonT6.h"
#include "ObjWriting.h"
#include "Utils/DistinctMapper.h"
#include "Utils/QuatInt16.h"
#include "XModel/Export/XModelExportWriter.h"
#include "XModel/Gltf/GltfBinOutput.h"
#include "XModel/Gltf/GltfTextOutput.h"
#include "XModel/Gltf/GltfWriter.h"
#include "XModel/Obj/ObjWriter.h"
#include "XModel/XModelWriter.h"

#include <cassert>
#include <format>

namespace GAME_NAMESPACE
{
    inline std::string GetFileNameForLod(const std::string& modelName, const unsigned lod, const std::string& extension)
    {
        return std::format("model_export/{}_lod{}{}", modelName, lod, extension);
    }

    inline GfxImage* GetImageFromTextureDef(const MaterialTextureDef& textureDef)
    {
#ifdef FEATURE_T6
        return textureDef.image;
#else
        return textureDef.u.image;
#endif
    }

    inline GfxImage* GetMaterialColorMap(const Material* material)
    {
        std::vector<MaterialTextureDef*> potentialTextureDefs;

        for (auto textureIndex = 0u; textureIndex < material->textureCount; textureIndex++)
        {
            MaterialTextureDef* def = &material->textureTable[textureIndex];

            if (def->semantic == TS_COLOR_MAP || def->semantic >= TS_COLOR0_MAP && def->semantic <= TS_COLOR15_MAP)
                potentialTextureDefs.push_back(def);
        }

        if (potentialTextureDefs.empty())
            return nullptr;
        if (potentialTextureDefs.size() == 1)
            return GetImageFromTextureDef(*potentialTextureDefs[0]);

        for (const auto* def : potentialTextureDefs)
        {
            if (tolower(def->nameStart) == 'c' && tolower(def->nameEnd) == 'p')
                return GetImageFromTextureDef(*def);
        }

        for (const auto* def : potentialTextureDefs)
        {
            if (tolower(def->nameStart) == 'r' && tolower(def->nameEnd) == 'k')
                return GetImageFromTextureDef(*def);
        }

        for (const auto* def : potentialTextureDefs)
        {
            if (tolower(def->nameStart) == 'd' && tolower(def->nameEnd) == 'p')
                return GetImageFromTextureDef(*def);
        }

        return GetImageFromTextureDef(*potentialTextureDefs[0]);
    }

    inline GfxImage* GetMaterialNormalMap(const Material* material)
    {
        std::vector<MaterialTextureDef*> potentialTextureDefs;

        for (auto textureIndex = 0u; textureIndex < material->textureCount; textureIndex++)
        {
            MaterialTextureDef* def = &material->textureTable[textureIndex];

            if (def->semantic == TS_NORMAL_MAP)
                potentialTextureDefs.push_back(def);
        }

        if (potentialTextureDefs.empty())
            return nullptr;
        if (potentialTextureDefs.size() == 1)
            return GetImageFromTextureDef(*potentialTextureDefs[0]);

        for (const auto* def : potentialTextureDefs)
        {
            if (def->nameStart == 'n' && def->nameEnd == 'p')
                return GetImageFromTextureDef(*def);
        }

        return GetImageFromTextureDef(*potentialTextureDefs[0]);
    }

    inline GfxImage* GetMaterialSpecularMap(const Material* material)
    {
        std::vector<MaterialTextureDef*> potentialTextureDefs;

        for (auto textureIndex = 0u; textureIndex < material->textureCount; textureIndex++)
        {
            MaterialTextureDef* def = &material->textureTable[textureIndex];

            if (def->semantic == TS_SPECULAR_MAP)
                potentialTextureDefs.push_back(def);
        }

        if (potentialTextureDefs.empty())
            return nullptr;
        if (potentialTextureDefs.size() == 1)
            return GetImageFromTextureDef(*potentialTextureDefs[0]);

        for (const auto* def : potentialTextureDefs)
        {
            if (def->nameStart == 's' && def->nameEnd == 'p')
                return GetImageFromTextureDef(*def);
        }

        return GetImageFromTextureDef(*potentialTextureDefs[0]);
    }

    inline bool HasDefaultArmature(const XModel* model, const unsigned lod)
    {
        if (model->numRootBones != 1 || model->numBones != 1)
            return false;

        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;

        if (!surfs)
            return true;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];

            if (surface.vertListCount != 1 || surface.vertInfo.vertsBlend)
                return false;

            const auto& vertList = surface.vertList[0];
            if (vertList.boneOffset != 0 || vertList.triOffset != 0 || vertList.triCount != surface.triCount || vertList.vertCount != surface.vertCount)
                return false;
        }

        return true;
    }

    inline void OmitDefaultArmature(XModelCommon& common)
    {
        common.m_bones.clear();
        common.m_bone_weight_data.weights.clear();
        common.m_vertex_bone_weights.resize(common.m_vertices.size());
        for (auto& vertexWeights : common.m_vertex_bone_weights)
        {
            vertexWeights.weightOffset = 0u;
            vertexWeights.weightCount = 0u;
        }
    }

    inline void AddXModelBones(XModelCommon& out, const AssetDumpingContext& context, const XModel* model)
    {
        for (auto boneNum = 0u; boneNum < model->numBones; boneNum++)
        {
            XModelBone bone;
            if (model->boneNames[boneNum] < context.m_zone->m_script_strings.Count())
                bone.name = context.m_zone->m_script_strings[model->boneNames[boneNum]];
            else
                bone.name = "INVALID_BONE_NAME";

            if (boneNum >= model->numRootBones)
                bone.parentIndex = static_cast<int>(boneNum - static_cast<unsigned int>(model->parentList[boneNum - model->numRootBones]));
            else
                bone.parentIndex = std::nullopt;

            bone.scale[0] = 1.0f;
            bone.scale[1] = 1.0f;
            bone.scale[2] = 1.0f;

            const auto& baseMat = model->baseMat[boneNum];
            bone.globalOffset[0] = baseMat.trans.x;
            bone.globalOffset[1] = baseMat.trans.y;
            bone.globalOffset[2] = baseMat.trans.z;
            bone.globalRotation = {
                baseMat.quat.x,
                baseMat.quat.y,
                baseMat.quat.z,
                baseMat.quat.w,
            };

            if (boneNum < model->numRootBones)
            {
                bone.localOffset[0] = 0;
                bone.localOffset[1] = 0;
                bone.localOffset[2] = 0;
                bone.localRotation = {0, 0, 0, 1};
            }
            else
            {
                const auto* trans = &model->trans[(boneNum - model->numRootBones) * 3];
                bone.localOffset[0] = trans[0];
                bone.localOffset[1] = trans[1];
                bone.localOffset[2] = trans[2];

                const auto& quat = model->quats[boneNum - model->numRootBones];
                bone.localRotation = {
                    QuatInt16::ToFloat(quat.v[0]),
                    QuatInt16::ToFloat(quat.v[1]),
                    QuatInt16::ToFloat(quat.v[2]),
                    QuatInt16::ToFloat(quat.v[3]),
                };
            }

            out.m_bones.emplace_back(std::move(bone));
        }
    }

    inline const char* AssetName(const char* input)
    {
        if (input && input[0] == ',')
            return &input[1];

        return input;
    }

    inline void AddXModelMaterials(XModelCommon& out, DistinctMapper<Material*>& materialMapper, const XModel* model)
    {
        for (auto surfaceMaterialNum = 0; surfaceMaterialNum < model->numsurfs; surfaceMaterialNum++)
        {
            Material* material = model->materialHandles[surfaceMaterialNum];
            if (materialMapper.Add(material))
            {
                XModelMaterial xMaterial;
                xMaterial.ApplyDefaults();

                xMaterial.name = AssetName(material->info.name);
                const auto* colorMap = GetMaterialColorMap(material);
                if (colorMap)
                    xMaterial.colorMapName = AssetName(colorMap->name);

                const auto* normalMap = GetMaterialNormalMap(material);
                if (normalMap)
                    xMaterial.normalMapName = AssetName(normalMap->name);

                const auto* specularMap = GetMaterialSpecularMap(material);
                if (specularMap)
                    xMaterial.specularMapName = AssetName(specularMap->name);

                out.m_materials.emplace_back(std::move(xMaterial));
            }
        }
    }

    inline void AddXModelObjects(XModelCommon& out, const XModel* model, const unsigned lod, const DistinctMapper<Material*>& materialMapper)
    {
        const auto surfCount = model->lodInfo[lod].numsurfs;
        const auto baseSurfaceIndex = model->lodInfo[lod].surfIndex;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            XModelObject object;
            object.name = std::format("surf{}", surfIndex);
            object.materialIndex = static_cast<int>(materialMapper.GetDistinctPositionByInputPosition(surfIndex + baseSurfaceIndex));

            out.m_objects.emplace_back(std::move(object));
        }
    }

    inline void AddXModelVertices(XModelCommon& out, const XModel* model, const unsigned lod)
    {
        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;

        if (!surfs)
            return;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];

            for (auto vertexIndex = 0u; vertexIndex < surface.vertCount; vertexIndex++)
            {
                const auto& v = surface.verts0[vertexIndex];

                XModelVertex vertex{};
                vertex.coordinates[0] = v.xyz.x;
                vertex.coordinates[1] = v.xyz.y;
                vertex.coordinates[2] = v.xyz.z;
                Common::Vec3UnpackUnitVec(v.normal, vertex.normal);
                Common::Vec4UnpackGfxColor(v.color, vertex.color);
                Common::Vec2UnpackTexCoords(v.texCoord, vertex.uv);

                out.m_vertices.emplace_back(vertex);
            }
        }
    }

    inline void AllocateXModelBoneWeights(const XModel* model, const unsigned lod, XModelVertexBoneWeightCollection& weightCollection)
    {
        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;

        if (!surfs)
            return;

        auto totalWeightCount = 0u;
        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];

            if (surface.vertList)
            {
                totalWeightCount += surface.vertListCount;
            }

            if (surface.vertInfo.vertsBlend)
            {
                totalWeightCount += surface.vertInfo.vertCount[0] * 1;
                totalWeightCount += surface.vertInfo.vertCount[1] * 2;
                totalWeightCount += surface.vertInfo.vertCount[2] * 3;
                totalWeightCount += surface.vertInfo.vertCount[3] * 4;
            }
        }

        weightCollection.weights.resize(totalWeightCount);
    }

    inline float BoneWeight16(const uint16_t value)
    {
        return static_cast<float>(value) / static_cast<float>(std::numeric_limits<uint16_t>::max());
    }

    inline void AddXModelVertexBoneWeights(XModelCommon& out, const XModel* model, const unsigned lod)
    {
        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;
        auto& weightCollection = out.m_bone_weight_data;

        if (!surfs)
            return;

        size_t weightOffset = 0u;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];
            auto handledVertices = 0u;

            if (surface.vertList)
            {
                for (auto vertListIndex = 0u; vertListIndex < surface.vertListCount; vertListIndex++)
                {
                    const auto& vertList = surface.vertList[vertListIndex];
                    const auto boneWeightOffset = weightOffset;

                    weightCollection.weights[weightOffset++] = XModelBoneWeight{vertList.boneOffset / sizeof(DObjSkelMat), 1.0f};

                    for (auto vertListVertexOffset = 0u; vertListVertexOffset < vertList.vertCount; vertListVertexOffset++)
                    {
                        out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 1);
                    }
                    handledVertices += vertList.vertCount;
                }
            }

            auto vertsBlendOffset = 0u;
            if (surface.vertInfo.vertsBlend)
            {
                // 1 bone weight
                for (auto vertIndex = 0; vertIndex < surface.vertInfo.vertCount[0]; vertIndex++)
                {
                    const auto boneWeightOffset = weightOffset;
                    const auto boneIndex0 = surface.vertInfo.vertsBlend[vertsBlendOffset + 0] / sizeof(DObjSkelMat);
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex0, 1.0f};

                    vertsBlendOffset += 1;

                    out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 1);
                }

                // 2 bone weights
                for (auto vertIndex = 0; vertIndex < surface.vertInfo.vertCount[1]; vertIndex++)
                {
                    const auto boneWeightOffset = weightOffset;
                    const auto boneIndex0 = surface.vertInfo.vertsBlend[vertsBlendOffset + 0] / sizeof(DObjSkelMat);
                    const auto boneIndex1 = surface.vertInfo.vertsBlend[vertsBlendOffset + 1] / sizeof(DObjSkelMat);
                    const auto boneWeight1 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 2]);
                    const auto boneWeight0 = 1.0f - boneWeight1;

                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex0, boneWeight0};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex1, boneWeight1};

                    vertsBlendOffset += 3;

                    out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 2);
                }

                // 3 bone weights
                for (auto vertIndex = 0; vertIndex < surface.vertInfo.vertCount[2]; vertIndex++)
                {
                    const auto boneWeightOffset = weightOffset;
                    const auto boneIndex0 = surface.vertInfo.vertsBlend[vertsBlendOffset + 0] / sizeof(DObjSkelMat);
                    const auto boneIndex1 = surface.vertInfo.vertsBlend[vertsBlendOffset + 1] / sizeof(DObjSkelMat);
                    const auto boneWeight1 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 2]);
                    const auto boneIndex2 = surface.vertInfo.vertsBlend[vertsBlendOffset + 3] / sizeof(DObjSkelMat);
                    const auto boneWeight2 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 4]);
                    const auto boneWeight0 = 1.0f - boneWeight1 - boneWeight2;

                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex0, boneWeight0};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex1, boneWeight1};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex2, boneWeight2};

                    vertsBlendOffset += 5;

                    out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 3);
                }

                // 4 bone weights
                for (auto vertIndex = 0; vertIndex < surface.vertInfo.vertCount[3]; vertIndex++)
                {
                    const auto boneWeightOffset = weightOffset;
                    const auto boneIndex0 = surface.vertInfo.vertsBlend[vertsBlendOffset + 0] / sizeof(DObjSkelMat);
                    const auto boneIndex1 = surface.vertInfo.vertsBlend[vertsBlendOffset + 1] / sizeof(DObjSkelMat);
                    const auto boneWeight1 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 2]);
                    const auto boneIndex2 = surface.vertInfo.vertsBlend[vertsBlendOffset + 3] / sizeof(DObjSkelMat);
                    const auto boneWeight2 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 4]);
                    const auto boneIndex3 = surface.vertInfo.vertsBlend[vertsBlendOffset + 5] / sizeof(DObjSkelMat);
                    const auto boneWeight3 = BoneWeight16(surface.vertInfo.vertsBlend[vertsBlendOffset + 6]);
                    const auto boneWeight0 = 1.0f - boneWeight1 - boneWeight2 - boneWeight3;

                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex0, boneWeight0};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex1, boneWeight1};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex2, boneWeight2};
                    weightCollection.weights[weightOffset++] = XModelBoneWeight{boneIndex3, boneWeight3};

                    vertsBlendOffset += 7;

                    out.m_vertex_bone_weights.emplace_back(boneWeightOffset, 4);
                }

                handledVertices +=
                    surface.vertInfo.vertCount[0] + surface.vertInfo.vertCount[1] + surface.vertInfo.vertCount[2] + surface.vertInfo.vertCount[3];
            }

            for (; handledVertices < surface.vertCount; handledVertices++)
            {
                out.m_vertex_bone_weights.emplace_back(0, 0);
            }
        }
    }

    inline void AddXModelFaces(XModelCommon& out, const XModel* model, const unsigned lod)
    {
        const auto* surfs = &model->surfs[model->lodInfo[lod].surfIndex];
        const auto surfCount = model->lodInfo[lod].numsurfs;

        if (!surfs)
            return;

        for (auto surfIndex = 0u; surfIndex < surfCount; surfIndex++)
        {
            const auto& surface = surfs[surfIndex];
            auto& object = out.m_objects[surfIndex];
            object.m_faces.reserve(surface.triCount);

            for (auto triIndex = 0u; triIndex < surface.triCount; triIndex++)
            {
                const auto& tri = surface.triIndices[triIndex];

                XModelFace face{};
                face.vertexIndex[0] = tri.i[0] + surface.baseVertIndex;
                face.vertexIndex[1] = tri.i[1] + surface.baseVertIndex;
                face.vertexIndex[2] = tri.i[2] + surface.baseVertIndex;
                object.m_faces.emplace_back(face);
            }
        }
    }

    inline void PopulateXModelWriter(XModelCommon& out, const AssetDumpingContext& context, const unsigned lod, const XModel* model)
    {
        DistinctMapper<Material*> materialMapper(model->numsurfs);
        AllocateXModelBoneWeights(model, lod, out.m_bone_weight_data);

        out.m_name = std::format("{}_lod{}", model->name, lod);
        AddXModelMaterials(out, materialMapper, model);
        AddXModelObjects(out, model, lod, materialMapper);
        AddXModelVertices(out, model, lod);
        AddXModelFaces(out, model, lod);

        if (!HasDefaultArmature(model, lod))
        {
            AddXModelBones(out, context, model);
            AddXModelVertexBoneWeights(out, model, lod);
        }
        else
        {
            OmitDefaultArmature(out);
        }
    }

    inline void DumpObjMtl(const XModelCommon& common, const AssetDumpingContext& context, const XAssetInfo<XModel>* asset)
    {
        const auto* model = asset->Asset();
        const auto mtlFile = context.OpenAssetFile(std::format("model_export/{}.mtl", model->name));

        if (!mtlFile)
            return;

        const auto writer = obj::CreateMtlWriter(*mtlFile, context.m_zone->m_game->GetShortName(), context.m_zone->m_name);
        DistinctMapper<Material*> materialMapper(model->numsurfs);

        writer->Write(common);
    }

    inline void DumpObjLod(const XModelCommon& common, const AssetDumpingContext& context, const XAssetInfo<XModel>* asset, const unsigned lod)
    {
        const auto* model = asset->Asset();
        const auto assetFile = context.OpenAssetFile(GetFileNameForLod(model->name, lod, ".obj"));

        if (!assetFile)
            return;

        const auto writer =
            obj::CreateObjWriter(*assetFile, std::format("{}.mtl", model->name), context.m_zone->m_game->GetShortName(), context.m_zone->m_name);
        DistinctMapper<Material*> materialMapper(model->numsurfs);

        writer->Write(common);
    }

    inline void DumpXModelExportLod(const XModelCommon& common, const AssetDumpingContext& context, const XAssetInfo<XModel>* asset, const unsigned lod)
    {
        const auto* model = asset->Asset();
        const auto assetFile = context.OpenAssetFile(GetFileNameForLod(model->name, lod, ".XMODEL_EXPORT"));

        if (!assetFile)
            return;

        const auto writer = xmodel_export::CreateWriterForVersion6(*assetFile, context.m_zone->m_game->GetShortName(), context.m_zone->m_name);
        writer->Write(common);
    }

    template<typename T>
    void DumpGltfLod(
        const XModelCommon& common, const AssetDumpingContext& context, const XAssetInfo<XModel>* asset, const unsigned lod, const std::string& extension)
    {
        const auto* model = asset->Asset();
        const auto assetFile = context.OpenAssetFile(GetFileNameForLod(model->name, lod, extension));

        if (!assetFile)
            return;

        const auto output = std::make_unique<T>(*assetFile);
        const auto writer = gltf::Writer::CreateWriter(output.get(), context.m_zone->m_game->GetShortName(), context.m_zone->m_name);

        writer->Write(common);
    }

    inline void DumpXModelSurfs(const AssetDumpingContext& context, const XAssetInfo<XModel>* asset)
    {
        const auto* model = asset->Asset();

        for (auto currentLod = 0u; currentLod < model->numLods; currentLod++)
        {
            XModelCommon common;
            PopulateXModelWriter(common, context, currentLod, asset->Asset());

            switch (ObjWriting::Configuration.ModelOutputFormat)
            {
            case ObjWriting::Configuration_t::ModelOutputFormat_e::OBJ:
                DumpObjLod(common, context, asset, currentLod);
                if (currentLod == 0u)
                    DumpObjMtl(common, context, asset);
                break;

            case ObjWriting::Configuration_t::ModelOutputFormat_e::XMODEL_EXPORT:
                DumpXModelExportLod(common, context, asset, currentLod);
                break;

            case ObjWriting::Configuration_t::ModelOutputFormat_e::GLTF:
                DumpGltfLod<gltf::TextOutput>(common, context, asset, currentLod, ".gltf");
                break;

            case ObjWriting::Configuration_t::ModelOutputFormat_e::GLB:
                DumpGltfLod<gltf::BinOutput>(common, context, asset, currentLod, ".glb");
                break;

            default:
                assert(false);
                break;
            }
        }
    }

    class JsonDumper
    {
    public:
        JsonDumper(AssetDumpingContext& context, std::ostream& stream)
            : m_stream(stream)
        {
        }

        void Dump(const XModel* xmodel) const
        {
            JsonXModel jsonXModel;
            CreateJsonXModel(jsonXModel, *xmodel);
            nlohmann::json jRoot = jsonXModel;

            jRoot["_type"] = "xmodel";
            jRoot["_version"] = 1;

            m_stream << std::setw(4) << jRoot << "\n";
        }

    private:
        static const char* AssetName(const char* input)
        {
            if (input && input[0] == ',')
                return &input[1];

            return input;
        }

        static const char* GetExtensionForModelByConfig()
        {
            switch (ObjWriting::Configuration.ModelOutputFormat)
            {
            case ObjWriting::Configuration_t::ModelOutputFormat_e::XMODEL_EXPORT:
                return ".XMODEL_EXPORT";
            case ObjWriting::Configuration_t::ModelOutputFormat_e::OBJ:
                return ".OBJ";
            case ObjWriting::Configuration_t::ModelOutputFormat_e::GLTF:
                return ".GLTF";
            case ObjWriting::Configuration_t::ModelOutputFormat_e::GLB:
                return ".GLB";
            default:
                assert(false);
                return "";
            }
        }

        static void CreateJsonXModel(JsonXModel& jXModel, const XModel& xmodel)
        {
            if (xmodel.collLod >= 0)
                jXModel.collLod = xmodel.collLod;

            for (auto lodNumber = 0u; lodNumber < xmodel.numLods; lodNumber++)
            {
                JsonXModelLod lod;
                lod.file = std::format("model_export/{}_lod{}{}", xmodel.name, lodNumber, GetExtensionForModelByConfig());
                lod.distance = xmodel.lodInfo[lodNumber].dist;

                jXModel.lods.emplace_back(std::move(lod));
            }

            if (xmodel.physPreset && xmodel.physPreset->name)
                jXModel.physPreset = AssetName(xmodel.physPreset->name);

            if (xmodel.physConstraints && xmodel.physConstraints->name)
                jXModel.physConstraints = AssetName(xmodel.physConstraints->name);

            jXModel.flags = xmodel.flags;

#ifdef FEATURE_T6
            jXModel.lightingOriginOffset.x = xmodel.lightingOriginOffset.x;
            jXModel.lightingOriginOffset.y = xmodel.lightingOriginOffset.y;
            jXModel.lightingOriginOffset.z = xmodel.lightingOriginOffset.z;
            jXModel.lightingOriginRange = xmodel.lightingOriginRange;
#endif
        }

        std::ostream& m_stream;
    };

    inline void DumpXModelJson(AssetDumpingContext& context, XAssetInfo<XModel>* asset)
    {
        const auto assetFile = context.OpenAssetFile(std::format("xmodel/{}.json", asset->m_name));
        if (!assetFile)
            return;

        const JsonDumper dumper(context, *assetFile);
        dumper.Dump(asset->Asset());
    }
} // namespace GAME_NAMESPACE