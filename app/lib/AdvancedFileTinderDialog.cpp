#include "AdvancedFileTinderDialog.hpp"
#include "FolderTreeModel.hpp"
#include "MindMapView.hpp"
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
#include <QTableWidget>
#include <QHeaderView>
#include <QGroupBox>
#include <QToolBar>
#include <QDir>

AdvancedFileTinderDialog::AdvancedFileTinderDialog(const QString& source_folder,
                                                   DatabaseManager& db,
                                                   QWidget* parent)
    : StandaloneFileTinderDialog(source_folder, db, parent)
    , main_splitter_(nullptr)
    , folder_panel_(nullptr)
    , mind_map_view_(nullptr)
    , quick_access_list_(nullptr)
    , folder_model_(nullptr) {
    
    setWindowTitle("File Tinder - Advanced Mode");
    setMinimumSize(ui::dimensions::kAdvancedFileTinderMinWidth,
                   ui::dimensions::kAdvancedFileTinderMinHeight);
    
    // Note: initialize() should be called after construction to complete setup
}

AdvancedFileTinderDialog::~AdvancedFileTinderDialog() = default;

void AdvancedFileTinderDialog::initialize() {
    // Call base class initialize which will call our overridden setup_ui()
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
    main_layout->setSpacing(10);
    
    // Title with mode toggle
    auto* title_bar = new QWidget();
    auto* title_layout = new QHBoxLayout(title_bar);
    title_layout->setContentsMargins(0, 0, 0, 0);
    
    auto* title_label = new QLabel("📁 FILE TINDER - Advanced Mode");
    title_label->setStyleSheet(QString(
        "font-size: %1px; font-weight: bold; color: %2;"
    ).arg(ui::fonts::kHeaderSize).arg(ui::colors::kNodeVirtualBg));
    title_layout->addWidget(title_label);
    
    title_layout->addStretch();
    
    switch_mode_btn_ = new QPushButton("🔄 Basic Mode");
    switch_mode_btn_->setStyleSheet(
        "QPushButton { padding: 5px 10px; background-color: #3498db; "
        "border-radius: 4px; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(switch_mode_btn_, &QPushButton::clicked, this, [this]() {
        save_folder_tree();
        emit switch_to_basic_mode();
    });
    title_layout->addWidget(switch_mode_btn_);
    
    main_layout->addWidget(title_bar);
    
    // Main splitter: preview | folder tree
    main_splitter_ = new QSplitter(Qt::Horizontal);
    
    // Left side: File preview panel
    auto* preview_panel = new QWidget();
    auto* preview_layout = new QVBoxLayout(preview_panel);
    preview_layout->setContentsMargins(5, 5, 5, 5);
    
    // Preview area
    auto* preview_group = new QGroupBox("File Preview");
    preview_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* preview_inner_layout = new QVBoxLayout(preview_group);
    
    preview_label_ = new QLabel();
    preview_label_->setMinimumSize(400, 350);
    preview_label_->setAlignment(Qt::AlignCenter);
    preview_label_->setStyleSheet(
        "background-color: #2c3e50; border-radius: 8px; padding: 10px;"
    );
    preview_inner_layout->addWidget(preview_label_);
    
    file_info_label_ = new QLabel();
    file_info_label_->setAlignment(Qt::AlignCenter);
    file_info_label_->setStyleSheet("color: #7f8c8d; padding: 5px;");
    preview_inner_layout->addWidget(file_info_label_);
    
    preview_layout->addWidget(preview_group, 1);
    
    // Progress
    auto* progress_widget = new QWidget();
    auto* progress_layout = new QVBoxLayout(progress_widget);
    progress_layout->setContentsMargins(0, 0, 0, 0);
    
    progress_bar_ = new QProgressBar();
    progress_bar_->setTextVisible(true);
    progress_bar_->setStyleSheet(
        "QProgressBar { border: 2px solid #34495e; border-radius: 5px; text-align: center; }"
        "QProgressBar::chunk { background-color: #9b59b6; }"
    );
    progress_layout->addWidget(progress_bar_);
    
    progress_label_ = new QLabel();
    progress_label_->setAlignment(Qt::AlignCenter);
    progress_layout->addWidget(progress_label_);
    
    preview_layout->addWidget(progress_widget);
    
    // Stats
    stats_label_ = new QLabel();
    stats_label_->setAlignment(Qt::AlignCenter);
    stats_label_->setStyleSheet(
        "font-size: 12px; padding: 8px; background-color: #34495e; border-radius: 5px;"
    );
    preview_layout->addWidget(stats_label_);
    
    // Basic action buttons
    auto* action_btns = new QWidget();
    auto* action_layout = new QHBoxLayout(action_btns);
    action_layout->setSpacing(8);
    
    delete_btn_ = new QPushButton("← Delete");
    delete_btn_->setMinimumSize(80, 40);
    delete_btn_->setStyleSheet(QString(
        "QPushButton { font-weight: bold; padding: 8px; "
        "background-color: %1; border-radius: 6px; color: white; }"
        "QPushButton:hover { background-color: #c0392b; }"
    ).arg(ui::colors::kDeleteColor));
    connect(delete_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_delete);
    action_layout->addWidget(delete_btn_);
    
    skip_btn_ = new QPushButton("↓ Skip");
    skip_btn_->setMinimumSize(80, 40);
    skip_btn_->setStyleSheet(QString(
        "QPushButton { font-weight: bold; padding: 8px; "
        "background-color: %1; border-radius: 6px; color: white; }"
        "QPushButton:hover { background-color: #e67e22; }"
    ).arg(ui::colors::kSkipColor));
    connect(skip_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_skip);
    action_layout->addWidget(skip_btn_);
    
    back_btn_ = new QPushButton("↑ Back");
    back_btn_->setMinimumSize(80, 40);
    back_btn_->setStyleSheet(
        "QPushButton { font-weight: bold; padding: 8px; "
        "background-color: #95a5a6; border-radius: 6px; }"
        "QPushButton:hover { background-color: #7f8c8d; }"
    );
    connect(back_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_back);
    action_layout->addWidget(back_btn_);
    
    preview_layout->addWidget(action_btns);
    
    main_splitter_->addWidget(preview_panel);
    
    // Right side: Folder tree panel
    setup_folder_panel();
    main_splitter_->addWidget(folder_panel_);
    
    // Set splitter sizes
    main_splitter_->setSizes({500, 600});
    main_splitter_->setStretchFactor(0, 1);
    main_splitter_->setStretchFactor(1, 2);
    
    main_layout->addWidget(main_splitter_, 1);
    
    // Quick access bar
    setup_quick_access_bar();
    
    // Bottom buttons
    setup_advanced_buttons();
    
    // Initialize folder model and view
    folder_model_ = new FolderTreeModel(this);
    folder_model_->set_root_folder(source_folder_);
    mind_map_view_->set_model(folder_model_);
    
    // Load saved folder tree
    load_folder_tree();
    
    // Update progress
    if (!files_.empty()) {
        progress_bar_->setMaximum(static_cast<int>(files_.size()));
        show_current_file();
    }
    
    update_quick_access();
    update_stats();
}

void AdvancedFileTinderDialog::setup_folder_panel() {
    folder_panel_ = new QWidget();
    auto* panel_layout = new QVBoxLayout(folder_panel_);
    panel_layout->setContentsMargins(5, 5, 5, 5);
    panel_layout->setSpacing(8);
    
    // Header with title and toolbar
    auto* header = new QWidget();
    auto* header_layout = new QHBoxLayout(header);
    header_layout->setContentsMargins(0, 0, 0, 0);
    
    auto* tree_label = new QLabel("📂 Destination Folders (Mind Map View)");
    tree_label->setStyleSheet("font-weight: bold; font-size: 14px;");
    header_layout->addWidget(tree_label);
    
    header_layout->addStretch();
    
    // Zoom controls
    auto* zoom_in_btn = new QToolButton();
    zoom_in_btn->setText("+");
    zoom_in_btn->setToolTip("Zoom In (Ctrl+Scroll)");
    zoom_in_btn->setStyleSheet("QToolButton { font-weight: bold; padding: 4px 8px; }");
    connect(zoom_in_btn, &QToolButton::clicked, this, &AdvancedFileTinderDialog::on_zoom_in);
    header_layout->addWidget(zoom_in_btn);
    
    auto* zoom_out_btn = new QToolButton();
    zoom_out_btn->setText("-");
    zoom_out_btn->setToolTip("Zoom Out");
    zoom_out_btn->setStyleSheet("QToolButton { font-weight: bold; padding: 4px 8px; }");
    connect(zoom_out_btn, &QToolButton::clicked, this, &AdvancedFileTinderDialog::on_zoom_out);
    header_layout->addWidget(zoom_out_btn);
    
    auto* zoom_fit_btn = new QToolButton();
    zoom_fit_btn->setText("⊡");
    zoom_fit_btn->setToolTip("Fit to View");
    zoom_fit_btn->setStyleSheet("QToolButton { font-weight: bold; padding: 4px 8px; }");
    connect(zoom_fit_btn, &QToolButton::clicked, this, &AdvancedFileTinderDialog::on_zoom_fit);
    header_layout->addWidget(zoom_fit_btn);
    
    panel_layout->addWidget(header);
    
    // Mind map view
    mind_map_view_ = new MindMapView();
    mind_map_view_->setMinimumSize(ui::dimensions::kFolderTreePanelMinWidth, 400);
    
    connect(mind_map_view_, &MindMapView::folder_clicked, 
            this, &AdvancedFileTinderDialog::on_folder_clicked);
    connect(mind_map_view_, &MindMapView::folder_double_clicked,
            this, &AdvancedFileTinderDialog::on_folder_double_clicked);
    connect(mind_map_view_, &MindMapView::folder_context_menu,
            this, &AdvancedFileTinderDialog::on_folder_context_menu);
    connect(mind_map_view_, &MindMapView::add_folder_requested,
            this, &AdvancedFileTinderDialog::on_add_folder);
    
    panel_layout->addWidget(mind_map_view_, 1);
    
    // Folder actions
    auto* folder_actions = new QWidget();
    auto* folder_actions_layout = new QHBoxLayout(folder_actions);
    folder_actions_layout->setContentsMargins(0, 0, 0, 0);
    
    auto* add_folder_btn = new QPushButton("+ Add Folder");
    add_folder_btn->setStyleSheet(
        "QPushButton { padding: 6px 12px; background-color: #27ae60; "
        "border-radius: 4px; color: white; }"
        "QPushButton:hover { background-color: #2ecc71; }"
    );
    connect(add_folder_btn, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_add_folder);
    folder_actions_layout->addWidget(add_folder_btn);
    
    auto* add_subfolder_btn = new QPushButton("+ Add Subfolder");
    add_subfolder_btn->setStyleSheet(
        "QPushButton { padding: 6px 12px; background-color: #3498db; "
        "border-radius: 4px; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(add_subfolder_btn, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_add_subfolder);
    folder_actions_layout->addWidget(add_subfolder_btn);
    
    folder_actions_layout->addStretch();
    
    panel_layout->addWidget(folder_actions);
    
    // Instruction label
    auto* hint_label = new QLabel(
        "💡 Click a folder node to move the current file there.\n"
        "   Ctrl+Scroll to zoom. Middle-click to pan."
    );
    hint_label->setStyleSheet("color: #7f8c8d; font-size: 11px; padding: 5px;");
    panel_layout->addWidget(hint_label);
}

void AdvancedFileTinderDialog::setup_quick_access_bar() {
    auto* quick_group = new QGroupBox("⭐ Quick Access");
    quick_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    quick_group->setMaximumHeight(ui::dimensions::kQuickAccessBarHeight + 30);
    
    auto* quick_layout = new QHBoxLayout(quick_group);
    
    quick_access_list_ = new QListWidget();
    quick_access_list_->setFlow(QListView::LeftToRight);
    quick_access_list_->setMaximumHeight(ui::dimensions::kQuickAccessBarHeight);
    quick_access_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    quick_access_list_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    quick_access_list_->setStyleSheet(
        "QListWidget { background-color: #34495e; border-radius: 6px; }"
        "QListWidget::item { padding: 8px 15px; margin: 3px; background-color: #3498db; "
        "border-radius: 4px; color: white; }"
        "QListWidget::item:hover { background-color: #2980b9; }"
        "QListWidget::item:selected { background-color: #27ae60; }"
    );
    
    connect(quick_access_list_, &QListWidget::itemClicked,
            this, &AdvancedFileTinderDialog::on_quick_access_clicked);
    
    quick_layout->addWidget(quick_access_list_);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(quick_group);
}

void AdvancedFileTinderDialog::setup_advanced_buttons() {
    auto* bottom_btns = new QWidget();
    auto* btn_layout = new QHBoxLayout(bottom_btns);
    
    auto* cancel_btn = new QPushButton("Cancel Session");
    cancel_btn->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #95a5a6; "
        "border-radius: 5px; }"
        "QPushButton:hover { background-color: #7f8c8d; }"
    );
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
    btn_layout->addWidget(cancel_btn);
    
    btn_layout->addStretch();
    
    finish_btn_ = new QPushButton("✓ Execute All Moves");
    finish_btn_->setStyleSheet(
        "QPushButton { padding: 10px 25px; background-color: #2ecc71; "
        "border-radius: 5px; color: white; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #27ae60; }"
    );
    connect(finish_btn_, &QPushButton::clicked, this, &AdvancedFileTinderDialog::on_finish);
    btn_layout->addWidget(finish_btn_);
    
    static_cast<QVBoxLayout*>(layout())->addWidget(bottom_btns);
}

void AdvancedFileTinderDialog::on_folder_clicked(const QString& folder_path) {
    // Move current file to this folder
    int file_idx = get_current_file_index();
    if (file_idx < 0) return;
    
    auto& file = files_[file_idx];
    
    // Update counts using helper method
    if (file.decision != "pending") {
        update_decision_count(file.decision, -1);
        if (file.decision == "move" && !file.destination_folder.isEmpty()) {
            folder_model_->unassign_file_from_folder(file.destination_folder);
        }
    }
    
    file.decision = "move";
    file.destination_folder = folder_path;
    move_count_++;
    
    folder_model_->assign_file_to_folder(folder_path);
    mind_map_view_->set_selected_folder(folder_path);
    
    db_.add_recent_folder(folder_path);
    update_quick_access();
    
    animate_swipe(true);
    advance_to_next();
}

void AdvancedFileTinderDialog::on_folder_double_clicked(const QString& folder_path) {
    // Toggle expand/collapse could be implemented
    Q_UNUSED(folder_path);
    mind_map_view_->refresh_layout();
}

void AdvancedFileTinderDialog::on_folder_context_menu(const QString& folder_path, const QPoint& pos) {
    QMenu menu;
    
    menu.addAction("📁 Move File Here", [this, folder_path]() {
        on_folder_clicked(folder_path);
    });
    
    menu.addSeparator();
    
    menu.addAction("⭐ Pin to Quick Access", [this, folder_path]() {
        folder_model_->set_folder_pinned(folder_path, true);
        FolderTreeEntry entry;
        entry.folder_path = folder_path;
        entry.is_pinned = true;
        db_.update_folder_pinned(source_folder_, folder_path, true);
        update_quick_access();
    });
    
    menu.addAction("➕ Add Subfolder", [this, folder_path]() {
        bool ok;
        QString name = QInputDialog::getText(this, "New Subfolder",
                                             "Enter folder name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            QString new_path = folder_path + "/" + name;
            folder_model_->add_folder(new_path, true);
            mind_map_view_->refresh_layout();
        }
    });
    
    menu.addSeparator();
    
    FolderNode* node = folder_model_->find_node(folder_path);
    if (node && !node->exists) {
        menu.addAction("🗑️ Remove Virtual Folder", [this, folder_path]() {
            folder_model_->remove_folder(folder_path);
            mind_map_view_->refresh_layout();
        });
    }
    
    menu.exec(pos);
}

void AdvancedFileTinderDialog::on_add_folder() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Folder",
                                         "Enter folder name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        // Create in source folder
        QString new_path = source_folder_ + "/" + name;
        folder_model_->add_folder(new_path, true);
        mind_map_view_->refresh_layout();
    }
}

void AdvancedFileTinderDialog::on_add_subfolder() {
    QString parent = QFileDialog::getExistingDirectory(this, "Select Parent Folder", source_folder_);
    if (parent.isEmpty()) return;
    
    bool ok;
    QString name = QInputDialog::getText(this, "New Subfolder",
                                         "Enter folder name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        QString new_path = parent + "/" + name;
        folder_model_->add_folder(new_path, true);
        mind_map_view_->refresh_layout();
    }
}

void AdvancedFileTinderDialog::on_remove_folder() {
    // Would need a selected folder path
}

void AdvancedFileTinderDialog::on_pin_folder() {
    // Would need a selected folder path
}

void AdvancedFileTinderDialog::on_connect_folders() {
    // Could implement multi-select connection
}

void AdvancedFileTinderDialog::update_quick_access() {
    quick_access_list_->clear();
    
    // Add pinned folders
    QStringList pinned = folder_model_->get_pinned_folders();
    for (const QString& path : pinned) {
        auto* item = new QListWidgetItem("⭐ " + QFileInfo(path).fileName());
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        quick_access_list_->addItem(item);
    }
    
    // Add recent folders
    QStringList recent = db_.get_recent_folders(5);
    for (const QString& path : recent) {
        if (!pinned.contains(path)) {
            auto* item = new QListWidgetItem(QFileInfo(path).fileName());
            item->setData(Qt::UserRole, path);
            item->setToolTip(path);
            quick_access_list_->addItem(item);
        }
    }
}

void AdvancedFileTinderDialog::on_quick_access_clicked(QListWidgetItem* item) {
    QString folder_path = item->data(Qt::UserRole).toString();
    if (!folder_path.isEmpty()) {
        on_folder_clicked(folder_path);
    }
}

void AdvancedFileTinderDialog::on_move_to_folder() {
    // In advanced mode, we use the folder tree
    // Show folder picker as fallback
    QString folder = show_folder_picker();
    if (!folder.isEmpty()) {
        on_folder_clicked(folder);
    }
}

void AdvancedFileTinderDialog::on_finish() {
    save_folder_tree();
    show_review_summary();
}

void AdvancedFileTinderDialog::keyPressEvent(QKeyEvent* event) {
    // Number keys 1-9 for quick folder access
    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        int index = event->key() - Qt::Key_1;
        if (index < quick_access_list_->count()) {
            auto* item = quick_access_list_->item(index);
            on_quick_access_clicked(item);
            return;
        }
    }
    
    switch (event->key()) {
        case Qt::Key_D:
            on_delete();
            break;
        case Qt::Key_S:
            on_skip();
            break;
        case Qt::Key_N:
            on_add_folder();
            break;
        case Qt::Key_F:
            on_toggle_folder_panel();
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
        default:
            StandaloneFileTinderDialog::keyPressEvent(event);
    }
}

void AdvancedFileTinderDialog::on_toggle_folder_panel() {
    if (folder_panel_->isVisible()) {
        folder_panel_->hide();
    } else {
        folder_panel_->show();
    }
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
    folder_model_->load_from_database(db_, source_folder_);
    mind_map_view_->refresh_layout();
    update_quick_access();
}

void AdvancedFileTinderDialog::save_folder_tree() {
    folder_model_->save_to_database(db_, source_folder_);
}
