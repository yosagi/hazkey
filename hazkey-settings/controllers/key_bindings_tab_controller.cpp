#include "controllers/key_bindings_tab_controller.h"

#include <QCoreApplication>
#include <QFont>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <algorithm>
#include <map>

#include "controllers/key_capture_dialog.h"
#include "hazkey/frontend/keybindings.h"
#include "ui_mainwindow.h"

namespace hazkey::settings {

namespace {

using hazkey::frontend::Action;
using hazkey::frontend::ActionInfo;
using hazkey::frontend::actionTable;
using hazkey::frontend::formatKeyChord;
using hazkey::frontend::KeyChord;
using hazkey::frontend::KeyContext;
using hazkey::frontend::parseKeyChord;

// Translations are looked up with this context spelled out, because
// lupdate records the class name without the namespace while tr() of a
// namespaced class looks up the qualified name and finds nothing.
constexpr const char* kTrContext = "KeyBindingsTabController";

QString trk(const char* text) {
    return QCoreApplication::translate(kTrContext, text);
}

// the index of each action in actionTable()
constexpr int kActionIndexRole = Qt::UserRole;

struct ActionName {
    const char* id;
    const char* name;
    const char* description;
};

// Display names of the actions of hazkey-frontend-core. An action missing
// here is shown by its id.
constexpr ActionName kActionNames[] = {
    {"composing.commit",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Commit"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Commit the text.")},
    {"composing.commit_prefix",
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Commit before the focused clause"),
     QT_TRANSLATE_NOOP(
         "KeyBindingsTabController",
         "Commit only the part before the focused (highlighted) clause.")},
    {"composing.shelve_trailing_clause",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Shelve the focused clause"),
     QT_TRANSLATE_NOOP(
         "KeyBindingsTabController",
         "Remove the focused clause and keep its reading. Committing brings it "
         "back; typing or cancelling discards it.")},
    {"composing.delete_left",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Delete a character"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Delete the character before the cursor.")},
    {"composing.delete_right",
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Delete the character to the right"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Delete the character after the cursor.")},
    {"composing.cancel",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Cancel"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Discard the text being composed.")},
    {"composing.convert",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Convert"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Convert and show the candidates.")},
    {"composing.insert_space",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Insert a space"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Insert a space character.")},
    {"composing.focus_candidates",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Focus the candidates"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move into the candidate list.")},
    {"composing.cursor_left",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Move the cursor left"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move the cursor to the left.")},
    {"composing.cursor_right",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Move the cursor right"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move the cursor to the right.")},
    {"composing.cursor_home",
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move the cursor to the start"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move the cursor to the start of the text.")},
    {"composing.cursor_end",
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move the cursor to the end"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move the cursor to the end of the text.")},
    {"composing.toggle_reading_view",
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Show the reading / the conversion"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Switch the text between the reading as typed, where "
                       "the cursor moves by characters, and the automatic "
                       "conversion, where it moves by elements.")},

    {"candidate.next",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Next candidate"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Select the next candidate.")},
    {"candidate.prev",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Previous candidate"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Select the previous candidate.")},
    {"candidate.next_page",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Next page"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Show the next page of candidates.")},
    {"candidate.prev_page",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Previous page"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Show the previous page of candidates.")},
    {"candidate.expand_segment",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Extend the clause"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move the clause boundary to the right.")},
    {"candidate.shrink_segment",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Shorten the clause"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Move the clause boundary to the left.")},
    {"candidate.commit",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Commit the candidate"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Commit the selected candidate.")},
    {"candidate.cancel",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Back to composing"),
     QT_TRANSLATE_NOOP(
         "KeyBindingsTabController",
         "Return to composing (while adjusting a clause boundary, undo the "
         "adjustment).")},
    {"candidate.back",
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Back to composing (delete)"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Return to composing.")},

    {"convert_to.hiragana",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "To hiragana"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Convert the text to hiragana.")},
    {"convert_to.katakana_full",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "To full-width katakana"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Convert the text to full-width katakana.")},
    {"convert_to.katakana_half",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "To half-width katakana"),
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "Convert the text to half-width katakana.")},
    {"convert_to.alphanumeric_full",
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "To full-width alphanumeric"),
     QT_TRANSLATE_NOOP(
         "KeyBindingsTabController",
         "Convert the text to full-width alphanumeric characters.")},
    {"convert_to.alphanumeric_half",
     QT_TRANSLATE_NOOP("KeyBindingsTabController",
                       "To half-width alphanumeric"),
     QT_TRANSLATE_NOOP(
         "KeyBindingsTabController",
         "Convert the text to half-width alphanumeric characters.")},

    {"idle.restore_reading",
     QT_TRANSLATE_NOOP("KeyBindingsTabController", "Restore reading"),
     QT_TRANSLATE_NOOP(
         "KeyBindingsTabController",
         "Bring back the reading of a recent composition. Pressing again goes "
         "back one more. While nothing is composed, this key does not reach "
         "the application.")},
};

const ActionName* findActionName(const char* id) {
    for (const auto& entry : kActionNames) {
        if (std::string(entry.id) == id) return &entry;
    }
    return nullptr;
}

QString contextName(KeyContext context) {
    switch (context) {
        case KeyContext::Composing:
            return QCoreApplication::translate("KeyBindingsTabController",
                                               "While composing");
        case KeyContext::Candidate:
            return QCoreApplication::translate("KeyBindingsTabController",
                                               "While choosing a candidate");
        case KeyContext::Any:
            return QCoreApplication::translate(
                "KeyBindingsTabController",
                "While composing or choosing a candidate");
        case KeyContext::Idle:
            return QCoreApplication::translate("KeyBindingsTabController",
                                               "While nothing is composed");
    }
    return QString();
}

// true when a key bound to both contexts is looked up for both
bool contextsOverlap(KeyContext a, KeyContext b) {
    if (a == b) return true;
    if (a == KeyContext::Idle || b == KeyContext::Idle) return false;
    return a == KeyContext::Any || b == KeyContext::Any;
}

QString joinKeys(const std::vector<std::string>& keys) {
    QStringList parts;
    for (const auto& key : keys) parts << QString::fromStdString(key);
    return parts.join(", ");
}

// canonical text if the key is readable, otherwise the text as written
std::string canonicalKey(const std::string& key) {
    auto chord = parseKeyChord(key);
    if (!chord.has_value()) return key;
    auto text = formatKeyChord(*chord);
    return text.has_value() ? *text : key;
}

bool sameKeySet(std::vector<std::string> a, std::vector<std::string> b) {
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    return a == b;
}

}  // namespace

KeyBindingsTabController::KeyBindingsTabController(Ui::MainWindow* ui,
                                                   QWidget* window,
                                                   QObject* parent)
    : QObject(parent), ui_(ui), window_(window) {
    const auto& table = actionTable();
    defaults_.resize(table.size());
    for (size_t i = 0; i < table.size(); i++) {
        for (const char* key : table[i].defaultKeys) {
            defaults_[i].push_back(canonicalKey(key));
        }
    }
    keys_ = defaults_;
    buildTree();
    updateButtons();
}

void KeyBindingsTabController::setContext(const TabContext& context) {
    context_ = context;
}

void KeyBindingsTabController::connectSignals() {
    connect(ui_->keyBindingAddButton, &QPushButton::clicked, this,
            &KeyBindingsTabController::onAddKey);
    connect(ui_->keyBindingRemoveButton, &QPushButton::clicked, this,
            &KeyBindingsTabController::onRemoveKey);
    connect(ui_->keyBindingResetButton, &QPushButton::clicked, this,
            &KeyBindingsTabController::onResetAction);
    connect(ui_->keyBindingResetAllButton, &QPushButton::clicked, this,
            &KeyBindingsTabController::onResetAll);
    connect(ui_->keyBindingTree, &QTreeWidget::itemSelectionChanged, this,
            &KeyBindingsTabController::onSelectionChanged);
    connect(ui_->keyBindingTree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* item, int) {
                if (item->data(0, kActionIndexRole).isValid()) onAddKey();
            });
}

void KeyBindingsTabController::buildTree() {
    QTreeWidget* tree = ui_->keyBindingTree;
    tree->clear();
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);

    const auto& table = actionTable();
    rows_.assign(table.size(), nullptr);

    // one group per context, in the order of the table
    std::map<KeyContext, QTreeWidgetItem*> groups;
    QFont groupFont = tree->font();
    groupFont.setBold(true);
    for (size_t i = 0; i < table.size(); i++) {
        const ActionInfo& info = table[i];
        auto it = groups.find(info.context);
        if (it == groups.end()) {
            auto* group = new QTreeWidgetItem(tree);
            group->setText(0, contextName(info.context));
            group->setFont(0, groupFont);
            group->setFirstColumnSpanned(true);
            group->setFlags(Qt::ItemIsEnabled);
            it = groups.emplace(info.context, group).first;
        }
        auto* row = new QTreeWidgetItem(it->second);
        row->setData(0, kActionIndexRole, static_cast<int>(i));
        row->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        const ActionName* name = findActionName(info.id);
        row->setText(0, name ? trk(name->name) : QString::fromUtf8(info.id));
        QString tip = QString::fromUtf8(info.id);
        if (name) tip = trk(name->description) + "\n" + tip;
        for (int col = 0; col < 3; col++) row->setToolTip(col, tip);
        row->setText(2, joinKeys(defaults_[i]));
        rows_[i] = row;
    }
    tree->expandAll();
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

bool KeyBindingsTabController::isDefault(size_t index) const {
    return sameKeySet(keys_[index], defaults_[index]);
}

void KeyBindingsTabController::refreshRow(size_t index) {
    QTreeWidgetItem* row = rows_[index];
    const auto& keys = keys_[index];
    row->setText(1, keys.empty() ? QCoreApplication::translate(
                                       "KeyBindingsTabController", "(none)")
                                 : joinKeys(keys));

    bool changed = !isDefault(index);
    QFont font = ui_->keyBindingTree->font();
    font.setBold(changed);
    for (int col = 0; col < 3; col++) row->setFont(col, font);

    bool unreadable = std::any_of(
        keys.begin(), keys.end(),
        [](const std::string& key) { return !parseKeyChord(key).has_value(); });
    row->setForeground(1, unreadable ? QBrush(Qt::red) : QBrush());
}

void KeyBindingsTabController::refreshAll() {
    for (size_t i = 0; i < rows_.size(); i++) refreshRow(i);
    refreshWarnings();
}

void KeyBindingsTabController::refreshWarnings() {
    const auto& table = actionTable();
    QStringList lines;

    // the same key on two actions that are looked up together
    for (size_t i = 0; i < table.size(); i++) {
        for (const auto& key : keys_[i]) {
            auto chord = parseKeyChord(key);
            if (!chord.has_value()) continue;
            for (size_t j = i + 1; j < table.size(); j++) {
                if (!contextsOverlap(table[i].context, table[j].context)) {
                    continue;
                }
                for (const auto& other : keys_[j]) {
                    auto otherChord = parseKeyChord(other);
                    if (otherChord.has_value() && *otherChord == *chord) {
                        lines
                            << QCoreApplication::translate(
                                   "KeyBindingsTabController",
                                   "%1 is assigned to both \"%2\" and \"%3\". "
                                   "The one listed first wins.")
                                   .arg(QString::fromStdString(key),
                                        rows_[i]->text(0), rows_[j]->text(0));
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < table.size(); i++) {
        for (const auto& key : keys_[i]) {
            if (!parseKeyChord(key).has_value()) {
                lines << QCoreApplication::translate(
                             "KeyBindingsTabController",
                             "\"%1\" of \"%2\" cannot be read and is ignored "
                             "by the clients.")
                             .arg(QString::fromStdString(key),
                                  rows_[i]->text(0));
            }
        }
    }

    ui_->keyBindingWarnings->setText(lines.join("\n"));
    ui_->keyBindingWarnings->setVisible(!lines.isEmpty());
}

int KeyBindingsTabController::selectedAction() const {
    auto items = ui_->keyBindingTree->selectedItems();
    if (items.isEmpty()) return -1;
    QVariant index = items.first()->data(0, kActionIndexRole);
    return index.isValid() ? index.toInt() : -1;
}

void KeyBindingsTabController::updateButtons() {
    int index = selectedAction();
    bool selected = index >= 0;
    ui_->keyBindingAddButton->setEnabled(selected);
    ui_->keyBindingRemoveButton->setEnabled(selected && !keys_[index].empty());
    ui_->keyBindingResetButton->setEnabled(selected && !isDefault(index));
}

void KeyBindingsTabController::onSelectionChanged() { updateButtons(); }

void KeyBindingsTabController::onAddKey() {
    int index = selectedAction();
    if (index < 0) return;

    KeyCaptureDialog dialog(window_);
    if (dialog.exec() != QDialog::Accepted || !dialog.key().has_value()) {
        return;
    }
    const std::string key = *dialog.key();
    auto& keys = keys_[index];
    if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
        keys.push_back(key);
    }
    refreshRow(index);
    refreshWarnings();
    updateButtons();
}

void KeyBindingsTabController::onRemoveKey() {
    int index = selectedAction();
    if (index < 0 || keys_[index].empty()) return;
    auto& keys = keys_[index];

    if (keys.size() == 1) {
        keys.clear();
    } else {
        // let the user pick which one
        QMenu menu(window_);
        for (const auto& key : keys) {
            menu.addAction(QString::fromStdString(key));
        }
        QAction* chosen = menu.exec(ui_->keyBindingRemoveButton->mapToGlobal(
            QPoint(0, ui_->keyBindingRemoveButton->height())));
        if (!chosen) return;
        const std::string key = chosen->text().toStdString();
        keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
    }
    refreshRow(index);
    refreshWarnings();
    updateButtons();
}

void KeyBindingsTabController::onResetAction() {
    int index = selectedAction();
    if (index < 0) return;
    keys_[index] = defaults_[index];
    refreshRow(index);
    refreshWarnings();
    updateButtons();
}

void KeyBindingsTabController::onResetAll() {
    auto reply = QMessageBox::question(
        window_,
        QCoreApplication::translate("KeyBindingsTabController",
                                    "Reset All to Default"),
        QCoreApplication::translate(
            "KeyBindingsTabController",
            "Reset the keys of all operations to the defaults?"),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;
    keys_ = defaults_;
    refreshAll();
    updateButtons();
}

void KeyBindingsTabController::loadFromConfig() {
    if (!context_.currentProfile) return;

    const auto& table = actionTable();
    keys_ = defaults_;
    unknown_.clear();
    for (const auto& binding : context_.currentProfile->key_bindings()) {
        auto it = std::find_if(table.begin(), table.end(),
                               [&](const ActionInfo& info) {
                                   return binding.action() == info.id;
                               });
        if (it == table.end()) {
            unknown_.push_back(binding);
            continue;
        }
        auto& keys = keys_[it - table.begin()];
        keys.clear();
        for (const auto& key : binding.keys()) {
            keys.push_back(canonicalKey(key));
        }
    }
    refreshAll();
    updateButtons();
}

void KeyBindingsTabController::saveToConfig() {
    if (!context_.currentProfile) return;

    const auto& table = actionTable();
    context_.currentProfile->clear_key_bindings();
    // actions not listed use the defaults, so only the changed ones go in
    for (size_t i = 0; i < table.size(); i++) {
        if (isDefault(i)) continue;
        auto* binding = context_.currentProfile->add_key_bindings();
        binding->set_action(table[i].id);
        for (const auto& key : keys_[i]) binding->add_keys(key);
    }
    for (const auto& binding : unknown_) {
        *context_.currentProfile->add_key_bindings() = binding;
    }
}

}  // namespace hazkey::settings
