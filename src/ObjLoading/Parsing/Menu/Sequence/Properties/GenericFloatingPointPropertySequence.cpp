#include "GenericFloatingPointPropertySequence.h"

#include <utility>

#include "Parsing/Menu/MenuMatcherFactory.h"

using namespace menu;

GenericFloatingPointPropertySequence::GenericFloatingPointPropertySequence(std::string keywordName, callback_t setCallback)
    : m_set_callback(std::move(setCallback))
{
    const MenuMatcherFactory create(this);

    AddMatchers({
        create.KeywordIgnoreCase(std::move(keywordName)),
        create.Numeric().Capture(CAPTURE_VALUE)
    });
}

void GenericFloatingPointPropertySequence::ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const
{
    if (m_set_callback)
    {
        const auto value = MenuMatcherFactory::TokenNumericFloatingPointValue(result.NextCapture(CAPTURE_VALUE));
        m_set_callback(state, value);
    }
}