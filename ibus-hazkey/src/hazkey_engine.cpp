#include "hazkey_engine.h"

#include <optional>
#include <string>
#include <vector>

#include "hazkey/frontend/frontend_hooks.h"
#include "hazkey/frontend/key_event.h"
#include "hazkey/frontend/output.h"
#include "hazkey/frontend/server_client.h"
#include "hazkey/frontend/state_machine.h"

namespace {

namespace hf = hazkey::frontend;

hf::ServerClient* g_server = nullptr;

// ibus has a single auxiliary text; the lower aux text of fcitx5 is appended
// to the upper one. Same wording as the Japanese translation of fcitx5-hazkey.
const char* auxDownText(hf::AuxDown aux) {
    switch (aux) {
        case hf::AuxDown::TabToSelect:
            return "[Tabキーで選択]";
        case hf::AuxDown::DirectInput:
            return "[直接入力]";
        case hf::AuxDown::None:
        default:
            return "";
    }
}

// Explicit underline and colors rather than the preedit hints of ibus
// 1.5.33: the Qt ibus plugin knows only underline, foreground and background
// and drops hints silently, leaving the preedit undecorated.
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void appendStyle(IBusAttrList* attrs, hf::SegmentStyle style, guint start,
                 guint end) {
    switch (style) {
        case hf::SegmentStyle::Underline:
            ibus_attr_list_append(
                attrs,
                ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, start, end));
            break;
        case hf::SegmentStyle::Highlight:
            ibus_attr_list_append(
                attrs,
                ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, start, end));
            ibus_attr_list_append(attrs,
                                  ibus_attr_foreground_new(0xffffff, start, end));
            ibus_attr_list_append(attrs,
                                  ibus_attr_background_new(0x3465a4, start, end));
            break;
        case hf::SegmentStyle::Normal:
            break;
    }
}
G_GNUC_END_IGNORE_DEPRECATIONS

// segments joined into one IBusText with attributes. caret is set to the
// start of caretSegment, or the end if it is -1.
IBusText* toIbusText(const std::vector<hf::Segment>& segments,
                     int caretSegment, guint* caret) {
    std::string all;
    IBusAttrList* attrs = ibus_attr_list_new();
    guint pos = 0;
    *caret = G_MAXUINT;
    for (int i = 0; static_cast<size_t>(i) < segments.size(); i++) {
        const auto& segment = segments[i];
        if (i == caretSegment) *caret = pos;
        auto length = static_cast<guint>(g_utf8_strlen(segment.text.c_str(), -1));
        appendStyle(attrs, segment.style, pos, pos + length);
        pos += length;
        all += segment.text;
    }
    if (*caret == G_MAXUINT) *caret = pos;
    IBusText* text = ibus_text_new_from_string(all.c_str());
    ibus_text_set_attributes(text, attrs);
    return text;
}

// ibus keyval is an X11 keysym; the modifiers come in the state mask.
hf::KeyEvent toCoreKeyEvent(guint keyval, guint state) {
    hf::KeyEvent event;
    event.sym = keyval;
    if (state & IBUS_SHIFT_MASK) event.mods |= hf::mod::Shift;
    if (state & IBUS_CONTROL_MASK) event.mods |= hf::mod::Ctrl;
    if (state & IBUS_MOD1_MASK) event.mods |= hf::mod::Alt;
    event.isRelease = (state & IBUS_RELEASE_MASK) != 0;
    if (!(state & (IBUS_CONTROL_MASK | IBUS_MOD1_MASK))) {
        // printable characters, including the kana keysyms
        gunichar uc = ibus_keyval_to_unicode(keyval);
        if (uc >= 0x20 && uc != 0x7f) {
            char buf[6];
            int n = g_unichar_to_utf8(uc, buf);
            event.text.assign(buf, n);
        }
    }
    return event;
}

// State of one input context: the core state machine and what it needs
// from ibus.
class IbusState : public hf::FrontendHooks {
   public:
    IbusState(IBusEngine* engine, hf::ServerApi& server)
        : engine_(engine), core_(server, *this) {}

    gboolean keyEvent(guint keyval, guint state) {
        auto output = core_.keyEvent(toCoreKeyEvent(keyval, state));
        if (output.result == hf::KeyResult::Ignored) {
            // key releases and a lone Shift press only update the lower aux
            // text, as in fcitx5-hazkey
            updateAux(output.auxDown);
        } else {
            apply(output);
        }
        return output.result == hf::KeyResult::Consumed;
    }

    // Discard the composition. The client has committed the preedit by
    // itself (COMMIT mode) when the focus has gone or on reset.
    void reset() { apply(core_.reset()); }

    // FrontendHooks
    std::optional<hf::SurroundingText> surroundingText() override {
        if (!(engine_->client_capabilities & IBUS_CAP_SURROUNDING_TEXT)) {
            return std::nullopt;
        }
        IBusText* text = nullptr;
        guint cursor = 0;
        guint anchor = 0;
        ibus_engine_get_surrounding_text(engine_, &text, &cursor, &anchor);
        if (text == nullptr) return std::nullopt;
        // the anchor, as fcitx5-hazkey passes
        return hf::SurroundingText{ibus_text_get_text(text),
                                   static_cast<int>(anchor)};
    }
    bool showTabToSelect() override { return true; }

   private:
    void apply(const hf::Output& output) {
        if (!output.commit.empty()) {
            ibus_engine_commit_text(
                engine_, ibus_text_new_from_string(output.commit.c_str()));
        }

        guint caret = 0;
        auto segments = output.preedit.displaySegments();
        // without a caret given, it goes before the furigana
        int caretSegment = output.preedit.caretSegment >= 0
                               ? output.preedit.caretSegment
                               : static_cast<int>(output.preedit.segments.size());
        IBusText* preedit = toIbusText(segments, caretSegment, &caret);
        // ibus detaches the engine from an input context before telling it
        // the focus has gone, so the engine cannot commit then. in COMMIT
        // mode the client commits the preedit by itself on focus out and
        // reset. ibus cannot leave a part of the preedit out of the commit,
        // so the furigana is committed too; the user can turn it off.
        ibus_engine_update_preedit_text_with_mode(
            engine_, preedit, caret, !segments.empty(),
            IBUS_ENGINE_PREEDIT_COMMIT);

        auxUp_ = output.auxUp;
        updateAux(output.auxDown);

        updateCandidates(output.candidates);
    }

    void updateAux(hf::AuxDown auxDown) {
        std::string up;
        for (const auto& segment : auxUp_) up += segment.text;
        std::string text;
        for (const std::string& part :
             {up, std::string(auxDownText(auxDown))}) {
            if (part.empty()) continue;
            if (!text.empty()) text += " ";
            text += part;
        }
        ibus_engine_update_auxiliary_text(
            engine_, ibus_text_new_from_string(text.c_str()), !text.empty());
    }

    void updateCandidates(const hf::CandidateWindow& window) {
        if (!window.visible || window.items.empty() || window.pageSize <= 0) {
            ibus_engine_hide_lookup_table(engine_);
            return;
        }
        // the page shown is the one holding the cursor. an unfocused list
        // stays on the first page.
        IBusLookupTable* table = ibus_lookup_table_new(
            window.pageSize, window.focused() ? window.cursor : 0,
            window.focused(), FALSE);
        ibus_lookup_table_set_orientation(table, IBUS_ORIENTATION_VERTICAL);
        static const char* kLabels[] = {"1", "2", "3", "4", "5",
                                        "6", "7", "8", "9", "0"};
        for (int i = 0; i < window.pageSize && i < 10; i++) {
            ibus_lookup_table_set_label(table, i,
                                        ibus_text_new_from_string(kLabels[i]));
        }
        for (const auto& candidate : window.items) {
            ibus_lookup_table_append_candidate(
                table, ibus_text_new_from_string(candidate.text.c_str()));
        }
        ibus_engine_update_lookup_table(engine_, table, TRUE);
    }

    IBusEngine* engine_;
    hf::StateMachine core_;
    std::vector<hf::Segment> auxUp_;
};

}  // namespace

struct HazkeyEngine {
    IBusEngine parent;
    IbusState* state;
};

struct HazkeyEngineClass {
    IBusEngineClass parent;
};

G_DEFINE_TYPE(HazkeyEngine, hazkey_engine, IBUS_TYPE_ENGINE)

void hazkey_engine_set_server(hazkey::frontend::ServerClient* server) {
    g_server = server;
}

namespace {

IbusState* stateOf(IBusEngine* engine) {
    return reinterpret_cast<HazkeyEngine*>(engine)->state;
}

gboolean processKeyEvent(IBusEngine* engine, guint keyval,
                         [[maybe_unused]] guint keycode, guint state) {
    return stateOf(engine)->keyEvent(keyval, state);
}

void focusOut(IBusEngine* engine) {
    g_debug("focus_out");
    stateOf(engine)->reset();
}

void reset(IBusEngine* engine) {
    g_debug("reset");
    stateOf(engine)->reset();
}

void enable(IBusEngine* engine) {
    g_debug("enable");
    // tells the input context that this engine uses the surrounding text
    ibus_engine_get_surrounding_text(engine, nullptr, nullptr, nullptr);
    stateOf(engine)->reset();
}

void disable(IBusEngine* engine) {
    g_debug("disable");
    stateOf(engine)->reset();
}

void destroy(IBusObject* object) {
    auto* self = reinterpret_cast<HazkeyEngine*>(object);
    delete self->state;
    self->state = nullptr;
    IBUS_OBJECT_CLASS(hazkey_engine_parent_class)->destroy(object);
}

}  // namespace

static void hazkey_engine_class_init(HazkeyEngineClass* klass) {
    auto* objectClass = IBUS_OBJECT_CLASS(klass);
    objectClass->destroy = destroy;

    auto* engineClass = IBUS_ENGINE_CLASS(klass);
    engineClass->process_key_event = processKeyEvent;
    engineClass->focus_out = focusOut;
    engineClass->reset = reset;
    engineClass->enable = enable;
    engineClass->disable = disable;
}

static void hazkey_engine_init(HazkeyEngine* self) {
    self->state = new IbusState(IBUS_ENGINE(self), *g_server);
}
