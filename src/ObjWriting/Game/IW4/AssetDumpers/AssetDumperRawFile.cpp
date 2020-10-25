#include "AssetDumperRawFile.h"

#include <zlib.h>

using namespace IW4;

bool AssetDumperRawFile::ShouldDump(XAssetInfo<RawFile>* asset)
{
    return true;
}

std::string AssetDumperRawFile::GetFileNameForAsset(Zone* zone, XAssetInfo<RawFile>* asset)
{
    return asset->m_name;
}

void AssetDumperRawFile::DumpAsset(Zone* zone, XAssetInfo<RawFile>* asset, FileAPI::File* out)
{
    const auto* rawFile = asset->Asset();
    if (rawFile->compressedLen > 0)
    {
        z_stream_s zs{};

        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.avail_in = 0;
        zs.next_in = Z_NULL;

        int ret = inflateInit(&zs);

        if (ret != Z_OK)
        {
            throw std::exception("Initializing inflate failed");
        }

        zs.next_in = reinterpret_cast<const Bytef*>(rawFile->data.compressedBuffer);
        zs.avail_in = rawFile->compressedLen;

        Bytef buffer[0x1000];

        while (zs.avail_in > 0)
        {
            zs.next_out = buffer;
            zs.avail_out = sizeof buffer;
            ret = inflate(&zs, Z_SYNC_FLUSH);

            if (ret < 0)
            {
                printf("Inflate failed for dumping rawfile '%s'\n", rawFile->name);
                inflateEnd(&zs);
                return;
            }

            out->Write(buffer, 1, sizeof buffer - zs.avail_out);
        }

        inflateEnd(&zs);
    }
    else if (rawFile->len > 0)
    {
        out->Write(rawFile->data.buffer, 1, rawFile->len);
    }
}