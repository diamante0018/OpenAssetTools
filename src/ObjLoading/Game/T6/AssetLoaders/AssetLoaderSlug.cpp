#include "AssetLoaderSlug.h"

#include "Game/T6/T6.h"
#include "Pool/GlobalAssetPool.h"

#include <cstring>

using namespace T6;

void* AssetLoaderSlug::CreateEmptyAsset(const std::string& assetName, MemoryManager* memory)
{
    auto* asset = memory->Alloc<AssetSlug::Type>();
    asset->name = memory->Dup(assetName.c_str());
    return asset;
}

bool AssetLoaderSlug::CanLoadFromRaw() const
{
    return true;
}

bool AssetLoaderSlug::LoadFromRaw(const std::string& assetName, ISearchPath* searchPath, MemoryManager* memory, IAssetLoadingManager* manager, Zone* zone) const
{
    const auto file = searchPath->Open(assetName);
    if (!file.IsOpen())
        return false;

    auto* slug = memory->Create<Slug>();
    slug->name = memory->Dup(assetName.c_str());
    slug->len = static_cast<int>(file.m_length);

    auto* fileBuffer = memory->Alloc<char>(static_cast<size_t>(file.m_length + 1));
    file.m_stream->read(fileBuffer, file.m_length);
    if (file.m_stream->gcount() != file.m_length)
        return false;
    fileBuffer[slug->len] = '\0';

    slug->buffer = static_cast<char16*>(fileBuffer);
    manager->AddAsset<AssetSlug>(assetName, slug);

    return true;
}
