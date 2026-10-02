#ifndef HAZKEY_SETTINGS_CONTROLLERS_KEY_BINDINGS_TAB_CONTROLLER_H_
#define HAZKEY_SETTINGS_CONTROLLERS_KEY_BINDINGS_TAB_CONTROLLER_H_

#include <QObject>
#include <string>
#include <vector>

#include "config.pb.h"
#include "controllers/tab_context.h"

class QTreeWidgetItem;
class QWidget;

namespace Ui {
class MainWindow;
}

namespace hazkey::settings {

// The "Key Bindings" tab: a table of the operations of
// hazkey-frontend-core and their keys. Keys are added by pressing them and
// removed with a button. Only the operations whose keys differ from the
// defaults are written to the profile, so the config keeps its meaning
// (operations not listed use the defaults of the clients).
class KeyBindingsTabController : public QObject {
    Q_OBJECT

   public:
    KeyBindingsTabController(Ui::MainWindow* ui, QWidget* window,
                             QObject* parent);
    void setContext(const TabContext& context);
    void connectSignals();
    void loadFromConfig();
    void saveToConfig();

   private slots:
    void onAddKey();
    void onRemoveKey();
    void onResetAction();
    void onResetAll();
    void onSelectionChanged();

   private:
    Ui::MainWindow* ui_;
    QWidget* window_;
    TabContext context_;

    // keys of each action, indexed like actionTable(). Readable keys are in
    // the canonical text; unreadable ones are kept as written
    std::vector<std::vector<std::string>> keys_;
    // the defaults in the canonical text, indexed like actionTable()
    std::vector<std::vector<std::string>> defaults_;
    // entries of the profile for actions this build does not know, kept
    // and written back unchanged
    std::vector<hazkey::config::KeyBinding> unknown_;
    // the row of each action, indexed like actionTable()
    std::vector<QTreeWidgetItem*> rows_;

    void buildTree();
    void refreshRow(size_t index);
    void refreshAll();
    void refreshWarnings();
    void updateButtons();
    // index into actionTable() of the selected row, or -1
    int selectedAction() const;
    bool isDefault(size_t index) const;
};

}  // namespace hazkey::settings

#endif  // HAZKEY_SETTINGS_CONTROLLERS_KEY_BINDINGS_TAB_CONTROLLER_H_
