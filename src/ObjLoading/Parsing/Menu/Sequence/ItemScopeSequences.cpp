#include "ItemScopeSequences.h"

#include <vector>
#include <string>

#include "Generic/GenericColorPropertySequence.h"
#include "Generic/GenericFloatingPointPropertySequence.h"
#include "Generic/GenericIntPropertySequence.h"
#include "Generic/GenericKeywordPropertySequence.h"
#include "Generic/GenericStringPropertySequence.h"
#include "Parsing/Menu/Matcher/MenuMatcherFactory.h"

using namespace menu;

namespace menu::item_scope_sequences
{
    class SequenceCloseBlock final : public MenuFileParser::sequence_t
    {
    public:
        SequenceCloseBlock()
        {
            const MenuMatcherFactory create(this);

            AddMatchers({
                create.Char('}')
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            state->m_current_item = nullptr;
        }
    };

    class SequenceRect final : public MenuFileParser::sequence_t
    {
        static constexpr auto CAPTURE_X = 1;
        static constexpr auto CAPTURE_Y = 2;
        static constexpr auto CAPTURE_W = 3;
        static constexpr auto CAPTURE_H = 4;
        static constexpr auto CAPTURE_ALIGN_HORIZONTAL = 5;
        static constexpr auto CAPTURE_ALIGN_VERTICAL = 6;

    public:
        SequenceRect()
        {
            const MenuMatcherFactory create(this);

            AddMatchers({
                create.KeywordIgnoreCase("rect"),
                create.Numeric().Capture(CAPTURE_X),
                create.Numeric().Capture(CAPTURE_Y),
                create.Numeric().Capture(CAPTURE_W),
                create.Numeric().Capture(CAPTURE_H),
                create.Optional(create.And({
                    create.Integer().Capture(CAPTURE_ALIGN_HORIZONTAL),
                    create.Integer().Capture(CAPTURE_ALIGN_VERTICAL)
                }))
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            assert(state->m_current_item);

            CommonRect rect
            {
                MenuMatcherFactory::TokenNumericFloatingPointValue(result.NextCapture(CAPTURE_X)),
                MenuMatcherFactory::TokenNumericFloatingPointValue(result.NextCapture(CAPTURE_Y)),
                MenuMatcherFactory::TokenNumericFloatingPointValue(result.NextCapture(CAPTURE_W)),
                MenuMatcherFactory::TokenNumericFloatingPointValue(result.NextCapture(CAPTURE_H)),
                0, 0
            };

            if (result.HasNextCapture(CAPTURE_ALIGN_HORIZONTAL) && result.HasNextCapture(CAPTURE_ALIGN_VERTICAL))
            {
                rect.horizontalAlign = result.NextCapture(CAPTURE_ALIGN_HORIZONTAL).IntegerValue();
                rect.verticalAlign = result.NextCapture(CAPTURE_ALIGN_VERTICAL).IntegerValue();
            }

            state->m_current_item->m_rect = rect;
        }
    };

    class SequenceDecodeEffect final : public MenuFileParser::sequence_t
    {
        static constexpr auto CAPTURE_LETTER_TIME = 1;
        static constexpr auto CAPTURE_DECAY_START_TIME = 2;
        static constexpr auto CAPTURE_DECAY_DURATION = 3;

    public:
        SequenceDecodeEffect()
        {
            const MenuMatcherFactory create(this);

            AddMatchers({
                create.KeywordIgnoreCase("decodeEffect"),
                create.Integer().Capture(CAPTURE_LETTER_TIME),
                create.Integer().Capture(CAPTURE_DECAY_START_TIME),
                create.Integer().Capture(CAPTURE_DECAY_DURATION),
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            assert(state->m_current_item);

            state->m_current_item->m_fx_letter_time = result.NextCapture(CAPTURE_LETTER_TIME).IntegerValue();
            state->m_current_item->m_fx_decay_start_time = result.NextCapture(CAPTURE_DECAY_START_TIME).IntegerValue();
            state->m_current_item->m_fx_decay_duration = result.NextCapture(CAPTURE_DECAY_DURATION).IntegerValue();
        }
    };

    class SequenceMultiTokenBlock final : public MenuFileParser::sequence_t
    {
    public:
        using callback_t = std::function<void(MenuFileParserState* state, std::vector<std::string> value)>;

    private:
        static constexpr auto CAPTURE_VALUE = 1;

        callback_t m_set_callback;

    public:
        SequenceMultiTokenBlock(std::string keyName, callback_t setCallback)
            : m_set_callback(std::move(setCallback))
        {
            const MenuMatcherFactory create(this);

            AddMatchers({
                create.KeywordIgnoreCase(std::move(keyName)),
                create.Char('{'),
                create.Optional(create.And({
                    create.Text().Capture(CAPTURE_VALUE),
                    create.OptionalLoop(create.And({
                        create.Char(';'),
                        create.Text().Capture(CAPTURE_VALUE)
                    }))
                })),
                create.Char('}'),
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            if (!m_set_callback)
                return;

            std::vector<std::string> values;
            while (result.HasNextCapture(CAPTURE_VALUE))
            {
                values.emplace_back(MenuMatcherFactory::TokenTextValue(result.NextCapture(CAPTURE_VALUE)));
            }

            m_set_callback(state, std::move(values));
        }
    };
}

using namespace item_scope_sequences;

ItemScopeSequences::ItemScopeSequences(std::vector<std::unique_ptr<MenuFileParser::sequence_t>>& allSequences, std::vector<MenuFileParser::sequence_t*>& scopeSequences)
    : AbstractScopeSequenceHolder(allSequences, scopeSequences)
{
}

void ItemScopeSequences::AddSequences(FeatureLevel featureLevel)
{
    AddSequence(std::make_unique<SequenceCloseBlock>());
    AddSequence(std::make_unique<GenericStringPropertySequence>("name", [](const MenuFileParserState* state, const std::string& value)
    {
        state->m_current_item->m_name = value;
    }));
    AddSequence(std::make_unique<GenericStringPropertySequence>("text", [](const MenuFileParserState* state, const std::string& value)
    {
        state->m_current_item->m_text = value;
    }));
    AddSequence(std::make_unique<GenericKeywordPropertySequence>("textsavegame", [](const MenuFileParserState* state)
    {
        state->m_current_item->m_text_save_game = true;
    }));
    AddSequence(std::make_unique<GenericKeywordPropertySequence>("textcinematicsubtitle", [](const MenuFileParserState* state)
    {
        state->m_current_item->m_text_cinematic_subtitle = true;
    }));
    AddSequence(std::make_unique<GenericStringPropertySequence>("group", [](const MenuFileParserState* state, const std::string& value)
    {
        state->m_current_item->m_group = value;
    }));
    AddSequence(std::make_unique<SequenceRect>());
    AddSequence(std::make_unique<GenericIntPropertySequence>("style", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_style = value;
    }));
    AddSequence(std::make_unique<GenericKeywordPropertySequence>("decoration", [](const MenuFileParserState* state)
    {
        state->m_current_item->m_decoration = true;
    }));
    AddSequence(std::make_unique<GenericKeywordPropertySequence>("autowrapped", [](const MenuFileParserState* state)
    {
        state->m_current_item->m_auto_wrapped = true;
    }));
    AddSequence(std::make_unique<GenericKeywordPropertySequence>("horizontalscroll", [](const MenuFileParserState* state)
    {
        state->m_current_item->m_horizontal_scroll = true;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("type", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_type = value;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("border", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_border = value;
    }));
    AddSequence(std::make_unique<GenericFloatingPointPropertySequence>("borderSize", [](const MenuFileParserState* state, const double value)
    {
        state->m_current_item->m_border_size = value;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("ownerdraw", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_owner_draw = value;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("align", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_align = value;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("textalign", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_text_align = value;
    }));
    AddSequence(std::make_unique<GenericFloatingPointPropertySequence>("textalignx", [](const MenuFileParserState* state, const double value)
    {
        state->m_current_item->m_text_align_x = value;
    }));
    AddSequence(std::make_unique<GenericFloatingPointPropertySequence>("textaligny", [](const MenuFileParserState* state, const double value)
    {
        state->m_current_item->m_text_align_y = value;
    }));
    AddSequence(std::make_unique<GenericFloatingPointPropertySequence>("textscale", [](const MenuFileParserState* state, const double value)
    {
        state->m_current_item->m_text_scale = value;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("textstyle", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_text_style = value;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("textfont", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_text_font = value;
    }));
    AddSequence(std::make_unique<GenericColorPropertySequence>("backcolor", [](const MenuFileParserState* state, const CommonColor value)
    {
        state->m_current_item->m_back_color = value;
    }));
    AddSequence(std::make_unique<GenericColorPropertySequence>("forecolor", [](const MenuFileParserState* state, const CommonColor value)
    {
        state->m_current_item->m_fore_color = value;
    }));
    AddSequence(std::make_unique<GenericColorPropertySequence>("bordercolor", [](const MenuFileParserState* state, const CommonColor value)
    {
        state->m_current_item->m_border_color = value;
    }));
    AddSequence(std::make_unique<GenericColorPropertySequence>("outlinecolor", [](const MenuFileParserState* state, const CommonColor value)
    {
        state->m_current_item->m_outline_color = value;
    }));
    AddSequence(std::make_unique<GenericColorPropertySequence>("disablecolor", [](const MenuFileParserState* state, const CommonColor value)
    {
        state->m_current_item->m_disable_color = value;
    }));
    AddSequence(std::make_unique<GenericColorPropertySequence>("glowcolor", [](const MenuFileParserState* state, const CommonColor value)
    {
        state->m_current_item->m_glow_color = value;
    }));
    AddSequence(std::make_unique<GenericStringPropertySequence>("background", [](const MenuFileParserState* state, const std::string& value)
    {
        state->m_current_item->m_background = value;
    }));
    AddSequence(std::make_unique<GenericStringPropertySequence>("focusSound", [](const MenuFileParserState* state, const std::string& value)
    {
        state->m_current_item->m_focus_sound = value;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("ownerdrawFlag", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_owner_draw_flags |= value;
    }));
    AddSequence(std::make_unique<GenericStringPropertySequence>("dvarTest", [](const MenuFileParserState* state, const std::string& value)
    {
        state->m_current_item->m_dvar_test = value;
    }));
    AddSequence(std::make_unique<SequenceMultiTokenBlock>("enableDvar", [](const MenuFileParserState* state, std::vector<std::string> value)
    {
        state->m_current_item->m_enable_dvar = std::move(value);
    }));
    AddSequence(std::make_unique<SequenceMultiTokenBlock>("disableDvar", [](const MenuFileParserState* state, std::vector<std::string> value)
    {
        state->m_current_item->m_disable_dvar = std::move(value);
    }));
    AddSequence(std::make_unique<SequenceMultiTokenBlock>("showDvar", [](const MenuFileParserState* state, std::vector<std::string> value)
    {
        state->m_current_item->m_show_dvar = std::move(value);
    }));
    AddSequence(std::make_unique<SequenceMultiTokenBlock>("hideDvar", [](const MenuFileParserState* state, std::vector<std::string> value)
    {
        state->m_current_item->m_hide_dvar = std::move(value);
    }));
    AddSequence(std::make_unique<SequenceMultiTokenBlock>("focusDvar", [](const MenuFileParserState* state, std::vector<std::string> value)
    {
        state->m_current_item->m_focus_dvar = std::move(value);
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("gamemsgwindowindex", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_game_message_window_index = value;
    }));
    AddSequence(std::make_unique<GenericIntPropertySequence>("gamemsgwindowmode", [](const MenuFileParserState* state, const int value)
    {
        state->m_current_item->m_game_message_window_mode = value;
    }));
    AddSequence(std::make_unique<SequenceDecodeEffect>());
}