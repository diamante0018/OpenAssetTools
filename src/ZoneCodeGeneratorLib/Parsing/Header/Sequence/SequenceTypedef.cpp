#include "SequenceTypedef.h"

#include "Parsing/Header/Matcher/HeaderMatcherFactory.h"
#include "Parsing/Header/Matcher/HeaderCommonMatchers.h"

SequenceTypedef::SequenceTypedef()
{
    const HeaderMatcherFactory create(this);

    AddLabeledMatchers(HeaderCommonMatchers::Align(this), HeaderCommonMatchers::LABEL_ALIGN);
    AddLabeledMatchers(HeaderCommonMatchers::ArrayDef(this), HeaderCommonMatchers::LABEL_ARRAY_DEF);
    AddLabeledMatchers(HeaderCommonMatchers::Typename(this), HeaderCommonMatchers::LABEL_TYPENAME);

    AddLabeledMatchers(
        {
            create.Optional(create.Type(HeaderParserValueType::CONST).Tag(TAG_CONST)),
            create.Label(HeaderCommonMatchers::LABEL_TYPENAME).Capture(CAPTURE_TYPE),
            create.OptionalLoop(create.Char('*').Tag(TAG_POINTER)),
            create.Identifier().Capture(CAPTURE_NAME),
            create.OptionalLoop(create.Label(HeaderCommonMatchers::LABEL_ARRAY_DEF).Capture(CAPTURE_ARRAY)),
            create.Char(';')
        }, LABEL_ARRAY_OF_POINTERS);

    AddLabeledMatchers(
        {
            create.Optional(create.Type(HeaderParserValueType::CONST).Tag(TAG_CONST)),
            create.Label(HeaderCommonMatchers::LABEL_TYPENAME).Capture(CAPTURE_TYPE),
            create.Char('('),
            create.Loop(create.Char('*').Tag(TAG_POINTER)),
            create.Identifier().Capture(CAPTURE_NAME),
            create.Char(')'),
            create.Loop(create.Label(HeaderCommonMatchers::LABEL_ARRAY_DEF).Capture(CAPTURE_ARRAY)),
            create.Char(';')
        }, LABEL_POINTER_TO_ARRAY);

    AddMatchers({
        create.Type(HeaderParserValueType::TYPEDEF),
        create.Optional(create.Label(HeaderCommonMatchers::LABEL_ALIGN).Capture(CAPTURE_ALIGN)),
        create.Or({
            create.Label(LABEL_ARRAY_OF_POINTERS).Tag(TAG_ARRAY_OF_POINTERS),
            create.Label(LABEL_POINTER_TO_ARRAY).Tag(TAG_POINTER_TO_ARRAY)
        })
    });
}

void SequenceTypedef::ProcessMatch(HeaderParserState* state, SequenceResult<HeaderParserValue>& result) const
{
}