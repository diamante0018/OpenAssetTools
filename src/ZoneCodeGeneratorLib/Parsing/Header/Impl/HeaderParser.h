#pragma once

#include "HeaderLexer.h"
#include "HeaderParserState.h"
#include "Parsing/IPackValueSupplier.h"
#include "Parsing/Impl/AbstractParser.h"
#include "Persistence/IDataRepository.h"

class HeaderParser final : public AbstractParser<HeaderParserValue, HeaderParserState>
{
public:
    HeaderParser(HeaderLexer* lexer, const IPackValueSupplier* packValueSupplier);

    bool SaveToRepository(IDataRepository* repository) const;

protected:
    const std::vector<sequence_t*>& GetTestsForState() override;
};
