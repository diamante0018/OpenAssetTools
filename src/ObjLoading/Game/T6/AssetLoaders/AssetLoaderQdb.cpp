#include "AssetLoaderQdb.h"

#include "Game/T6/T6.h"
#include "Pool/GlobalAssetPool.h"

#include <cstring>

using namespace T6;

void* AssetLoaderQdb::CreateEmptyAsset(const std::string& assetName, MemoryManager* memory)
{
    auto* asset = memory->Alloc<AssetQdb::Type>();
    asset->name = memory->Dup(assetName.c_str());
    return asset;
}

bool AssetLoaderQdb::CanLoadFromRaw() const
{
    return true;
}

bool AssetLoaderQdb::LoadFromRaw(const std::string& assetName, ISearchPath* searchPath, MemoryManager* memory, IAssetLoadingManager* manager, Zone* zone) const
{
    const auto file = searchPath->Open(assetName);
    if (!file.IsOpen())
        return false;

    auto* qdb = memory->Create<Qdb>();
    qdb->name = memory->Dup(assetName.c_str());
    qdb->len = static_cast<int>(file.m_length);

    auto* fileBuffer = memory->Alloc<char>(static_cast<size_t>(file.m_length + 1));
    file.m_stream->read(fileBuffer, file.m_length);
    if (file.m_stream->gcount() != file.m_length)
        return false;
    fileBuffer[qdb->len] = '\0';

    qdb->buffer = static_cast<char16*>(fileBuffer);
    manager->AddAsset<AssetQdb>(assetName, qdb);

    return true;
}
