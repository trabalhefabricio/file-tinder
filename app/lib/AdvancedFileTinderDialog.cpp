#include "AdvancedFileTinderDialog.hpp"
#include "FolderTreeModel.hpp"
#include "MindMapView.hpp"
#include "FilterWidget.hpp"
#include "DatabaseManager.hpp"
#include "FileTinderExecutor.hpp"
#include "ui_constants.hpp"
#include <QKeyEvent>
#include <QCloseEvent>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QListView>
#include <QTreeView>
#include <QMessageBox>
#include <QProgressDialog>
#include <QGroupBox>
#include <QScrollArea>
#include <QDir>
#include <QMimeDatabase>
#include <QSettings>
#include <QScreen>
#include <QDesktopServices>
#include <QUrl>

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
    setMinimumWidth(ui::scaling::scaled(ui::dimensions::kAdvancedFileTinderMinWidth));
}

AdvancedFileTinderDialog::~AdvancedFileTinderDialog() = default;

void AdvancedFileTinderDialog::initialize() {
    StandaloneFileTinderDialog::initialize();
    
    // Override the base class sizing for advanced mode (needs more width)
    if (auto* screen = QApplication::primaryScreen()) {
        QRect avail = screen->availableGeometry();
        int w = qMin(ui::scaling::scaled(ui::dimensions::kAdvancedFileTinderMinWidth), avail.width() * 9 / 10);
        int h = qMin(ui::scaling::scaled(ui::dimensions::kAdvancedFileTinderMinHeight), avail.height() * 8 / 10);
        resize(w, h);
    }
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
    
    // Requirement 2: Detect missing folders (deleted from disk between sessions)
    check_missing_folders();
    
    // Requirement 1: First-time onboarding prompt
    if (folder_model_ && folder_model_->root_node() && 
        folder_model_->root_node()->children.empty()) {
        QMessageBox::information(this, "Welcome to Advanced Mode",
            "Welcome to Advanced Mode!\n\n"
            "• Click the [+ Add Folder] button to add destination folders\n"
            "• Click any folder to assign the current file to it\n"
            "• Use keys 1-0 for Quick Access slots\n"
            "• Right-click folders for more options\n\n"
            "Tip: You can add folders outside the source directory — "
            "they'll be shown in purple.");
    }
    
    // Initialize display
    if (!files_.empty()) {
        show_current_file();
    }
}

void AdvancedFileTinderDialog::setup_filter_bar() {
    filter_widget_ = new FilterWidget(this);
    connect(filter_widget_, &FilterWidget::filter_changed, this, &AdvancedFileTinderDialog::on_filter_changed);
    connect(filter_widget_, &FilterWidget::sort_changed, this, &AdvancedFileTinderDialog::on_sort_changed);
    connect(filter_widget_, &FilterWidget::include_folders_changed, this, [this](bool include) {
        include_folders_ = include;
        // Reset counts before re-scanning and reloading state
        keep_count_ = 0;
        delete_count_ = 0;
        skip_count_ = 0;
        move_count_ = 0;
        scan_files();
        apply_sort();
        rebuild_filtered_indices();
        load_session_state();
        if (!filtered_indices_.empty()) {
            show_current_file();
        }
    });
    static_cast<QVBoxLayout*>(layout())->addWidget(filter_widget_);
}

void AdvancedFileTinderDialog::setup_mind_map() {
    auto* map_group = new QGroupBox("Destination Folders");
    map_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* map_layout = new QVBoxLayout(map_group);
    map_layout->setContentsMargins(5, 15, 5, 5);
    
    mind_map_view_ = new MindMapView();
    mind_map_view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    connect(mind_map_view_, &MindMapView::folder_clicked, 
            this, &AdvancedFileTinderDialog::on_folder_clicked);
    connect(mind_map_view_, &MindMapView::folder_context_menu,
            this, &AdvancedFileTinderDialog::on_folder_context_menu);
    connect(mind_map_view_, &MindMapView::add_folder_requested,
            this, &AdvancedFileTinderDialog::on_add_node_clicked);
    
    map_layout->addWidget(mind_map_view_);
    
    // Hint label
    auto* hint = new QLabel("Click folder to assign file. [+] to add destination. Right-click for options. Keys: K=Keep, D/←=Delete, S/↓=Skip, N=New folder, 1-0=Quick Access");
    hint->setStyleSheet("color: #666; font-size: 10px;");
    hint->setWordWrap(true);
    map_layout->addWidget(hint);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(map_group, 1);
}

void AdvancedFileTinderDialog::setup_file_info_panel() {
    file_info_panel_ = new QWidget();
    file_info_panel_->setStyleSheet("background-color: #f5f5f5; border-radius: 4px; padding: 8px;");
    file_info_panel_->setCursor(Qt::PointingHandCursor);
    file_info_panel_->setToolTip("Double-click to open file");
    file_info_panel_->installEventFilter(this);
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
    
    int btn_h = ui::scaling::scaled(ui::dimensions::kThinButtonHeight);
    
    delete_btn_ = new QPushButton("Delete [<-]");
    delete_btn_->setMinimumHeight(btn_h);
    delete_btn_->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #c0392b; }"
    ).arg(ui::colors::kDeleteColor));
    connect(delete_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_delete);
    action_layout->addWidget(delete_btn_, 1);
    
    keep_btn_ = new QPushButton("Keep [K]");
    keep_btn_->setMinimumHeight(btn_h);
    keep_btn_->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #27ae60; }"
    ).arg(ui::colors::kKeepColor));
    connect(keep_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_keep);
    action_layout->addWidget(keep_btn_, 1);
    
    skip_btn_ = new QPushButton("Skip [Down]");
    skip_btn_->setMinimumHeight(btn_h);
    skip_btn_->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #e67e22; }"
    ).arg(ui::colors::kSkipColor));
    connect(skip_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_skip);
    action_layout->addWidget(skip_btn_, 1);
    
    back_btn_ = new QPushButton("Back [Up]");
    back_btn_->setMinimumHeight(btn_h);
    back_btn_->setStyleSheet(
        "QPushButton { background-color: #95a5a6; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #7f8c8d; }"
    );
    connect(back_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_back);
    action_layout->addWidget(back_btn_, 1);
    
    undo_btn_ = new QPushButton("Undo [Z]");
    undo_btn_->setMinimumHeight(btn_h);
    undo_btn_->setStyleSheet(
        "QPushButton { background-color: #9b59b6; color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #8e44ad; }"
        "QPushButton:disabled { background-color: #5d4e6e; color: #888; }"
    );
    undo_btn_->setEnabled(false);
    connect(undo_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_undo);
    action_layout->addWidget(undo_btn_, 1);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(action_widget);
    
    // Bottom bar with Cancel and Finish
    auto* bottom_widget = new QWidget();
    auto* bottom_layout = new QHBoxLayout(bottom_widget);
    
    auto* cancel_btn = new QPushButton("Cancel");
    cancel_btn->setStyleSheet("QPushButton { padding: 8px 16px; }");
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
    bottom_layout->addWidget(cancel_btn);
    
    // Reset Progress button
    auto* reset_btn = new QPushButton("Reset");
    reset_btn->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #e74c3c; "
        "color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: #c0392b; }"
    );
    reset_btn->setToolTip("Reset all decisions and start over");
    connect(reset_btn, &QPushButton::clicked, this, [this]() { on_reset_progress(); });
    bottom_layout->addWidget(reset_btn);
    
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
    
    // Requirement 8: Clicking root folder = Keep in place
    if (folder_path == source_folder_) {
        on_keep();
        return;
    }
    
    auto& file = files_[file_idx];
    
    QString old_decision = file.decision;
    QString old_dest_folder = file.destination_folder;
    
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
    
    // Record for undo (store the OLD destination so it can be restored)
    record_action(file_idx, old_decision, "move", old_dest_folder);
    
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
            // Requirement 5: Duplicate folder prompt
            if (folder_model_ && folder_model_->find_node(new_path)) {
                QMessageBox::information(this, "Already Added",
                    QString("The folder '%1' is already in the tree.").arg(name));
                return;
            }
            if (folder_model_) folder_model_->add_folder(new_path, true);  // Virtual folder
            if (mind_map_view_) mind_map_view_->refresh_layout();
        }
    });
    
    menu.addAction("Add Existing Folder(s)...", [this]() {
        // Use non-native dialog to support multi-select
        QFileDialog dlg(this, "Select Folder(s)", source_folder_);
        dlg.setFileMode(QFileDialog::Directory);
        dlg.setOption(QFileDialog::ShowDirsOnly, true);
        dlg.setOption(QFileDialog::DontUseNativeDialog, true);
        // Enable multi-selection in the file list view
        if (auto* list_view = dlg.findChild<QListView*>("listView")) {
            list_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }
        if (auto* tree_view = dlg.findChild<QTreeView*>()) {
            tree_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }
        
        if (dlg.exec() == QDialog::Accepted) {
            QStringList folders = dlg.selectedFiles();
            int added = 0;
            for (const QString& folder : folders) {
                if (folder.isEmpty()) continue;
                
                // Requirement 5: Duplicate folder prompt
                if (folder_model_ && folder_model_->find_node(folder)) {
                    continue;  // Skip duplicates silently during multi-add
                }
                // Requirement 4: Warn about external folders
                bool is_external = !folder.startsWith(source_folder_);
                if (is_external && added == 0) {
                    auto reply = QMessageBox::question(this, "External Folder",
                        QString("One or more folders are outside the source directory.\n\n"
                                "Files moved there will leave the original folder's location. Continue?"),
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                    if (reply != QMessageBox::Yes) return;
                }
                if (folder_model_) folder_model_->add_folder(folder, false);
                added++;
            }
            if (added > 0 && mind_map_view_) mind_map_view_->refresh_layout();
        }
    });
    
    menu.exec(QCursor::pos());
}

void AdvancedFileTinderDialog::on_folder_context_menu(const QString& folder_path, const QPoint& pos) {
    QMenu menu;
    
    // Don't offer "Move File Here" for root (Req 8 — root = keep)
    if (folder_path != source_folder_) {
        menu.addAction("Move File Here", [this, folder_path]() {
            on_folder_clicked(folder_path);
        });
        menu.addSeparator();
    }
    
    menu.addAction("Add to Quick Access", [this, folder_path]() {
        add_to_quick_access(folder_path);
    });
    
    menu.addAction("Add Subfolder...", [this, folder_path]() {
        bool ok;
        QString name = QInputDialog::getText(this, "New Subfolder",
            "Enter folder name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            QString new_path = folder_path + "/" + name;
            if (folder_model_ && folder_model_->find_node(new_path)) {
                QMessageBox::information(this, "Already Added",
                    QString("The folder '%1' is already in the tree.").arg(name));
                return;
            }
            if (folder_model_) folder_model_->add_folder(new_path, true);
            if (mind_map_view_) mind_map_view_->refresh_layout();
        }
    });
    
    if (folder_model_) {
        FolderNode* node = folder_model_->find_node(folder_path);
        if (node) {
            // Requirement 13: Root removal — prompt to change root and start new session
            if (node->path == source_folder_) {
                menu.addSeparator();
                menu.addAction("Change Root Folder...", [this]() {
                    auto reply = QMessageBox::question(this, "Change Root Folder",
                        "Changing the root folder will save this session and start a new one.\n\n"
                        "Do you want to proceed?",
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                    if (reply == QMessageBox::Yes) {
                        save_folder_tree();
                        save_quick_access();
                        save_session_state();
                        reject();
                    }
                });
            }
            // Requirement 14: Allow removing any non-root folder (virtual or real)
            else {
                menu.addSeparator();
                menu.addAction("Remove from Tree", [this, folder_path, node]() {
                    // Requirement 12: Warn about assigned files
                    if (node->assigned_file_count > 0) {
                        auto reply = QMessageBox::warning(this, "Folder Has Assigned Files",
                            QString("This folder has %1 file(s) assigned to it.\n\n"
                                    "Removing it will set those files back to 'pending' "
                                    "and they will appear at the bottom of the file list.\n\n"
                                    "Do you want to proceed?")
                                .arg(node->assigned_file_count),
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                        if (reply != QMessageBox::Yes) return;
                        
                        // Revert assigned files to pending
                        for (auto& file : files_) {
                            if (file.decision == "move" && file.destination_folder == folder_path) {
                                file.decision = "pending";
                                file.destination_folder.clear();
                                move_count_--;
                            }
                        }
                    }
                    
                    // Requirement 11: If folder is in Quick Access, ask user
                    if (quick_access_folders_.contains(folder_path)) {
                        auto reply = QMessageBox::question(this, "Also in Quick Access",
                            "This folder is also in Quick Access. Remove it from Quick Access too?",
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                        if (reply == QMessageBox::Yes) {
                            quick_access_folders_.removeAll(folder_path);
                            update_quick_access_display();
                        }
                    }
                    
                    if (folder_model_) folder_model_->remove_folder(folder_path);
                    if (mind_map_view_) mind_map_view_->refresh_layout();
                    update_stats();
                    show_current_file();
                });
            }
        }
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
        // Requirement 10: Prompt to choose one to replace
        auto reply = QMessageBox::question(this, "Quick Access Full",
            QString("Quick Access is full (%1/%2).\n\n"
                    "Do you want to choose a slot to replace?")
                .arg(quick_access_folders_.size()).arg(kMaxQuickAccess),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (reply != QMessageBox::Yes) return;
        
        // Build list of current slots for user to pick from
        QStringList items;
        for (int i = 0; i < quick_access_folders_.size(); ++i) {
            items << QString("%1: %2").arg(i == 9 ? 0 : i + 1)
                     .arg(QFileInfo(quick_access_folders_[i]).fileName());
        }
        bool ok;
        QString chosen = QInputDialog::getItem(this, "Replace Quick Access Slot",
            "Select a slot to replace:", items, 0, false, &ok);
        if (!ok) return;
        
        int slot = items.indexOf(chosen);
        if (slot >= 0) {
            quick_access_folders_[slot] = folder_path;
            update_quick_access_display();
        }
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
    if (size < 1024LL) size_str = QString("%1 B").arg(size);
    else if (size < 1024LL*1024) size_str = QString("%1 KB").arg(size/1024.0, 0, 'f', 1);
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
    
    // Update progress bar with filtered items
    if (progress_bar_) {
        int filtered_total = static_cast<int>(filtered_indices_.size());
        int filtered_reviewed = 0;
        for (int idx : filtered_indices_) {
            if (files_[idx].decision != "pending") {
                filtered_reviewed++;
            }
        }
        progress_bar_->setMaximum(filtered_total);
        progress_bar_->setValue(filtered_reviewed);
        progress_bar_->setFormat(QString("%1 / %2 assigned").arg(filtered_reviewed).arg(filtered_total));
    }
    
    // Requirement 16: Highlight the assigned folder when showing a file
    if (mind_map_view_) {
        int file_idx = get_current_file_index();
        if (file_idx >= 0 && file_idx < static_cast<int>(files_.size())) {
            const auto& file = files_[file_idx];
            if (file.decision == "move" && !file.destination_folder.isEmpty()) {
                mind_map_view_->set_selected_folder(file.destination_folder);
            } else {
                mind_map_view_->set_selected_folder(QString());
            }
        } else {
            mind_map_view_->set_selected_folder(QString());
        }
    }
}

void AdvancedFileTinderDialog::closeEvent(QCloseEvent* event) {
    // Route through reject() — don't delegate to QDialog::closeEvent
    event->ignore();
    reject();
}

void AdvancedFileTinderDialog::reject() {
    // Save advanced-mode-specific state before base class handles close prompt
    save_folder_tree();
    save_quick_access();
    
    // Base class reject() shows save prompt and terminates exec()
    StandaloneFileTinderDialog::reject();
}

bool AdvancedFileTinderDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == file_info_panel_ && event->type() == QEvent::MouseButtonDblClick) {
        int file_idx = get_current_file_index();
        if (file_idx >= 0 && file_idx < static_cast<int>(files_.size())) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(files_[file_idx].path));
        }
        return true;
    }
    return StandaloneFileTinderDialog::eventFilter(obj, event);
}

void AdvancedFileTinderDialog::on_finish() {
    save_folder_tree();
    save_quick_access();
    show_review_summary();
}

void AdvancedFileTinderDialog::on_undo() {
    if (undo_stack_.empty()) return;
    
    // Peek at the action we're about to undo to handle folder model
    const ActionRecord& last_action = undo_stack_.back();
    
    // If we're undoing a "move", unassign from the destination folder in the model
    if (last_action.new_decision == "move" && folder_model_) {
        int file_idx = last_action.file_index;
        if (file_idx >= 0 && file_idx < static_cast<int>(files_.size())) {
            const auto& file = files_[file_idx];
            if (!file.destination_folder.isEmpty()) {
                folder_model_->unassign_file_from_folder(file.destination_folder);
            }
        }
    }
    
    // Call base implementation to do the actual undo
    StandaloneFileTinderDialog::on_undo();
    
    // Requirement 15: Visual deselect after undo
    if (mind_map_view_) {
        // If the current file has a folder assignment, show it; otherwise clear selection
        int file_idx = get_current_file_index();
        if (file_idx >= 0 && file_idx < static_cast<int>(files_.size())) {
            const auto& file = files_[file_idx];
            if (file.decision == "move" && !file.destination_folder.isEmpty()) {
                mind_map_view_->set_selected_folder(file.destination_folder);
            } else {
                mind_map_view_->set_selected_folder(QString());  // Clear selection
            }
        } else {
            mind_map_view_->set_selected_folder(QString());
        }
    }
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
    
    // Arrow keys and letter shortcuts
    switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_D:
            on_delete();
            break;
        case Qt::Key_Right:
            // In Advanced Mode, right doesn't auto-keep, need to select folder
            break;
        case Qt::Key_K:
            on_keep();
            break;
        case Qt::Key_Z:
            on_undo();
            break;
        case Qt::Key_Down:
        case Qt::Key_S:
            on_skip();
            break;
        case Qt::Key_Up:
            on_back();
            break;
        case Qt::Key_N:
            prompt_add_folder();
            break;
        default:
            StandaloneFileTinderDialog::keyPressEvent(event);
    }
}

void AdvancedFileTinderDialog::on_filter_changed() {
    if (!filter_widget_) return;
    
    FileFilterType filter_type = filter_widget_->get_filter_type();
    
    // Prompt user about resetting progress when filter changes
    int reviewed = keep_count_ + delete_count_ + skip_count_ + move_count_;
    if (reviewed > 0) {
        auto reply = QMessageBox::question(this, "Filter Changed",
            QString("You have %1 decisions made. Do you want to reset progress?\n\n"
                    "• Yes — clear all decisions\n"
                    "• No — keep existing decisions").arg(reviewed),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            for (auto& file : files_) {
                if (file.decision == "move" && !file.destination_folder.isEmpty() && folder_model_) {
                    folder_model_->unassign_file_from_folder(file.destination_folder);
                }
                file.decision = "pending";
                file.destination_folder.clear();
            }
            keep_count_ = 0;
            delete_count_ = 0;
            skip_count_ = 0;
            move_count_ = 0;
            undo_stack_.clear();
            if (undo_btn_) undo_btn_->setEnabled(false);
            db_.clear_session(source_folder_);
        }
    }
    
    current_filter_ = filter_type;
    
    if (filter_type == FileFilterType::Custom) {
        custom_extensions_.clear();
        for (const QString& ext : filter_widget_->get_custom_extensions()) {
            custom_extensions_.append("." + ext);
        }
    }
    
    include_folders_ = filter_widget_->get_include_folders();
    
    rebuild_filtered_indices();
    
    // Find first pending file
    current_filtered_index_ = 0;
    for (size_t i = 0; i < filtered_indices_.size(); ++i) {
        if (files_[filtered_indices_[i]].decision == "pending") {
            current_filtered_index_ = static_cast<int>(i);
            break;
        }
    }
    
    if (!filtered_indices_.empty()) {
        show_current_file();
    }
    update_stats();
}

void AdvancedFileTinderDialog::on_sort_changed() {
    if (!filter_widget_) return;
    
    SortField sf = filter_widget_->get_sort_field();
    switch (sf) {
        case SortField::Name: sort_field_ = FileSortField::Name; break;
        case SortField::Size: sort_field_ = FileSortField::Size; break;
        case SortField::Type: sort_field_ = FileSortField::Type; break;
        case SortField::DateModified: sort_field_ = FileSortField::DateModified; break;
    }
    
    sort_order_ = filter_widget_->get_sort_order();
    
    apply_sort();
    rebuild_filtered_indices();
    if (!filtered_indices_.empty()) {
        show_current_file();
    }
}

void AdvancedFileTinderDialog::load_folder_tree() {
    if (folder_model_) {
        // Block signals during bulk load to prevent O(N) refresh_layout() calls
        // (each add_folder() emits folder_structure_changed → refresh_layout)
        folder_model_->blockSignals(true);
        folder_model_->load_from_database(db_, source_folder_);
        folder_model_->blockSignals(false);
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

void AdvancedFileTinderDialog::check_missing_folders() {
    if (!folder_model_ || !folder_model_->root_node()) return;
    
    // Collect non-virtual folders that no longer exist on disk
    QStringList missing;
    std::function<void(FolderNode*)> check = [&](FolderNode* node) {
        // Only check non-root, non-virtual folders
        if (node != folder_model_->root_node() && node->exists && !QDir(node->path).exists()) {
            missing.append(node->path);
        }
        for (const auto& child : node->children) {
            check(child.get());
        }
    };
    check(folder_model_->root_node());
    
    if (missing.isEmpty()) return;
    
    // Requirement 2: Warn user about missing folders
    QString msg = QString("%1 folder(s) no longer exist on disk:\n\n").arg(missing.size());
    for (const QString& path : missing) {
        msg += "• " + QFileInfo(path).fileName() + "\n";
    }
    msg += "\nDo you want to keep them as virtual folders (will be created during execution), "
           "or remove them from the tree?";
    
    auto reply = QMessageBox::question(this, "Missing Folders Detected", msg,
        QMessageBox::Yes | QMessageBox::No,  // Yes=keep as virtual, No=remove
        QMessageBox::Yes);
    
    if (reply == QMessageBox::Yes) {
        // Mark them as virtual (not existing)
        for (const QString& path : missing) {
            FolderNode* node = folder_model_->find_node(path);
            if (node) {
                node->exists = false;
            }
        }
    } else {
        // Remove them from tree, reverting any assigned files
        for (const QString& path : missing) {
            FolderNode* node = folder_model_->find_node(path);
            if (node && node->assigned_file_count > 0) {
                for (auto& file : files_) {
                    if (file.decision == "move" && file.destination_folder == path) {
                        file.decision = "pending";
                        file.destination_folder.clear();
                        move_count_--;
                    }
                }
            }
            folder_model_->remove_folder(path);
        }
    }
    
    if (mind_map_view_) mind_map_view_->refresh_layout();
}
