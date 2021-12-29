#pragma once

#include <map>

#include "AssetLoading/IZoneAssetLoaderState.h"
#include "Game/IW4/IW4.h"

namespace IW4
{
    class MenuConversionZoneState final : public IZoneAssetLoaderState
    {
        Zone* m_zone;
        std::vector<Statement_s*> m_functions;
        std::map<std::string, Statement_s*> m_function_by_name;

        std::vector<StaticDvar*> m_static_dvars;
        std::map<std::string, size_t> m_dvars_by_name;

        std::vector<const char*> m_strings;
        std::map<std::string, const char*> m_strings_by_value;

    public:
        ExpressionSupportingData* m_supporting_data;

        MenuConversionZoneState();
        void SetZone(Zone* zone) override;

        Statement_s* FindFunction(const std::string& functionName);

        Statement_s* AddFunction(const std::string& functionName, Statement_s* function);
        size_t AddStaticDvar(const std::string& dvarName);
        const char* AddString(const std::string& str);

        void FinalizeSupportingData() const;
    };
}