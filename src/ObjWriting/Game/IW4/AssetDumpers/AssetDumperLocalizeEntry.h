#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4/IW4.h"

namespace IW4
{
    class AssetDumperLocalizeEntry final : public IAssetDumper<LocalizeEntry>
    {
    public:
        void DumpPool(Zone* zone, AssetPool<LocalizeEntry>* pool, const std::string& basePath) override;
    };
}