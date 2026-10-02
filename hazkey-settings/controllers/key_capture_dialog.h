#ifndef HAZKEY_SETTINGS_CONTROLLERS_KEY_CAPTURE_DIALOG_H_
#define HAZKEY_SETTINGS_CONTROLLERS_KEY_CAPTURE_DIALOG_H_

#include <QDialog>
#include <optional>
#include <string>

class QLabel;

namespace hazkey::settings {

// Asks the user to press a key and reports it in the syntax of the key
// bindings ("Control+Shift+h"). Keys the bindings cannot express (Meta,
// keys without a name in hazkey-frontend-core) are refused with a message
// and the dialog keeps waiting.
class KeyCaptureDialog : public QDialog {
    Q_OBJECT

   public:
    explicit KeyCaptureDialog(QWidget* parent);

    // the key pressed, if the dialog was accepted
    std::optional<std::string> key() const { return key_; }

   protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    QLabel* message_;
    std::optional<std::string> key_;
};

}  // namespace hazkey::settings

#endif  // HAZKEY_SETTINGS_CONTROLLERS_KEY_CAPTURE_DIALOG_H_
