#include "FileListWindow.hpp"
#include "StandaloneFileTinderDialog.hpp"
#include "ui_constants.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDrag>
#include <QMimeData>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QApplication>
#include <QMouseEvent>

static const int kFileIndexRole = Qt::UserRole + 200;

FileListWindow::FileListWindow(std::vector<FileToProcess>& files,
                               const std::vector<int>& filtered_indices,
                               int current_index,
                               QWidget* parent)
    : QDialog(parent, Qt::Tool | Qt::WindowStaysOnTopHint)
    , files_(files)
    , filtered_indices_(filtered_indices)
    , current_index_(current_index)
{
    setWindowTitle("File List");
    build_ui();
    update_list();
}

void FileListWindow::build_ui() {
    setMinimumSize(ui::scaling::scaled(320), ui::scaling::scaled(400));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Filter bar
    auto* filter_row = new QHBoxLayout();
    filter_edit_ = new QLineEdit();
    filter_edit_->setPlaceholderText("Filter files...");
    filter_edit_->setStyleSheet(
        "padding: 5px 8px; background-color: #2d2d2d; border: 1px solid #555; color: #ecf0f1;");
    connect(filter_edit_, &QLineEdit::textChanged, this, &FileListWindow::on_filter_changed);
    filter_row->addWidget(filter_edit_);
    layout->addLayout(filter_row);

    // File list
    list_widget_ = new QListWidget();
    list_widget_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list_widget_->setDragEnabled(true);
    list_widget_->setStyleSheet(
        "QListWidget { background-color: #1e1e1e; border: 1px solid #404040; color: #ecf0f1; }"
        "QListWidget::item { padding: 3px 6px; border-bottom: 1px solid #333; }"
        "QListWidget::item:selected { background-color: #0078d4; }"
        "QListWidget::item:hover { background-color: #2a2a2a; }");
    connect(list_widget_, &QListWidget::itemClicked, this, &FileListWindow::on_item_clicked);
    connect(list_widget_, &QListWidget::itemDoubleClicked, this, &FileListWindow::on_item_double_clicked);

    // Context menu for assigning selected files
    list_widget_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(list_widget_, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto selected = list_widget_->selectedItems();
        if (selected.isEmpty() || destination_folders_.isEmpty()) return;

        QMenu menu(this);
        auto* header = menu.addAction("Move selected to:");
        header->setEnabled(false);
        menu.addSeparator();

        for (const QString& folder : destination_folders_) {
            QString display = QFileInfo(folder).fileName();
            if (display.isEmpty()) display = folder;
            auto* action = menu.addAction(display);
            action->setData(folder);
            action->setToolTip(folder);
        }

        auto* chosen = menu.exec(list_widget_->viewport()->mapToGlobal(pos));
        if (chosen && chosen->data().isValid()) {
            QList<int> indices;
            for (auto* item : selected) {
                int fi = item->data(kFileIndexRole).toInt();
                indices.append(fi);
            }
            emit files_assigned(indices, chosen->data().toString());
        }
    });

    layout->addWidget(list_widget_, 1);

    // Status bar
    auto* status_row = new QHBoxLayout();
    count_label_ = new QLabel();
    count_label_->setStyleSheet("color: #888; font-size: 10px;");
    status_row->addWidget(count_label_);
    status_row->addStretch();
    selection_label_ = new QLabel();
    selection_label_->setStyleSheet("color: #888; font-size: 10px;");
    status_row->addWidget(selection_label_);
    layout->addLayout(status_row);

    // Tip
    auto* tip = new QLabel("Click: navigate | Ctrl/Shift+Click: multi-select | Right-click: assign to folder");
    tip->setStyleSheet("color: #666; font-size: 9px;");
    tip->setWordWrap(true);
    layout->addWidget(tip);
}

void FileListWindow::refresh(const std::vector<int>& filtered_indices, int current_index) {
    filtered_indices_ = filtered_indices;
    current_index_ = current_index;
    update_list();
}

void FileListWindow::set_destination_folders(const QStringList& folders) {
    destination_folders_ = folders;
}

void FileListWindow::update_list() {
    list_widget_->clear();
    QString filter_text = filter_edit_->text().toLower();

    int shown = 0;
    for (int i = 0; i < static_cast<int>(filtered_indices_.size()); ++i) {
        int fi = filtered_indices_[i];
        if (fi < 0 || fi >= static_cast<int>(files_.size())) continue;
        const auto& file = files_[fi];

        // Apply text filter
        if (!filter_text.isEmpty() && !file.name.toLower().contains(filter_text)) continue;

        // Build display text
        QString status;
        if (file.decision == "pending") status = "[ ]";
        else if (file.decision == "keep") status = "[K]";
        else if (file.decision == "delete") status = "[D]";
        else if (file.decision == "skip") status = "[S]";
        else if (file.decision == "move") status = "[M]";
        else if (file.decision == "copy") status = "[C]";
        else status = "[?]";

        QString display = QString("%1 %2").arg(status, file.name);

        auto* item = new QListWidgetItem(display);
        item->setData(kFileIndexRole, fi);
        item->setData(Qt::UserRole + 201, i);  // filtered index

        // Color by decision
        if (file.decision == "keep") item->setForeground(QColor("#2ecc71"));
        else if (file.decision == "delete") item->setForeground(QColor("#e74c3c"));
        else if (file.decision == "skip") item->setForeground(QColor("#95a5a6"));
        else if (file.decision == "move") item->setForeground(QColor("#3498db"));
        else if (file.decision == "copy") item->setForeground(QColor("#9b59b6"));

        // Highlight current file
        if (i == current_index_) {
            item->setBackground(QColor("#2c3e50"));
            auto font = item->font();
            font.setBold(true);
            item->setFont(font);
        }

        item->setToolTip(file.path);
        list_widget_->addItem(item);
        ++shown;
    }

    count_label_->setText(QString("%1 / %2 files").arg(shown).arg(filtered_indices_.size()));
    selection_label_->setText("0 selected");

    connect(list_widget_, &QListWidget::itemSelectionChanged, this, [this]() {
        int sel = list_widget_->selectedItems().size();
        selection_label_->setText(QString("%1 selected").arg(sel));
    });
}

void FileListWindow::on_filter_changed(const QString&) {
    update_list();
}

void FileListWindow::on_item_clicked(QListWidgetItem* item) {
    if (!item) return;
    // Only navigate on single-click without modifier keys
    if (QApplication::keyboardModifiers() & (Qt::ShiftModifier | Qt::ControlModifier))
        return;
    int filtered_idx = item->data(Qt::UserRole + 201).toInt();
    emit file_selected(filtered_idx);
}

void FileListWindow::on_item_double_clicked(QListWidgetItem* item) {
    if (!item) return;
    int filtered_idx = item->data(Qt::UserRole + 201).toInt();
    emit file_selected(filtered_idx);
}
