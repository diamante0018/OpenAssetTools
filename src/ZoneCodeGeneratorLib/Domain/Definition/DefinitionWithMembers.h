#pragma once

#include <memory>
#include <vector>

#include "DataDefinition.h"
#include "Variable.h"

class DefinitionWithMembers : public DataDefinition
{
public:
    static constexpr int FLAG_SIZE_CALCULATED = 1 << 0;
    static constexpr int FLAG_ALIGNMENT_CALCULATED = 1 << 1;
    static constexpr int FLAG_ALIGNMENT_FORCED = 1 << 2;

    unsigned m_flags;
    unsigned m_size;
    unsigned m_alignment;

    /*virtual void CalculateSize() = 0;
    void CalculateAlignment();*/

    bool m_has_alignment_override;
    bool m_anonymous;

    const unsigned m_pack;
    unsigned m_alignment_override;

    std::vector<std::unique_ptr<Variable>> m_members;

    DefinitionWithMembers(std::string _namespace, std::string name, unsigned pack);

    _NODISCARD unsigned GetAlignment() const override;
    _NODISCARD bool GetForceAlignment() const override;
    _NODISCARD unsigned GetSize() const override;
};