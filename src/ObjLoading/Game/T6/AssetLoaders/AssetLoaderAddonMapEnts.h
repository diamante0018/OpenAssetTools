#pragma once

#include "AssetLoading/BasicAssetLoader.h"
#include "Game/T6/T6.h"
#include "SearchPath/ISearchPath.h"

namespace T6
{
    class AssetLoaderAddonMapEnts final : public BasicAssetLoader<AssetAddonMapEnts>
    {
    public:
        _NODISCARD void* CreateEmptyAsset(const std::string& assetName, MemoryManager* memory) override;
    };
} // namespace T6