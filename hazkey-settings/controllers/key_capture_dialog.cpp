#include "controllers/key_capture_dialog.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "hazkey/frontend/keybindings.h"
#include "hazkey/frontend/keysyms.h"

namespace hazkey::settings {

namespace {

namespace ks = hazkey::frontend::keysym;

struct QtKeyName {
    int qtKey;
    uint32_t sym;
};

// Qt keys of the named keys hazkey-frontend-core knows. Printable ASCII is
// handled by range.
constexpr QtKeyName kQtKeys[] = {
    {Qt::Key_Space, ks::Space},
    {Qt::Key_Backspace, ks::BackSpace},
    {Qt::Key_Tab, ks::Tab},
    {Qt::Key_Return, ks::Return},
    {Qt::Key_Enter, ks::Return},
    {Qt::Key_Escape, ks::Escape},
    {Qt::Key_Delete, ks::Delete},
    {Qt::Key_Insert, ks::Insert},
    {Qt::Key_Home, ks::Home},
    {Qt::Key_End, ks::End},
    {Qt::Key_PageUp, ks::Page_Up},
    {Qt::Key_PageDown, ks::Page_Down},
    {Qt::Key_Left, ks::Left},
    {Qt::Key_Up, ks::Up},
    {Qt::Key_Right, ks::Right},
    {Qt::Key_Down, ks::Down},
    {Qt::Key_Muhenkan, ks::Muhenkan},
    {Qt::Key_Henkan, ks::Henkan},
    {Qt::Key_Hiragana_Katakana, ks::Hiragana_Katakana},
    {Qt::Key_Zenkaku_Hankaku, ks::Zenkaku_Hankaku},
    {Qt::Key_Eisu_toggle, ks::Eisu_toggle},
    {Qt::Key_F1, ks::F1},
    {Qt::Key_F2, ks::F2},
    {Qt::Key_F3, ks::F3},
    {Qt::Key_F4, ks::F4},
    {Qt::Key_F5, ks::F5},
    {Qt::Key_F6, ks::F6},
    {Qt::Key_F7, ks::F7},
    {Qt::Key_F8, ks::F8},
    {Qt::Key_F9, ks::F9},
    {Qt::Key_F10, ks::F10},
    {Qt::Key_F11, ks::F11},
    {Qt::Key_F12, ks::F12},
};

bool isModifierKey(int key) {
    switch (key) {
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
        case Qt::Key_Meta:
        case Qt::Key_Super_L:
        case Qt::Key_Super_R:
        case Qt::Key_Hyper_L:
        case Qt::Key_Hyper_R:
        case Qt::Key_CapsLock:
        case Qt::Key_NumLock:
        case Qt::Key_ScrollLock:
        case Qt::Key_unknown:
            return true;
        default:
            return false;
    }
}

// The key of the event in the syntax of the bindings, or nullopt when the
// bindings cannot express it.
std::optional<std::string> keyText(const QKeyEvent* event) {
    using namespace hazkey::frontend;

    KeyChord chord;
    int key = event->key();
    Qt::KeyboardModifiers mods = event->modifiers();

    if (key == Qt::Key_Backtab) {
        // Shift+Tab arrives as Backtab
        key = Qt::Key_Tab;
        mods |= Qt::ShiftModifier;
    }

    if (mods & Qt::MetaModifier) return std::nullopt;
    if (mods & Qt::ControlModifier) chord.mods |= mod::Ctrl;
    if (mods & Qt::ShiftModifier) chord.mods |= mod::Shift;
    if (mods & Qt::AltModifier) chord.mods |= mod::Alt;

    for (const auto& entry : kQtKeys) {
        if (entry.qtKey == key) {
            chord.sym = entry.sym;
            break;
        }
    }
    if (chord.sym == 0) {
        // Qt::Key_Exclam .. Qt::Key_AsciiTilde are the ASCII codes, as are
        // the X11 keysyms
        if (key > 0x20 && key < 0x7f) {
            chord.sym = static_cast<uint32_t>(key);
        } else {
            return std::nullopt;
        }
    }

    auto text = formatKeyChord(chord);
    // keep the text in the form the bindings read back
    if (text.has_value() && !parseKeyChord(*text).has_value()) {
        return std::nullopt;
    }
    return text;
}

}  // namespace

KeyCaptureDialog::KeyCaptureDialog(QWidget* parent)
    : QDialog(parent), message_(new QLabel(this)) {
    setWindowTitle(QCoreApplication::translate("KeyCaptureDialog", "Add Key"));
    setModal(true);
    // keys go to this dialog, not to the input method
    setAttribute(Qt::WA_InputMethodEnabled, false);
    setFocusPolicy(Qt::StrongFocus);

    auto* layout = new QVBoxLayout(this);
    auto* prompt =
        new QLabel(QCoreApplication::translate(
                       "KeyCaptureDialog",
                       "Press the key to assign.\n"
                       "Modifiers (Control, Shift, Alt) are taken as held."),
                   this);
    prompt->setWordWrap(true);
    layout->addWidget(prompt);

    message_->setWordWrap(true);
    message_->setMinimumHeight(message_->fontMetrics().height() * 2);
    layout->addWidget(message_);

    auto* cancel = new QPushButton(
        QCoreApplication::translate("KeyCaptureDialog", "Cancel"), this);
    // Escape and Return are keys to assign, so no button takes them
    cancel->setFocusPolicy(Qt::NoFocus);
    cancel->setAutoDefault(false);
    cancel->setDefault(false);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(cancel, 0, Qt::AlignRight);

    setMinimumWidth(320);
}

void KeyCaptureDialog::keyPressEvent(QKeyEvent* event) {
    event->accept();
    if (isModifierKey(event->key())) return;
    if (event->isAutoRepeat()) return;

    auto text = keyText(event);
    if (!text.has_value()) {
        message_->setText(QCoreApplication::translate(
            "KeyCaptureDialog", "This key cannot be assigned."));
        return;
    }
    key_ = text;
    accept();
}

void KeyCaptureDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    message_->clear();
    key_.reset();
    setFocus();
    grabKeyboard();
}

void KeyCaptureDialog::hideEvent(QHideEvent* event) {
    releaseKeyboard();
    QDialog::hideEvent(event);
}

}  // namespace hazkey::settings
