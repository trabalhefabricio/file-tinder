#include "AdvancedFileTinderDialog.hpp"
#include "FolderTreeModel.hpp"
#include "MindMapView.hpp"
#include "FilterWidget.hpp"
#include "DatabaseManager.hpp"
#include "FileTinderExecutor.hpp"
#include "ui_constants.hpp"
#include <QKeyEvent>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolButton>
#include <QProgressDialog>
#include <QGroupBox>
#include <QScrollArea>
#include <QDir>
#include <QMimeDatabase>
#include <QSettings>

AdvancedFileTinderDialog::AdvancedFileTinderDialog(const QString& source_folder,
                                                   DatabaseManager& db,
                                                   QWidget* parent)
    : StandaloneFileTinderDialog(source_folder, db, parent)
    , scroll_area_(nullptr)
    , main_content_(nullptr)
    , mind_map_view_(nullptr)
    , file_info_panel_(nullptr)
    , adv_file_icon_label_(nullptr)
    , file_name_label_(nullptr)
    , file_details_label_(nullptr)
    , quick_access_panel_(nullptr)
    , quick_access_list_(nullptr)
    , filter_widget_(nullptr)
    , folder_model_(nullptr) {
    
    setWindowTitle("File Tinder - Advanced Mode");
    setMinimumSize(ui::dimensions::kAdvancedFileTinderMinWidth,
                   ui::dimensions::kAdvancedFileTinderMinHeight);
}

AdvancedFileTinderDialog::~AdvancedFileTinderDialog() = default;

void AdvancedFileTinderDialog::initialize() {
    StandaloneFileTinderDialog::initialize();
}

void AdvancedFileTinderDialog::setup_ui() {
    // Clear existing layout if any
    if (layout()) {
        QLayoutItem* item;
        while ((item = layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete layout();
    }
    
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(10, 10, 10, 10);
    main_layout->setSpacing(8);
    
    // Title bar with mode switch in upper right
    auto* title_bar = new QWidget();
    auto* title_layout = new QHBoxLayout(title_bar);
    title_layout->setContentsMargins(0, 0, 0, 0);
    
    auto* title_label = new QLabel("File Tinder - Advanced Mode");
    title_label->setStyleSheet("font-size: 16px; font-weight: bold;");
    title_layout->addWidget(title_label);
    
    title_layout->addStretch();
    
    // Zoom controls
    auto* zoom_out_btn = new QToolButton();
    zoom_out_btn->setText("-");
    zoom_out_btn->setToolTip("Zoom Out");
    connect(zoom_out_btn, &QToolButton::clicked, this, &AdvancedFileTinderDialog::on_zoom_out);
    title_layout->addWidget(zoom_out_btn);
    
    auto* zoom_in_btn = new QToolButton();
    zoom_in_btn->setText("+");
    zoom_in_btn->setToolTip("Zoom In");
    connect(zoom_in_btn, &QToolButton::clicked, this, &AdvancedFileTinderDialog::on_zoom_in);
    title_layout->addWidget(zoom_in_btn);
    
    auto* zoom_fit_btn = new QToolButton();
    zoom_fit_btn->setText("Fit");
    zoom_fit_btn->setToolTip("Fit to View");
    connect(zoom_fit_btn, &QToolButton::clicked, this, &AdvancedFileTinderDialog::on_zoom_fit);
    title_layout->addWidget(zoom_fit_btn);
    
    title_layout->addSpacing(20);
    
    switch_mode_btn_ = new QPushButton("Basic Mode");
    switch_mode_btn_->setStyleSheet(
        "QPushButton { padding: 5px 15px; background-color: #3498db; "
        "border-radius: 4px; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(switch_mode_btn_, &QPushButton::clicked, this, [this]() {
        save_folder_tree();
        save_quick_access();
        emit switch_to_basic_mode();
    });
    title_layout->addWidget(switch_mode_btn_);
    
    main_layout->addWidget(title_bar);
    
    // Filter bar (same as Basic Mode)
    setup_filter_bar();
    
    // Mind Map (main focus - no file preview)
    setup_mind_map();
    
    // File info panel below mind map
    setup_file_info_panel();
    
    // Quick Access panel (manual add/remove, 1-0 shortcuts)
    setup_quick_access_panel();
    
    // Action buttons at bottom
    setup_action_buttons();
    
    // Initialize folder model
    folder_model_ = new FolderTreeModel(this);
    folder_model_->set_root_folder(source_folder_);
    mind_map_view_->set_model(folder_model_);
    
    // Load saved data
    load_folder_tree();
    load_quick_access();
    
    // Initialize display
    if (!files_.empty()) {
        show_current_file();
    }
}

void AdvancedFileTinderDialog::setup_filter_bar() {
    filter_widget_ = new FilterWidget(this);
    connect(filter_widget_, &FilterWidget::filter_changed, this, &AdvancedFileTinderDialog::on_filter_changed);
    connect(filter_widget_, &FilterWidget::sort_changed, this, &AdvancedFileTinderDialog::on_sort_changed);
    static_cast<QVBoxLayout*>(layout())->addWidget(filter_widget_);
}

void AdvancedFileTinderDialog::setup_mind_map() {
    auto* map_group = new QGroupBox("Destination Folders");
    map_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* map_layout = new QVBoxLayout(map_group);
    map_layout->setContentsMargins(5, 15, 5, 5);
    
    mind_map_view_ = new MindMapView();
    mind_map_view_->setMinimumHeight(350);
    mind_map_view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    connect(mind_map_view_, &MindMapView::folder_clicked, 
            this, &AdvancedFileTinderDialog::on_folder_clicked);
    connect(mind_map_view_, &MindMapView::folder_context_menu,
            this, &AdvancedFileTinderDialog::on_folder_context_menu);
    connect(mind_map_view_, &MindMapView::add_folder_requested,
            this, &AdvancedFileTinderDialog::on_add_node_clicked);
    
    map_layout->addWidget(mind_map_view_);
    
    // Hint label
    auto* hint = new QLabel("Click folder to assign file. Click [+] to add destination. Right-click for options.");
    hint->setStyleSheet("color: #666; font-size: 10px;");
    map_layout->addWidget(hint);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(map_group, 1);
}

void AdvancedFileTinderDialog::setup_file_info_panel() {
    file_info_panel_ = new QWidget();
    file_info_panel_->setStyleSheet("background-color: #f5f5f5; border-radius: 4px; padding: 8px;");
    auto* info_layout = new QHBoxLayout(file_info_panel_);
    info_layout->setContentsMargins(10, 8, 10, 8);
    
    // File type icon
    adv_file_icon_label_ = new QLabel("[FILE]");
    adv_file_icon_label_->setStyleSheet("font-size: 18px; font-weight: bold; color: #333; min-width: 60px;");
    adv_file_icon_label_->setAlignment(Qt::AlignCenter);
    info_layout->addWidget(adv_file_icon_label_);
    
    // File name and details
    auto* text_widget = new QWidget();
    auto* text_layout = new QVBoxLayout(text_widget);
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);
    
    file_name_label_ = new QLabel("No file selected");
    file_name_label_->setStyleSheet("font-size: 14px; font-weight: bold;");
    text_layout->addWidget(file_name_label_);
    
    file_details_label_ = new QLabel("");
    file_details_label_->setStyleSheet("font-size: 11px; color: #666;");
    text_layout->addWidget(file_details_label_);
    
    info_layout->addWidget(text_widget, 1);
    
    // Progress
    progress_bar_ = new QProgressBar();
    progress_bar_->setMaximumWidth(150);
    progress_bar_->setTextVisible(true);
    info_layout->addWidget(progress_bar_);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(file_info_panel_);
}

void AdvancedFileTinderDialog::setup_quick_access_panel() {
    quick_access_panel_ = new QWidget();
    auto* qa_layout = new QHBoxLayout(quick_access_panel_);
    qa_layout->setContentsMargins(0, 0, 0, 0);
    
    auto* qa_label = new QLabel("Quick Access (1-0):");
    qa_label->setStyleSheet("font-weight: bold;");
    qa_layout->addWidget(qa_label);
    
    quick_access_list_ = new QListWidget();
    quick_access_list_->setFlow(QListView::LeftToRight);
    quick_access_list_->setMaximumHeight(40);
    quick_access_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    quick_access_list_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    quick_access_list_->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { padding: 4px 10px; background: #eee; border-radius: 3px; margin-right: 4px; }"
        "QListWidget::item:hover { background: #ddd; }"
        "QListWidget::item:selected { background: #0078d4; color: white; }"
    );
    connect(quick_access_list_, &QListWidget::itemClicked,
            this, &AdvancedFileTinderDialog::on_quick_access_clicked);
    qa_layout->addWidget(quick_access_list_, 1);
    
    auto* add_qa_btn = new QPushButton("+");
    add_qa_btn->setMaximumWidth(30);
    add_qa_btn->setToolTip("Add folder to Quick Access");
    connect(add_qa_btn, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_add_quick_access);
    qa_layout->addWidget(add_qa_btn);
    
    auto* remove_qa_btn = new QPushButton("-");
    remove_qa_btn->setMaximumWidth(30);
    remove_qa_btn->setToolTip("Remove selected from Quick Access");
    connect(remove_qa_btn, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_remove_quick_access);
    qa_layout->addWidget(remove_qa_btn);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(quick_access_panel_);
}

void AdvancedFileTinderDialog::setup_action_buttons() {
    // Action buttons (thin, full width)
    auto* action_widget = new QWidget();
    auto* action_layout = new QHBoxLayout(action_widget);
    action_layout->setSpacing(8);
    
    delete_btn_ = new QPushButton("Delete [<-]");
    delete_btn_->setMinimumHeight(ui::dimensions::kThinButtonHeight);
    delete_btn_->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #c0392b; }"
    ).arg(ui::colors::kDeleteColor));
    connect(delete_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_delete);
    action_layout->addWidget(delete_btn_, 1);
    
    skip_btn_ = new QPushButton("Skip [Down]");
    skip_btn_->setMinimumHeight(ui::dimensions::kThinButtonHeight);
    skip_btn_->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #e67e22; }"
    ).arg(ui::colors::kSkipColor));
    connect(skip_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_skip);
    action_layout->addWidget(skip_btn_, 1);
    
    back_btn_ = new QPushButton("Back [Up]");
    back_btn_->setMinimumHeight(ui::dimensions::kThinButtonHeight);
    back_btn_->setStyleSheet(
        "QPushButton { background-color: #95a5a6; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #7f8c8d; }"
    );
    connect(back_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_back);
    action_layout->addWidget(back_btn_, 1);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(action_widget);
    
    // Bottom bar with Cancel and Finish
    auto* bottom_widget = new QWidget();
    auto* bottom_layout = new QHBoxLayout(bottom_widget);
    
    auto* cancel_btn = new QPushButton("Cancel");
    cancel_btn->setStyleSheet("QPushButton { padding: 8px 16px; }");
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
    bottom_layout->addWidget(cancel_btn);
    
    bottom_layout->addStretch();
    
    // Stats
    stats_label_ = new QLabel();
    stats_label_->setStyleSheet("color: #666;");
    bottom_layout->addWidget(stats_label_);
    
    bottom_layout->addStretch();
    
    finish_btn_ = new QPushButton("Finish & Execute");
    finish_btn_->setStyleSheet(
        "QPushButton { padding: 10px 25px; background-color: #27ae60; "
        "color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #2ecc71; }"
    );
    connect(finish_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_finish);
    bottom_layout->addWidget(finish_btn_);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(bottom_widget);
}

void AdvancedFileTinderDialog::on_folder_clicked(const QString& folder_path) {
    int file_idx = get_current_file_index();
    if (file_idx < 0) return;
    
    auto& file = files_[file_idx];
    
    // Update counts
    if (file.decision != "pending") {
        update_decision_count(file.decision, -1);
        if (file.decision == "move" && !file.destination_folder.isEmpty() && folder_model_) {
            folder_model_->unassign_file_from_folder(file.destination_folder);
        }
    }
    
    file.decision = "move";
    file.destination_folder = folder_path;
    move_count_++;
    
    if (folder_model_) folder_model_->assign_file_to_folder(folder_path);
    if (mind_map_view_) mind_map_view_->set_selected_folder(folder_path);
    
    advance_to_next();
}

void AdvancedFileTinderDialog::on_add_node_clicked() {
    prompt_add_folder();
}

void AdvancedFileTinderDialog::prompt_add_folder() {
    QMenu menu;
    
    menu.addAction("Create New Folder...", [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, "New Folder",
            "Enter folder name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            QString new_path = source_folder_ + "/" + name;
            folder_model_->add_folder(new_path, true);  // Virtual folder
            mind_map_view_->refresh_layout();
        }
    });
    
    menu.addAction("Add Existing Folder...", [this]() {
        QString folder = QFileDialog::getExistingDirectory(this, "Select Folder", source_folder_);
        if (!folder.isEmpty()) {
            folder_model_->add_folder(folder, false);  // Existing folder
            mind_map_view_->refresh_layout();
        }
    });
    
    menu.exec(QCursor::pos());
}

void AdvancedFileTinderDialog::on_folder_context_menu(const QString& folder_path, const QPoint& pos) {
    QMenu menu;
    
    menu.addAction("Move File Here", [this, folder_path]() {
        on_folder_clicked(folder_path);
    });
    
    menu.addSeparator();
    
    menu.addAction("Add to Quick Access", [this, folder_path]() {
        add_to_quick_access(folder_path);
    });
    
    menu.addAction("Add Subfolder...", [this, folder_path]() {
        bool ok;
        QString name = QInputDialog::getText(this, "New Subfolder",
            "Enter folder name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            QString new_path = folder_path + "/" + name;
            folder_model_->add_folder(new_path, true);
            mind_map_view_->refresh_layout();
        }
    });
    
    FolderNode* node = folder_model_->find_node(folder_path);
    if (node && !node->exists && node->path != source_folder_) {
        menu.addSeparator();
        menu.addAction("Remove", [this, folder_path]() {
            folder_model_->remove_folder(folder_path);
            mind_map_view_->refresh_layout();
        });
    }
    
    menu.exec(pos);
}

void AdvancedFileTinderDialog::load_quick_access() {
    quick_access_folders_ = db_.get_quick_access_folders(source_folder_);
    update_quick_access_display();
}

void AdvancedFileTinderDialog::save_quick_access() {
    db_.save_quick_access_folders(source_folder_, quick_access_folders_);
}

void AdvancedFileTinderDialog::add_to_quick_access(const QString& folder_path) {
    if (quick_access_folders_.contains(folder_path)) return;
    if (quick_access_folders_.size() >= kMaxQuickAccess) {
        QMessageBox::warning(this, "Quick Access Full",
            "Quick Access is limited to 10 folders. Remove one first.");
        return;
    }
    quick_access_folders_.append(folder_path);
    update_quick_access_display();
}

void AdvancedFileTinderDialog::remove_from_quick_access(int index) {
    if (index >= 0 && index < quick_access_folders_.size()) {
        quick_access_folders_.removeAt(index);
        update_quick_access_display();
    }
}

void AdvancedFileTinderDialog::update_quick_access_display() {
    quick_access_list_->clear();
    
    for (int i = 0; i < quick_access_folders_.size(); ++i) {
        QString path = quick_access_folders_[i];
        QString label = QString("%1: %2").arg(i == 9 ? 0 : i + 1).arg(QFileInfo(path).fileName());
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        quick_access_list_->addItem(item);
    }
}

void AdvancedFileTinderDialog::on_quick_access_clicked(QListWidgetItem* item) {
    QString folder_path = item->data(Qt::UserRole).toString();
    if (!folder_path.isEmpty()) {
        on_folder_clicked(folder_path);
    }
}

void AdvancedFileTinderDialog::on_add_quick_access() {
    QString folder = QFileDialog::getExistingDirectory(this, "Add to Quick Access", source_folder_);
    if (!folder.isEmpty()) {
        add_to_quick_access(folder);
    }
}

void AdvancedFileTinderDialog::on_remove_quick_access() {
    int row = quick_access_list_->currentRow();
    if (row >= 0) {
        remove_from_quick_access(row);
    }
}

void AdvancedFileTinderDialog::update_file_info_display() {
    int idx = get_current_file_index();
    if (idx < 0 || idx >= static_cast<int>(files_.size())) {
        if (adv_file_icon_label_) adv_file_icon_label_->setText("[---]");
        if (file_name_label_) file_name_label_->setText("No file selected");
        if (file_details_label_) file_details_label_->setText("");
        return;
    }
    
    const auto& file = files_[idx];
    QString path = file.path;
    QFileInfo info(path);
    
    // Icon
    if (adv_file_icon_label_) adv_file_icon_label_->setText(get_file_type_icon(path));
    
    // Name
    if (file_name_label_) file_name_label_->setText(info.fileName());
    
    // Details
    QString size_str;
    qint64 size = info.size();
    if (size < 1024) size_str = QString("%1 B").arg(size);
    else if (size < 1024*1024) size_str = QString("%1 KB").arg(size/1024.0, 0, 'f', 1);
    else if (size < 1024LL*1024*1024) size_str = QString("%1 MB").arg(size/(1024.0*1024.0), 0, 'f', 2);
    else size_str = QString("%1 GB").arg(size/(1024.0*1024.0*1024.0), 0, 'f', 2);
    
    QString details = QString("%1 | %2 | %3")
        .arg(size_str)
        .arg(info.isDir() ? "Folder" : info.suffix().toUpper())
        .arg(info.lastModified().toString("yyyy-MM-dd hh:mm"));
    if (file_details_label_) file_details_label_->setText(details);
}

QString AdvancedFileTinderDialog::get_file_type_icon(const QString& path) {
    QFileInfo info(path);
    if (info.isDir()) return "[DIR]";
    
    QMimeDatabase db;
    QString mime = db.mimeTypeForFile(path).name();
    
    if (mime.startsWith("image/")) return "[IMG]";
    if (mime.startsWith("video/")) return "[VID]";
    if (mime.startsWith("audio/")) return "[AUD]";
    if (mime.contains("pdf") || mime.contains("document") || mime.contains("text")) return "[DOC]";
    if (mime.contains("zip") || mime.contains("archive") || mime.contains("compressed")) return "[ZIP]";
    return "[FILE]";
}

void AdvancedFileTinderDialog::show_current_file() {
    update_file_info_display();
    update_stats();
    
    // Update progress bar
    if (progress_bar_) {
        int reviewed = keep_count_ + delete_count_ + skip_count_ + move_count_;
        int total = static_cast<int>(files_.size());
        progress_bar_->setMaximum(total);
        progress_bar_->setValue(reviewed);
        progress_bar_->setFormat(QString("%1 / %2").arg(reviewed).arg(total));
    }
}

void AdvancedFileTinderDialog::on_finish() {
    save_folder_tree();
    save_quick_access();
    show_review_summary();
}

void AdvancedFileTinderDialog::keyPressEvent(QKeyEvent* event) {
    // Number keys 1-9, 0 for quick access
    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        int index = event->key() - Qt::Key_1;
        if (index < quick_access_folders_.size()) {
            on_folder_clicked(quick_access_folders_[index]);
            return;
        }
    }
    if (event->key() == Qt::Key_0) {
        if (quick_access_folders_.size() >= 10) {
            on_folder_clicked(quick_access_folders_[9]);
            return;
        }
    }
    
    // Arrow keys same as Basic Mode
    switch (event->key()) {
        case Qt::Key_Left:
            on_delete();
            break;
        case Qt::Key_Right:
            // In Advanced Mode, right doesn't auto-keep, need to select folder
            break;
        case Qt::Key_Down:
            on_skip();
            break;
        case Qt::Key_Up:
            on_back();
            break;
        case Qt::Key_Plus:
            if (event->modifiers() & Qt::ControlModifier) {
                on_zoom_in();
            }
            break;
        case Qt::Key_Minus:
            if (event->modifiers() & Qt::ControlModifier) {
                on_zoom_out();
            }
            break;
        case Qt::Key_N:
            prompt_add_folder();
            break;
        default:
            StandaloneFileTinderDialog::keyPressEvent(event);
    }
}

void AdvancedFileTinderDialog::on_filter_changed() {
    // Re-apply filter from FilterWidget
    // This would need integration with the filter system
}

void AdvancedFileTinderDialog::on_sort_changed() {
    // Re-apply sort from FilterWidget
}

void AdvancedFileTinderDialog::on_zoom_in() {
    mind_map_view_->zoom_in();
}

void AdvancedFileTinderDialog::on_zoom_out() {
    mind_map_view_->zoom_out();
}

void AdvancedFileTinderDialog::on_zoom_fit() {
    mind_map_view_->zoom_fit();
}

void AdvancedFileTinderDialog::load_folder_tree() {
    if (folder_model_) {
        folder_model_->load_from_database(db_, source_folder_);
    }
    if (mind_map_view_) {
        mind_map_view_->refresh_layout();
    }
}

void AdvancedFileTinderDialog::save_folder_tree() {
    if (folder_model_) {
        folder_model_->save_to_database(db_, source_folder_);
    }
}
