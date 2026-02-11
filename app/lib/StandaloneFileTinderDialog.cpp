#include "StandaloneFileTinderDialog.hpp"
#include "DatabaseManager.hpp"
#include "FileTinderExecutor.hpp"
#include "AppLogger.hpp"
#include "ImagePreviewWindow.hpp"
#include "ui_constants.hpp"
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QPixmap>
#include <QMimeDatabase>
#include <QTextStream>
#include <QScrollArea>
#include <QGroupBox>
#include <QProgressDialog>
#include <QInputDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QApplication>
#include <QSettings>
#include <QSplitter>
#include <QPointer>
#include <QSet>
#include <algorithm>

StandaloneFileTinderDialog::StandaloneFileTinderDialog(const QString& source_folder,
                                                       DatabaseManager& db,
                                                       QWidget* parent)
    : QDialog(parent)
    , current_filtered_index_(0)
    , source_folder_(source_folder)
    , db_(db)
    , current_filter_(FileFilterType::All)
    , sort_field_(FileSortField::Name)
    , sort_order_(SortOrder::Ascending)
    , include_folders_(false)
    , keep_count_(0)
    , delete_count_(0)
    , skip_count_(0)
    , move_count_(0)
    , image_preview_window_(nullptr)
    , preview_label_(nullptr)
    , file_info_label_(nullptr)
    , file_icon_label_(nullptr)
    , progress_label_(nullptr)
    , stats_label_(nullptr)
    , progress_bar_(nullptr)
    , filter_combo_(nullptr)
    , sort_combo_(nullptr)
    , sort_order_btn_(nullptr)
    , folders_checkbox_(nullptr)
    , shortcuts_label_(nullptr)
    , back_btn_(nullptr)
    , delete_btn_(nullptr)
    , skip_btn_(nullptr)
    , keep_btn_(nullptr)
    , undo_btn_(nullptr)
    , preview_btn_(nullptr)
    , finish_btn_(nullptr)
    , switch_mode_btn_(nullptr)
    , help_btn_(nullptr)
    , swipe_animation_(nullptr)
    , resize_timer_(nullptr) {
    
    setWindowTitle("File Tinder - Basic Mode");
    
    // DPI-aware minimum size
    setMinimumSize(ui::scaling::scaled(ui::dimensions::kStandaloneFileTinderMinWidth),
                   ui::scaling::scaled(ui::dimensions::kStandaloneFileTinderMinHeight));
    
    // Setup resize timer for debouncing preview updates
    resize_timer_ = new QTimer(this);
    resize_timer_->setSingleShot(true);
    resize_timer_->setInterval(150);  // 150ms debounce
    connect(resize_timer_, &QTimer::timeout, this, [this]() {
        int file_idx = get_current_file_index();
        if (file_idx >= 0 && file_idx < static_cast<int>(files_.size())) {
            update_preview(files_[file_idx].path);
        }
    });
    
    // Don't call setup_ui() here - it's virtual and could cause issues
    // with derived classes. Call initialize() after construction instead.
}

StandaloneFileTinderDialog::~StandaloneFileTinderDialog() = default;

void StandaloneFileTinderDialog::initialize() {
    setup_ui();
    scan_files();
    apply_sort();  // Sort files after loading
    rebuild_filtered_indices();
    load_session_state();
    
    if (!filtered_indices_.empty()) {
        show_current_file();
    }
    
    // Save this folder as last used
    save_last_folder();
}

void StandaloneFileTinderDialog::save_last_folder() {
    QSettings settings("FileTinder", "FileTinder");
    settings.setValue("lastFolder", source_folder_);
}

QString StandaloneFileTinderDialog::get_last_folder() {
    QSettings settings("FileTinder", "FileTinder");
    return settings.value("lastFolder", QDir::homePath()).toString();
}

void StandaloneFileTinderDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(15, 15, 15, 15);
    main_layout->setSpacing(10);
    
    // Top bar: Title + Mode switch (upper right)
    auto* top_bar = new QWidget();
    auto* top_layout = new QHBoxLayout(top_bar);
    top_layout->setContentsMargins(0, 0, 0, 0);
    
    auto* title_label = new QLabel("File Tinder - Basic Mode");
    title_label->setStyleSheet(QString(
        "font-size: %1px; font-weight: bold; color: %2;"
    ).arg(ui::fonts::kHeaderSize).arg(ui::colors::kMoveColor));
    top_layout->addWidget(title_label);
    
    top_layout->addStretch();
    
    // Mode switch in upper right corner
    switch_mode_btn_ = new QPushButton("Advanced Mode");
    switch_mode_btn_->setFixedSize(ui::scaling::scaled(130), ui::scaling::scaled(32));
    switch_mode_btn_->setStyleSheet(
        "QPushButton { font-size: 11px; padding: 5px 10px; "
        "background-color: #9b59b6; border-radius: 4px; color: white; }"
        "QPushButton:hover { background-color: #8e44ad; }"
    );
    connect(switch_mode_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_switch_mode_clicked);
    top_layout->addWidget(switch_mode_btn_);
    
    // Help button
    help_btn_ = new QPushButton("?");
    help_btn_->setFixedSize(ui::scaling::scaled(32), ui::scaling::scaled(32));
    help_btn_->setToolTip("Keyboard Shortcuts");
    help_btn_->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: bold; background-color: #34495e; "
        "border-radius: 16px; color: white; }"
        "QPushButton:hover { background-color: #3d566e; }"
    );
    connect(help_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::show_shortcuts_help);
    top_layout->addWidget(help_btn_);
    
    main_layout->addWidget(top_bar);
    
    // Filter/Sort bar
    auto* filter_bar = new QWidget();
    auto* filter_layout = new QHBoxLayout(filter_bar);
    filter_layout->setContentsMargins(0, 5, 0, 5);
    filter_layout->setSpacing(10);
    
    // Filter
    filter_layout->addWidget(new QLabel("Filter:"));
    filter_combo_ = new QComboBox();
    filter_combo_->addItem("All Files", static_cast<int>(FileFilterType::All));
    filter_combo_->addItem("Images", static_cast<int>(FileFilterType::Images));
    filter_combo_->addItem("Videos", static_cast<int>(FileFilterType::Videos));
    filter_combo_->addItem("Audio", static_cast<int>(FileFilterType::Audio));
    filter_combo_->addItem("Documents", static_cast<int>(FileFilterType::Documents));
    filter_combo_->addItem("Archives", static_cast<int>(FileFilterType::Archives));
    filter_combo_->addItem("Other", static_cast<int>(FileFilterType::Other));
    filter_combo_->addItem("Folders Only", static_cast<int>(FileFilterType::FoldersOnly));
    filter_combo_->addItem("Specify...", static_cast<int>(FileFilterType::Custom));
    filter_combo_->setMinimumWidth(120);
    filter_combo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; background-color: #34495e; "
        "border-radius: 4px; color: white; }"
        "QComboBox:hover { background-color: #3d566e; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox:focus { border: 2px solid #3498db; }"
    );
    connect(filter_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StandaloneFileTinderDialog::on_filter_changed);
    filter_layout->addWidget(filter_combo_);
    
    // Include folders checkbox
    folders_checkbox_ = new QCheckBox("Include Folders");
    folders_checkbox_->setStyleSheet("color: #bdc3c7;");
    connect(folders_checkbox_, &QCheckBox::stateChanged, 
            this, &StandaloneFileTinderDialog::on_folders_toggle_changed);
    filter_layout->addWidget(folders_checkbox_);
    
    filter_layout->addSpacing(20);
    
    // Sort
    filter_layout->addWidget(new QLabel("Sort:"));
    sort_combo_ = new QComboBox();
    sort_combo_->addItem("Name", static_cast<int>(FileSortField::Name));
    sort_combo_->addItem("Size", static_cast<int>(FileSortField::Size));
    sort_combo_->addItem("Type", static_cast<int>(FileSortField::Type));
    sort_combo_->addItem("Date Modified", static_cast<int>(FileSortField::DateModified));
    sort_combo_->setMinimumWidth(100);
    sort_combo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; background-color: #34495e; "
        "border-radius: 4px; color: white; }"
        "QComboBox:hover { background-color: #3d566e; }"
        "QComboBox::drop-down { border: none; }"
    );
    connect(sort_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StandaloneFileTinderDialog::on_sort_changed);
    filter_layout->addWidget(sort_combo_);
    
    // Sort order toggle button
    sort_order_btn_ = new QPushButton("Asc");
    sort_order_btn_->setFixedSize(ui::scaling::scaled(50), ui::scaling::scaled(28));
    sort_order_btn_->setStyleSheet(
        "QPushButton { padding: 4px; background-color: #34495e; "
        "border-radius: 4px; color: white; font-size: 11px; }"
        "QPushButton:hover { background-color: #3d566e; }"
    );
    connect(sort_order_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_sort_order_toggled);
    filter_layout->addWidget(sort_order_btn_);
    
    filter_layout->addStretch();
    main_layout->addWidget(filter_bar);
    
    // Preview area with centered icon
    auto* preview_widget = new QWidget();
    preview_widget->setStyleSheet("background-color: #2c3e50; border-radius: 8px;");
    auto* preview_layout = new QVBoxLayout(preview_widget);
    preview_layout->setContentsMargins(15, 15, 15, 15);
    
    // Centered file icon (for non-image files)
    file_icon_label_ = new QLabel();
    file_icon_label_->setAlignment(Qt::AlignCenter);
    file_icon_label_->setMinimumHeight(80);
    file_icon_label_->setStyleSheet("font-size: 64px;");
    preview_layout->addWidget(file_icon_label_);
    
    // Preview label (for images/text)
    preview_label_ = new QLabel();
    preview_label_->setMinimumSize(ui::scaling::scaled(300), ui::scaling::scaled(200));
    preview_label_->setAlignment(Qt::AlignCenter);
    preview_label_->setWordWrap(true);
    preview_layout->addWidget(preview_label_, 1);
    
    // File name and info below preview
    file_info_label_ = new QLabel();
    file_info_label_->setAlignment(Qt::AlignCenter);
    file_info_label_->setStyleSheet("color: #ecf0f1; padding: 10px; font-size: 13px;");
    file_info_label_->setWordWrap(true);
    preview_layout->addWidget(file_info_label_);
    
    main_layout->addWidget(preview_widget, 1);  // Stretch to fill space
    
    // Progress section
    auto* progress_widget = new QWidget();
    auto* progress_vlayout = new QVBoxLayout(progress_widget);
    progress_vlayout->setContentsMargins(0, 5, 0, 5);
    progress_vlayout->setSpacing(5);
    
    progress_bar_ = new QProgressBar();
    progress_bar_->setTextVisible(true);
    progress_bar_->setFixedHeight(ui::scaling::scaled(20));
    progress_bar_->setStyleSheet(
        "QProgressBar { border: 1px solid #34495e; border-radius: 4px; text-align: center; background: #2c3e50; }"
        "QProgressBar::chunk { background-color: #3498db; }"
    );
    progress_vlayout->addWidget(progress_bar_);
    
    progress_label_ = new QLabel();
    progress_label_->setAlignment(Qt::AlignCenter);
    progress_label_->setStyleSheet("color: #bdc3c7; font-size: 11px;");
    progress_vlayout->addWidget(progress_label_);
    
    main_layout->addWidget(progress_widget);
    
    // Stats bar
    stats_label_ = new QLabel();
    stats_label_->setAlignment(Qt::AlignCenter);
    stats_label_->setStyleSheet(
        "font-size: 12px; padding: 8px; background-color: #34495e; border-radius: 4px;"
    );
    main_layout->addWidget(stats_label_);
    
    // ============ ACTION BUTTONS SECTION ============
    // New layout: Keep/Delete SIDE BY SIDE (large), Back/Skip below (thin)
    
    auto* action_widget = new QWidget();
    auto* action_layout = new QVBoxLayout(action_widget);
    action_layout->setSpacing(10);
    action_layout->setContentsMargins(20, 10, 20, 10);
    
    // Row 1: KEEP and DELETE side by side (large buttons)
    auto* main_btn_row = new QHBoxLayout();
    main_btn_row->setSpacing(20);
    
    delete_btn_ = new QPushButton("DELETE\n[Left]");
    delete_btn_->setMinimumSize(ui::scaling::scaled(ui::dimensions::kMainButtonWidth),
                                ui::scaling::scaled(ui::dimensions::kMainButtonHeight));
    delete_btn_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    delete_btn_->setStyleSheet(QString(
        "QPushButton { font-size: 18px; font-weight: bold; "
        "background-color: %1; border: 2px solid #c0392b; color: white; border-radius: 8px; }"
        "QPushButton:hover { background-color: #c0392b; }"
    ).arg(ui::colors::kDeleteColor));
    connect(delete_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_delete);
    main_btn_row->addWidget(delete_btn_);
    
    keep_btn_ = new QPushButton("KEEP\n[Right]");
    keep_btn_->setMinimumSize(ui::scaling::scaled(ui::dimensions::kMainButtonWidth),
                              ui::scaling::scaled(ui::dimensions::kMainButtonHeight));
    keep_btn_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    keep_btn_->setStyleSheet(QString(
        "QPushButton { font-size: 18px; font-weight: bold; "
        "background-color: %1; border: 2px solid #27ae60; color: white; border-radius: 8px; }"
        "QPushButton:hover { background-color: #27ae60; }"
    ).arg(ui::colors::kKeepColor));
    connect(keep_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_keep);
    main_btn_row->addWidget(keep_btn_);
    
    action_layout->addLayout(main_btn_row);
    
    // Row 2: BACK and SKIP (thinner buttons)
    auto* nav_btn_row = new QHBoxLayout();
    nav_btn_row->setSpacing(20);
    
    back_btn_ = new QPushButton("Back [Up]");
    back_btn_->setFixedHeight(ui::scaling::scaled(40));
    back_btn_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    back_btn_->setStyleSheet(
        "QPushButton { font-size: 12px; font-weight: bold; "
        "background-color: #7f8c8d; border: 1px solid #6c7a7d; color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: #95a5a6; }"
    );
    connect(back_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_back);
    nav_btn_row->addWidget(back_btn_);
    
    skip_btn_ = new QPushButton("Skip [Down]");
    skip_btn_->setFixedHeight(ui::scaling::scaled(40));
    skip_btn_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    skip_btn_->setStyleSheet(QString(
        "QPushButton { font-size: 12px; font-weight: bold; "
        "background-color: %1; border: 1px solid #d68910; color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: #e67e22; }"
    ).arg(ui::colors::kSkipColor));
    connect(skip_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_skip);
    nav_btn_row->addWidget(skip_btn_);
    
    action_layout->addLayout(nav_btn_row);
    
    main_layout->addWidget(action_widget);
    
    // Bottom bar: Undo, Preview, and Finish buttons
    auto* bottom_bar = new QWidget();
    auto* bottom_layout = new QHBoxLayout(bottom_bar);
    bottom_layout->setContentsMargins(0, 5, 0, 0);
    bottom_layout->setSpacing(15);
    
    // Undo button (replaces Move to Folder in Basic Mode)
    undo_btn_ = new QPushButton("Undo [Z]");
    undo_btn_->setFixedHeight(ui::scaling::scaled(36));
    undo_btn_->setStyleSheet(
        "QPushButton { font-size: 12px; padding: 8px 15px; "
        "background-color: #9b59b6; border-radius: 4px; color: white; }"
        "QPushButton:hover { background-color: #8e44ad; }"
        "QPushButton:disabled { background-color: #5d4e6e; color: #888; }"
    );
    undo_btn_->setEnabled(false);  // Disabled until there's something to undo
    connect(undo_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_undo);
    bottom_layout->addWidget(undo_btn_);
    
    // Preview button (for opening images in separate window)
    preview_btn_ = new QPushButton("Preview [P]");
    preview_btn_->setFixedHeight(ui::scaling::scaled(36));
    preview_btn_->setStyleSheet(QString(
        "QPushButton { font-size: 12px; padding: 8px 15px; "
        "background-color: %1; border-radius: 4px; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
    ).arg(ui::colors::kMoveColor));
    connect(preview_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_show_preview);
    bottom_layout->addWidget(preview_btn_);
    
    bottom_layout->addStretch();
    
    finish_btn_ = new QPushButton("Finish Review");
    finish_btn_->setFixedHeight(ui::scaling::scaled(36));
    finish_btn_->setStyleSheet(
        "QPushButton { font-size: 12px; padding: 8px 15px; "
        "background-color: #1abc9c; border-radius: 4px; color: white; }"
        "QPushButton:hover { background-color: #16a085; }"
    );
    connect(finish_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_finish);
    bottom_layout->addWidget(finish_btn_);
    
    main_layout->addWidget(bottom_bar);
    
    // Keyboard shortcuts hint
    shortcuts_label_ = new QLabel("Keys: Right=Keep | Left=Delete | Down=Skip | Up=Back | Z=Undo | P=Preview | ?=Help");
    shortcuts_label_->setAlignment(Qt::AlignCenter);
    shortcuts_label_->setStyleSheet("color: #7f8c8d; font-size: 10px;");
    main_layout->addWidget(shortcuts_label_);
    
    update_stats();
}

void StandaloneFileTinderDialog::scan_files() {
    files_.clear();
    QMimeDatabase mime_db;
    
    QDir dir(source_folder_);
    if (!dir.exists()) {
        LOG_ERROR("BasicMode", QString("Source folder does not exist: %1").arg(source_folder_));
        QMessageBox::warning(this, "Error", "Source folder does not exist.");
        return;
    }
    
    // Include both files and directories based on settings
    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot;
    if (include_folders_) {
        filters |= QDir::Dirs;
    }
    
    QStringList entries = dir.entryList(filters);
    
    for (const QString& entry : entries) {
        try {
            QString full_path = dir.absoluteFilePath(entry);
            QFileInfo info(full_path);
            QMimeType mime_type = mime_db.mimeTypeForFile(full_path);
            
            FileToProcess file;
            file.path = full_path;
            file.name = info.fileName();
            file.extension = info.suffix().toLower();
            file.size = info.isDir() ? 0 : info.size();
            file.modified_datetime = info.lastModified();
            file.modified_date = file.modified_datetime.toString("MMM d, yyyy HH:mm");
            file.decision = "pending";
            file.mime_type = mime_type.name();
            file.is_directory = info.isDir();
            
            files_.push_back(file);
        } catch (const std::exception& ex) {
            LOG_ERROR("BasicMode", QString("Error scanning file %1: %2").arg(entry, ex.what()));
        }
    }
    
    LOG_INFO("BasicMode", QString("Scanned %1 files from %2").arg(files_.size()).arg(source_folder_));
    
    // Update progress
    if (!files_.empty() && progress_bar_) {
        progress_bar_->setMaximum(static_cast<int>(files_.size()));
        progress_bar_->setValue(0);
    }
}

void StandaloneFileTinderDialog::load_session_state() {
    auto decisions = db_.get_session_decisions(source_folder_);
    
    for (const auto& decision : decisions) {
        for (auto& file : files_) {
            if (file.path == decision.file_path) {
                file.decision = decision.decision;
                file.destination_folder = decision.destination_folder;
                
                if (decision.decision == "keep") keep_count_++;
                else if (decision.decision == "delete") delete_count_++;
                else if (decision.decision == "skip") skip_count_++;
                else if (decision.decision == "move") move_count_++;
                
                break;
            }
        }
    }
    
    // Find first pending file in filtered list
    current_filtered_index_ = 0;
    for (size_t i = 0; i < filtered_indices_.size(); ++i) {
        if (files_[filtered_indices_[i]].decision == "pending") {
            current_filtered_index_ = static_cast<int>(i);
            break;
        }
    }
    
    update_progress();
    update_stats();
}

void StandaloneFileTinderDialog::save_session_state() {
    for (const auto& file : files_) {
        if (file.decision != "pending") {
            db_.save_file_decision(source_folder_, file.path, file.decision, file.destination_folder);
        }
    }
}

void StandaloneFileTinderDialog::show_current_file() {
    int file_idx = get_current_file_index();
    if (file_idx < 0 || file_idx >= static_cast<int>(files_.size())) {
        if (file_icon_label_) file_icon_label_->clear();
        if (preview_label_) preview_label_->setText("No more files to review");
        if (file_info_label_) file_info_label_->setText("");
        return;
    }
    
    const auto& file = files_[file_idx];
    update_preview(file.path);
    update_file_info(file);
    update_progress();
}

void StandaloneFileTinderDialog::update_preview(const QString& file_path) {
    if (!preview_label_) return;
    
    QFileInfo finfo(file_path);
    QMimeDatabase mime_db;
    QMimeType mime_type = mime_db.mimeTypeForFile(file_path);
    QString type = mime_type.name();
    
    // Clear previous content
    if (file_icon_label_) file_icon_label_->clear();
    preview_label_->clear();
    // Reset style from any previous text file preview
    preview_label_->setStyleSheet("");
    
    // Determine icon for the file type (always shown centered)
    QString icon = "[FILE]";
    if (finfo.isDir()) {
        icon = "[DIR]";
    } else if (type.startsWith("image/")) {
        icon = "[IMG]";
    } else if (type.startsWith("video/")) {
        icon = "[VID]";
    } else if (type.startsWith("audio/")) {
        icon = "[AUD]";
    } else if (type.contains("pdf")) {
        icon = "[PDF]";
    } else if (type.contains("zip") || type.contains("archive") || type.contains("compressed")) {
        icon = "[ZIP]";
    } else if (type.contains("spreadsheet") || type.contains("excel")) {
        icon = "[XLS]";
    } else if (type.contains("document") || type.contains("word")) {
        icon = "[DOC]";
    } else if (type.startsWith("text/")) {
        icon = "[TXT]";
    }
    
    // Set the centered icon
    if (file_icon_label_) {
        file_icon_label_->setText(QString("<span style='font-family: monospace; font-size: 48px; "
                                          "color: #3498db; font-weight: bold;'>%1</span>").arg(icon));
    }
    
    // For images, show preview (but not a separate window)
    if (type.startsWith("image/") && !finfo.isDir()) {
        QPixmap pixmap(file_path);
        if (!pixmap.isNull()) {
            // Scale to fit the preview area
            int max_w = preview_label_->width() > 100 ? preview_label_->width() - 20 : 400;
            int max_h = preview_label_->height() > 100 ? preview_label_->height() - 20 : 300;
            pixmap = pixmap.scaled(max_w, max_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            preview_label_->setPixmap(pixmap);
            return;
        }
    }
    
    // For text files, show content preview
    if (type.startsWith("text/") && !finfo.isDir()) {
        QFile file(file_path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QString content = stream.read(1500);  // Reduced to avoid overflow
            if (!stream.atEnd()) {
                content += "\n...(truncated)";
            }
            preview_label_->setText(content);
            preview_label_->setStyleSheet("color: #ecf0f1; font-family: monospace; font-size: 11px;");
            return;
        }
    }
    
    // For directories
    if (finfo.isDir()) {
        QDir dir(file_path);
        int file_count = dir.entryList(QDir::Files | QDir::NoDotAndDotDot).count();
        int dir_count = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).count();
        preview_label_->setText(QString("Directory contains:\n%1 files\n%2 subdirectories")
                               .arg(file_count).arg(dir_count));
        return;
    }
    
    // Default: show MIME type info
    preview_label_->setText(QString("File Type: %1\n\nNo preview available").arg(mime_type.comment()));
}

void StandaloneFileTinderDialog::update_file_info(const FileToProcess& file) {
    QString size_str;
    if (file.is_directory) {
        size_str = "Directory";
    } else if (file.size < 1024) {
        size_str = QString("%1 B").arg(file.size);
    } else if (file.size < 1024 * 1024) {
        size_str = QString("%1 KB").arg(file.size / 1024.0, 0, 'f', 1);
    } else if (file.size < 1024LL * 1024 * 1024) {
        size_str = QString("%1 MB").arg(file.size / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        size_str = QString("%1 GB").arg(file.size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
    
    QString type_str = file.is_directory ? "Folder" : 
                       (file.extension.isEmpty() ? "Unknown" : file.extension.toUpper());
    
    if (!file_info_label_) return;
    file_info_label_->setText(QString("<b style='font-size: 14px;'>%1</b><br>"
                                      "<span style='color: #95a5a6;'>%2 | %3 | %4</span>")
                              .arg(file.name, size_str, type_str, file.modified_date));
}

void StandaloneFileTinderDialog::update_progress() {
    int reviewed = keep_count_ + delete_count_ + skip_count_ + move_count_;
    int total = static_cast<int>(files_.size());
    int filtered_total = static_cast<int>(filtered_indices_.size());
    
    if (progress_bar_) progress_bar_->setValue(reviewed);
    
    int percent = total > 0 ? (reviewed * 100 / total) : 0;
    QString filter_info = (current_filter_ != FileFilterType::All) 
        ? QString(" (showing %1 of %2)").arg(filtered_total).arg(total)
        : "";
    if (progress_label_) {
        progress_label_->setText(QString("Progress: %1 / %2 files (%3%)%4")
                                 .arg(reviewed).arg(total).arg(percent).arg(filter_info));
    }
}

void StandaloneFileTinderDialog::update_stats() {
    if (!stats_label_) return;
    
    QString stats = QString(
        "<span style='color: %1;'>✓ Keep: %2</span>  |  "
        "<span style='color: %3;'>✗ Delete: %4</span>  |  "
        "<span style='color: %5;'>↓ Skip: %6</span>"
    ).arg(ui::colors::kKeepColor).arg(keep_count_)
     .arg(ui::colors::kDeleteColor).arg(delete_count_)
     .arg(ui::colors::kSkipColor).arg(skip_count_);
    
    // Only show Move count if there are moves (relevant in Advanced Mode)
    if (move_count_ > 0) {
        stats += QString("  |  <span style='color: %1;'>📁 Move: %2</span>")
            .arg(ui::colors::kMoveColor).arg(move_count_);
    }
    
    stats_label_->setText(stats);
}

// Helper to update decision counts (deduplication of count logic)
void StandaloneFileTinderDialog::update_decision_count(const QString& old_decision, int delta) {
    if (old_decision == "keep") keep_count_ += delta;
    else if (old_decision == "delete") delete_count_ += delta;
    else if (old_decision == "skip") skip_count_ += delta;
    else if (old_decision == "move") move_count_ += delta;
}

int StandaloneFileTinderDialog::get_current_file_index() const {
    if (current_filtered_index_ < 0 || 
        current_filtered_index_ >= static_cast<int>(filtered_indices_.size())) {
        return -1;
    }
    return filtered_indices_[current_filtered_index_];
}

void StandaloneFileTinderDialog::record_action(int file_index, const QString& old_decision, 
                                               const QString& new_decision, const QString& old_dest_folder) {
    ActionRecord record;
    record.file_index = file_index;
    record.previous_decision = old_decision;
    record.new_decision = new_decision;
    record.destination_folder = old_dest_folder;  // Store old destination for undo restore
    undo_stack_.push_back(record);
    
    // Enable undo button
    if (undo_btn_) {
        undo_btn_->setEnabled(true);
    }
    
    // Save the NEW decision to DB immediately (crash safety)
    if (file_index >= 0 && file_index < static_cast<int>(files_.size())) {
        const auto& file = files_[file_index];
        db_.save_file_decision(source_folder_, file.path, file.decision, file.destination_folder);
    }
}

void StandaloneFileTinderDialog::on_keep() {
    try {
        int file_idx = get_current_file_index();
        if (file_idx < 0) return;
        
        auto& file = files_[file_idx];
        LOG_INFO("BasicMode", QString("Marking file as KEEP: %1").arg(file.name));
        
        QString old_decision = file.decision;
        if (old_decision != "pending") {
            update_decision_count(old_decision, -1);
        }
        
        file.decision = "keep";
        keep_count_++;
        
        // Record for undo
        record_action(file_idx, old_decision, "keep");
        
        animate_swipe(true);
        advance_to_next();
    } catch (const std::exception& ex) {
        LOG_ERROR("BasicMode", QString("Error in on_keep: %1").arg(ex.what()));
        QMessageBox::warning(this, "Error", QString("An error occurred: %1").arg(ex.what()));
    }
}

void StandaloneFileTinderDialog::on_delete() {
    try {
        int file_idx = get_current_file_index();
        if (file_idx < 0) return;
        
        auto& file = files_[file_idx];
        LOG_INFO("BasicMode", QString("Marking file as DELETE: %1").arg(file.name));
        
        QString old_decision = file.decision;
        if (old_decision != "pending") {
            update_decision_count(old_decision, -1);
        }
        
        file.decision = "delete";
        delete_count_++;
        
        // Record for undo
        record_action(file_idx, old_decision, "delete");
        
        animate_swipe(true);
        advance_to_next();
    } catch (const std::exception& ex) {
        LOG_ERROR("BasicMode", QString("Error in on_delete: %1").arg(ex.what()));
        QMessageBox::warning(this, "Error", QString("An error occurred: %1").arg(ex.what()));
    }
}

void StandaloneFileTinderDialog::on_skip() {
    try {
        int file_idx = get_current_file_index();
        if (file_idx < 0) return;
        
        auto& file = files_[file_idx];
        LOG_DEBUG("BasicMode", QString("Skipping file: %1").arg(file.name));
        
        QString old_decision = file.decision;
        if (old_decision != "pending") {
            update_decision_count(old_decision, -1);
        }
        
        file.decision = "skip";
        skip_count_++;
        
        // Record for undo
        record_action(file_idx, old_decision, "skip");
        
        animate_swipe(true);
        advance_to_next();
    } catch (const std::exception& ex) {
        LOG_ERROR("BasicMode", QString("Error in on_skip: %1").arg(ex.what()));
        QMessageBox::warning(this, "Error", QString("An error occurred: %1").arg(ex.what()));
    }
}

void StandaloneFileTinderDialog::on_back() {
    try {
        LOG_DEBUG("BasicMode", "Going back to previous file");
        animate_swipe(false);
        go_to_previous();
    } catch (const std::exception& ex) {
        LOG_ERROR("BasicMode", QString("Error in on_back: %1").arg(ex.what()));
    }
}

void StandaloneFileTinderDialog::on_undo() {
    try {
        if (undo_stack_.empty()) {
            LOG_DEBUG("BasicMode", "Nothing to undo");
            return;
        }
        
        // Pop the last action
        ActionRecord last_action = undo_stack_.back();
        undo_stack_.pop_back();
        
        // Revert the file's decision
        auto& file = files_[last_action.file_index];
        LOG_INFO("BasicMode", QString("Undoing action on file: %1 (was %2, reverting to %3)")
                             .arg(file.name, last_action.new_decision, last_action.previous_decision));
        
        // Decrement count for the action we're undoing
        update_decision_count(last_action.new_decision, -1);
        
        // Restore previous decision
        file.decision = last_action.previous_decision;
        file.destination_folder = last_action.destination_folder;
        
        // Save restored decision to DB immediately
        db_.save_file_decision(source_folder_, file.path, 
                              last_action.previous_decision, last_action.destination_folder);
        
        // Increment count for the restored decision (if not pending)
        if (last_action.previous_decision != "pending") {
            update_decision_count(last_action.previous_decision, 1);
        }
        
        // Navigate to the undone file
        for (int i = 0; i < static_cast<int>(filtered_indices_.size()); ++i) {
            if (filtered_indices_[i] == last_action.file_index) {
                current_filtered_index_ = i;
                break;
            }
        }
        
        // Update UI
        update_stats();
        update_progress();
        show_current_file();
        
        // Disable undo button if stack is empty
        if (undo_stack_.empty() && undo_btn_) {
            undo_btn_->setEnabled(false);
        }
        
    } catch (const std::exception& ex) {
        LOG_ERROR("BasicMode", QString("Error in on_undo: %1").arg(ex.what()));
        QMessageBox::warning(this, "Error", QString("An error occurred: %1").arg(ex.what()));
    }
}

void StandaloneFileTinderDialog::on_show_preview() {
    try {
        int file_idx = get_current_file_index();
        if (file_idx < 0) return;
        
        const auto& file = files_[file_idx];
        
        // Check if it's an image
        if (!file.mime_type.startsWith("image/")) {
            QMessageBox::information(this, "Preview", 
                "Preview is only available for image files.\n"
                "Current file type: " + file.mime_type);
            return;
        }
        
        // Create or show the image preview window
        if (!image_preview_window_) {
            image_preview_window_ = new ImagePreviewWindow(this);
            
            // Connect navigation signals
            connect(image_preview_window_, &ImagePreviewWindow::next_requested, this, [this]() {
                on_skip();  // Move to next file
                if (image_preview_window_ && image_preview_window_->isVisible()) {
                    int idx = get_current_file_index();
                    if (idx >= 0 && files_[idx].mime_type.startsWith("image/")) {
                        image_preview_window_->set_image(files_[idx].path);
                    }
                }
            });
            
            connect(image_preview_window_, &ImagePreviewWindow::previous_requested, this, [this]() {
                on_back();  // Move to previous file
                if (image_preview_window_ && image_preview_window_->isVisible()) {
                    int idx = get_current_file_index();
                    if (idx >= 0 && files_[idx].mime_type.startsWith("image/")) {
                        image_preview_window_->set_image(files_[idx].path);
                    }
                }
            });
        }
        
        image_preview_window_->set_image(file.path);
        image_preview_window_->show();
        image_preview_window_->raise();
        image_preview_window_->activateWindow();
        
        LOG_DEBUG("BasicMode", QString("Opened preview for: %1").arg(file.name));
        
    } catch (const std::exception& ex) {
        LOG_ERROR("BasicMode", QString("Error in on_show_preview: %1").arg(ex.what()));
        QMessageBox::warning(this, "Error", QString("An error occurred: %1").arg(ex.what()));
    }
}

void StandaloneFileTinderDialog::on_finish() {
    try {
        LOG_INFO("BasicMode", "Finishing review, showing summary");
        show_review_summary();
    } catch (const std::exception& ex) {
        LOG_ERROR("BasicMode", QString("Error in on_finish: %1").arg(ex.what()));
        QMessageBox::warning(this, "Error", QString("An error occurred: %1").arg(ex.what()));
    }
}

void StandaloneFileTinderDialog::advance_to_next() {
    update_stats();
    update_progress();
    
    // Find next pending file in filtered list
    int start = current_filtered_index_ + 1;
    for (int i = start; i < static_cast<int>(filtered_indices_.size()); ++i) {
        if (files_[filtered_indices_[i]].decision == "pending") {
            current_filtered_index_ = i;
            show_current_file();
            return;
        }
    }
    
    // No more pending files in filter
    current_filtered_index_ = static_cast<int>(filtered_indices_.size());
    if (file_icon_label_) file_icon_label_->clear();
    if (preview_label_) {
        preview_label_->setText("<div style='text-align: center; font-size: 48px;'>🎉</div>"
                               "<div style='text-align: center; font-size: 18px; color: #2ecc71;'>"
                               "All files reviewed!</div>");
    }
    if (file_info_label_) {
        file_info_label_->setText("Click 'Finish Review' to execute your decisions.");
    }
}

void StandaloneFileTinderDialog::go_to_previous() {
    if (current_filtered_index_ <= 0) return;
    
    // Find previous file in filtered list
    for (int i = current_filtered_index_ - 1; i >= 0; --i) {
        current_filtered_index_ = i;
        show_current_file();
        return;
    }
}

QString StandaloneFileTinderDialog::show_folder_picker() {
    // Show dialog with recent folders and browse option
    QDialog dialog(this);
    dialog.setWindowTitle("Select Destination Folder");
    dialog.setMinimumSize(ui::scaling::scaled(400), ui::scaling::scaled(300));
    
    auto* layout = new QVBoxLayout(&dialog);
    
    // Recent folders
    auto* recent_label = new QLabel("Recent Folders:");
    recent_label->setStyleSheet("font-weight: bold;");
    layout->addWidget(recent_label);
    
    auto* recent_list = new QListWidget();
    QStringList recent = db_.get_recent_folders(10);
    for (const QString& folder : recent) {
        recent_list->addItem(folder);
    }
    layout->addWidget(recent_list);
    
    QString selected_folder;
    
    // Buttons
    auto* btn_layout = new QHBoxLayout();
    
    auto* new_folder_btn = new QPushButton("Create New Folder...");
    connect(new_folder_btn, &QPushButton::clicked, &dialog, [&]() {
        bool ok;
        QString name = QInputDialog::getText(&dialog, "New Folder", 
                                             "Enter folder name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            QString parent = QFileDialog::getExistingDirectory(&dialog, "Select Parent Directory", 
                                                              source_folder_);
            if (!parent.isEmpty()) {
                selected_folder = parent + "/" + name;
                QDir().mkpath(selected_folder);
                dialog.accept();
            }
        }
    });
    btn_layout->addWidget(new_folder_btn);
    
    auto* browse_btn = new QPushButton("Browse...");
    connect(browse_btn, &QPushButton::clicked, &dialog, [&]() {
        QString folder = QFileDialog::getExistingDirectory(&dialog, "Select Destination Folder", 
                                                          source_folder_);
        if (!folder.isEmpty()) {
            selected_folder = folder;
            dialog.accept();
        }
    });
    btn_layout->addWidget(browse_btn);
    
    auto* cancel_btn = new QPushButton("Cancel");
    connect(cancel_btn, &QPushButton::clicked, &dialog, &QDialog::reject);
    btn_layout->addWidget(cancel_btn);
    
    layout->addLayout(btn_layout);
    
    // Double-click on recent
    connect(recent_list, &QListWidget::itemDoubleClicked, &dialog, [&](QListWidgetItem* item) {
        selected_folder = item->text();
        dialog.accept();
    });
    
    if (dialog.exec() == QDialog::Accepted) {
        return selected_folder;
    }
    
    return QString();
}

void StandaloneFileTinderDialog::show_review_summary() {
    QDialog summary_dialog(this);
    summary_dialog.setWindowTitle("Review Summary");
    summary_dialog.setMinimumSize(ui::scaling::scaled(700), ui::scaling::scaled(500));
    
    auto* layout = new QVBoxLayout(&summary_dialog);
    
    // Summary stats
    auto* stats_widget = new QWidget();
    stats_widget->setStyleSheet("background-color: #34495e; border-radius: 8px; padding: 15px;");
    auto* stats_layout = new QHBoxLayout(stats_widget);
    
    auto create_stat_box = [](const QString& label, int count, const QString& color) {
        auto* box = new QWidget();
        box->setStyleSheet(QString("background-color: %1; border-radius: 6px; padding: 10px;").arg(color));
        auto* box_layout = new QVBoxLayout(box);
        auto* count_label = new QLabel(QString::number(count));
        count_label->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
        count_label->setAlignment(Qt::AlignCenter);
        auto* text_label = new QLabel(label);
        text_label->setStyleSheet("color: white;");
        text_label->setAlignment(Qt::AlignCenter);
        box_layout->addWidget(count_label);
        box_layout->addWidget(text_label);
        return box;
    };
    
    stats_layout->addWidget(create_stat_box("Keep", keep_count_, ui::colors::kKeepColor));
    stats_layout->addWidget(create_stat_box("Delete", delete_count_, ui::colors::kDeleteColor));
    stats_layout->addWidget(create_stat_box("Skip", skip_count_, ui::colors::kSkipColor));
    stats_layout->addWidget(create_stat_box("Move", move_count_, ui::colors::kMoveColor));
    
    layout->addWidget(stats_widget);
    
    // Move destinations table
    if (move_count_ > 0) {
        auto* move_label = new QLabel("Move Destinations:");
        move_label->setStyleSheet("font-weight: bold; margin-top: 15px;");
        layout->addWidget(move_label);
        
        auto* table = new QTableWidget();
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Destination Folder", "Files", "Size"});
        table->horizontalHeader()->setStretchLastSection(true);
        
        // Collect move destinations
        QMap<QString, QPair<int, qint64>> dest_stats;
        for (const auto& file : files_) {
            if (file.decision == "move" && !file.destination_folder.isEmpty()) {
                auto& stats = dest_stats[file.destination_folder];
                stats.first++;
                stats.second += file.size;
            }
        }
        
        table->setRowCount(dest_stats.size());
        int row = 0;
        int new_folder_count = 0;
        for (auto it = dest_stats.begin(); it != dest_stats.end(); ++it, ++row) {
            QString folder_display = it.key();
            // Mark folders that don't exist yet (will be created)
            if (!QDir(it.key()).exists()) {
                folder_display = it.key() + "  [NEW]";
                new_folder_count++;
            }
            table->setItem(row, 0, new QTableWidgetItem(folder_display));
            table->setItem(row, 1, new QTableWidgetItem(QString::number(it.value().first)));
            
            qint64 size = it.value().second;
            QString size_str;
            if (size < 1024) size_str = QString("%1 B").arg(size);
            else if (size < 1024LL*1024) size_str = QString("%1 KB").arg(size/1024.0, 0, 'f', 1);
            else size_str = QString("%1 MB").arg(size/(1024.0*1024.0), 0, 'f', 2);
            table->setItem(row, 2, new QTableWidgetItem(size_str));
        }
        
        table->resizeColumnsToContents();
        layout->addWidget(table);
        
        if (new_folder_count > 0) {
            auto* new_folders_note = new QLabel(
                QString("Note: %1 folder(s) marked [NEW] will be created during execution.")
                    .arg(new_folder_count));
            new_folders_note->setStyleSheet("color: #f39c12; font-style: italic; margin-top: 4px;");
            layout->addWidget(new_folders_note);
        }
    }
    
    // Buttons
    auto* btn_layout = new QHBoxLayout();
    
    auto* cancel_btn = new QPushButton("Cancel");
    connect(cancel_btn, &QPushButton::clicked, &summary_dialog, &QDialog::reject);
    btn_layout->addWidget(cancel_btn);
    
    btn_layout->addStretch();
    
    auto* execute_btn = new QPushButton("Execute All ✓");
    execute_btn->setStyleSheet(
        "QPushButton { background-color: #2ecc71; color: white; font-weight: bold; "
        "padding: 10px 20px; border-radius: 6px; }"
        "QPushButton:hover { background-color: #27ae60; }"
    );
    connect(execute_btn, &QPushButton::clicked, &summary_dialog, [&]() {
        summary_dialog.accept();
        execute_decisions();
    });
    btn_layout->addWidget(execute_btn);
    
    layout->addLayout(btn_layout);
    
    summary_dialog.exec();
}

void StandaloneFileTinderDialog::execute_decisions() {
    ExecutionPlan plan;
    
    // Collect unique destination folders from move decisions
    QSet<QString> dest_folders;
    
    for (const auto& file : files_) {
        if (file.decision == "delete") {
            plan.files_to_delete.push_back(file.path);
        } else if (file.decision == "move" && !file.destination_folder.isEmpty()) {
            plan.files_to_move.push_back({file.path, file.destination_folder});
            dest_folders.insert(file.destination_folder);
        }
    }
    
    // Ensure all destination folders exist (handles virtual folders from advanced mode)
    for (const QString& folder : dest_folders) {
        if (!QDir(folder).exists()) {
            plan.folders_to_create.push_back(folder);
        }
    }
    
    // Progress dialog
    QProgressDialog progress("Executing decisions...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    
    FileTinderExecutor executor;
    auto result = executor.execute(plan, [&](int current, int total, const QString& msg) {
        if (total > 0) {
            progress.setValue(current * 100 / total);
        }
        progress.setLabelText(msg);
        QApplication::processEvents();
    });
    
    progress.close();
    
    // Show result
    QString message = QString(
        "Execution complete!\n\n"
        "Files deleted: %1\n"
        "Files moved: %2\n"
        "Folders created: %3\n"
        "Errors: %4"
    ).arg(result.files_deleted).arg(result.files_moved)
     .arg(result.folders_created).arg(result.errors);
    
    if (!result.error_messages.isEmpty()) {
        message += "\n\nErrors:\n" + result.error_messages.join("\n");
    }
    
    QMessageBox::information(this, "Execution Complete", message);
    
    // Clear session
    db_.clear_session(source_folder_);
    
    emit session_completed();
    accept();
}

void StandaloneFileTinderDialog::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Right:
            on_keep();
            break;
        case Qt::Key_Left:
            on_delete();
            break;
        case Qt::Key_Down:
            on_skip();
            break;
        case Qt::Key_Up:
        case Qt::Key_Backspace:
            on_back();
            break;
        case Qt::Key_Z:
            // Undo last action
            on_undo();
            break;
        case Qt::Key_P:
            // Open preview in separate window
            on_show_preview();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            on_finish();
            break;
        case Qt::Key_Question:
        case Qt::Key_Slash:
            if (event->modifiers() & Qt::ShiftModifier) {
                show_shortcuts_help();
            }
            break;
        // Note: Number keys 1-0 are reserved for Quick Access in Advanced Mode
        default:
            QDialog::keyPressEvent(event);
    }
}

void StandaloneFileTinderDialog::closeEvent(QCloseEvent* event) {
    // Route all close requests through reject() which handles save logic
    // and properly terminates the exec() event loop.
    event->ignore();
    reject();
}

void StandaloneFileTinderDialog::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    
    // Use debounced timer to update preview after resize stops
    // This prevents stutter during continuous resizing
    if (resize_timer_) {
        resize_timer_->start();  // Restart the timer on each resize event
    }
}

void StandaloneFileTinderDialog::reject() {
    // Guard against re-entrant calls
    if (closing_) return;
    closing_ = true;
    
    // Check if there are any pending decisions to save
    int reviewed = keep_count_ + delete_count_ + skip_count_ + move_count_;
    
    if (reviewed > 0 && !files_.empty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            "Save Progress?",
            QString("You have made %1 decisions. Do you want to save your progress before closing?\n\n"
                    "Your session will be saved and you can continue later.")
                    .arg(reviewed),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save
        );
        
        if (reply == QMessageBox::Save) {
            save_session_state();
        } else if (reply == QMessageBox::Cancel) {
            closing_ = false;
            return;  // Don't close
        }
        // Discard: fall through to close without saving
    } else {
        // No decisions made, just save state and close
        save_session_state();
    }
    
    // Properly terminate the exec() event loop
    QDialog::reject();
}

void StandaloneFileTinderDialog::on_switch_mode_clicked() {
    save_session_state();
    emit switch_to_advanced_mode();
}

// Sorting implementation
void StandaloneFileTinderDialog::on_sort_changed(int index) {
    if (!sort_combo_) return;
    sort_field_ = static_cast<FileSortField>(sort_combo_->itemData(index).toInt());
    apply_sort();
    rebuild_filtered_indices();
    if (!filtered_indices_.empty()) {
        show_current_file();
    }
    update_progress();
}

void StandaloneFileTinderDialog::on_sort_order_toggled() {
    if (sort_order_ == SortOrder::Ascending) {
        sort_order_ = SortOrder::Descending;
        sort_order_btn_->setText("Desc");
    } else {
        sort_order_ = SortOrder::Ascending;
        sort_order_btn_->setText("Asc");
    }
    apply_sort();
    rebuild_filtered_indices();
    if (!filtered_indices_.empty()) {
        show_current_file();
    }
    update_progress();
}

void StandaloneFileTinderDialog::on_folders_toggle_changed(int state) {
    include_folders_ = (state == Qt::Checked);
    // Reset counts before re-scanning and reloading state
    keep_count_ = 0;
    delete_count_ = 0;
    skip_count_ = 0;
    move_count_ = 0;
    scan_files();  // Re-scan with new settings
    apply_sort();
    rebuild_filtered_indices();
    load_session_state();
    if (!filtered_indices_.empty()) {
        show_current_file();
    }
    update_progress();
}

void StandaloneFileTinderDialog::apply_sort() {
    if (files_.empty()) return;
    
    auto compare_fn = [this](const FileToProcess& a, const FileToProcess& b) {
        int cmp = 0;
        switch (sort_field_) {
            case FileSortField::Name:
                cmp = a.name.compare(b.name, Qt::CaseInsensitive);
                break;
            case FileSortField::Size:
                cmp = (a.size < b.size) ? -1 : (a.size > b.size) ? 1 : 0;
                break;
            case FileSortField::Type:
                cmp = a.extension.compare(b.extension, Qt::CaseInsensitive);
                break;
            case FileSortField::DateModified:
                cmp = (a.modified_datetime < b.modified_datetime) ? -1 : 
                      (a.modified_datetime > b.modified_datetime) ? 1 : 0;
                break;
        }
        return sort_order_ == SortOrder::Ascending ? cmp < 0 : cmp > 0;
    };
    
    std::sort(files_.begin(), files_.end(), compare_fn);
}

void StandaloneFileTinderDialog::show_custom_extension_dialog() {
    bool ok;
    QString text = QInputDialog::getText(this, "Custom Filter",
        "Enter file extensions separated by commas (e.g., txt,csv,log):",
        QLineEdit::Normal, custom_extensions_.join(","), &ok);
    
    if (ok && !text.isEmpty()) {
        custom_extensions_.clear();
        QStringList parts = text.split(",", Qt::SkipEmptyParts);
        for (const QString& part : parts) {
            QString ext = part.trimmed().toLower();
            if (!ext.startsWith(".")) ext = "." + ext;
            custom_extensions_.append(ext);
        }
        rebuild_filtered_indices();
        if (!filtered_indices_.empty()) {
            show_current_file();
        } else {
            if (preview_label_)
                preview_label_->setText("No files match the specified extensions");
            if (file_info_label_)
                file_info_label_->setText("Extensions: " + custom_extensions_.join(", "));
        }
        update_progress();
    }
}

// Filter implementation
void StandaloneFileTinderDialog::on_filter_changed(int index) {
    FileFilterType filter = static_cast<FileFilterType>(filter_combo_->itemData(index).toInt());
    
    // Handle custom filter specially
    if (filter == FileFilterType::Custom) {
        show_custom_extension_dialog();
        return;
    }
    
    apply_filter(filter);
}

void StandaloneFileTinderDialog::apply_filter(FileFilterType filter) {
    current_filter_ = filter;
    rebuild_filtered_indices();
    
    // Find first pending file in new filter
    current_filtered_index_ = 0;
    for (size_t i = 0; i < filtered_indices_.size(); ++i) {
        if (files_[filtered_indices_[i]].decision == "pending") {
            current_filtered_index_ = static_cast<int>(i);
            break;
        }
    }
    
    if (!filtered_indices_.empty()) {
        show_current_file();
    } else {
        if (preview_label_)
            preview_label_->setText("<div style='text-align: center; font-size: 24px; color: #f39c12;'>"
                                   "No files match this filter</div>");
        if (file_info_label_)
            file_info_label_->setText("Try selecting a different filter or 'All Files'.");
    }
    
    update_progress();
}

void StandaloneFileTinderDialog::rebuild_filtered_indices() {
    filtered_indices_.clear();
    
    for (size_t i = 0; i < files_.size(); ++i) {
        if (file_matches_filter(files_[i])) {
            filtered_indices_.push_back(static_cast<int>(i));
        }
    }
}

bool StandaloneFileTinderDialog::file_matches_filter(const FileToProcess& file) const {
    if (current_filter_ == FileFilterType::All) {
        return true;
    }
    
    QString mime = file.mime_type.toLower();
    
    switch (current_filter_) {
        case FileFilterType::Images:
            return !file.is_directory && mime.startsWith("image/");
        case FileFilterType::Videos:
            return !file.is_directory && mime.startsWith("video/");
        case FileFilterType::Audio:
            return !file.is_directory && mime.startsWith("audio/");
        case FileFilterType::Documents:
            return !file.is_directory && (mime.startsWith("text/") || 
                   mime.contains("pdf") || 
                   mime.contains("document") ||
                   mime.contains("spreadsheet") ||
                   mime.contains("presentation"));
        case FileFilterType::Archives:
            return !file.is_directory && (mime.contains("zip") || 
                   mime.contains("tar") || 
                   mime.contains("archive") ||
                   mime.contains("compressed"));
        case FileFilterType::Other:
            // Match anything not covered by other filters (excluding directories)
            return !file.is_directory &&
                   !mime.startsWith("image/") && 
                   !mime.startsWith("video/") && 
                   !mime.startsWith("audio/") &&
                   !mime.startsWith("text/") &&
                   !mime.contains("pdf") &&
                   !mime.contains("document") &&
                   !mime.contains("spreadsheet") &&
                   !mime.contains("presentation") &&
                   !mime.contains("zip") &&
                   !mime.contains("archive");
        case FileFilterType::FoldersOnly:
            return file.is_directory;
        case FileFilterType::Custom: {
            // Match custom extensions
            if (custom_extensions_.isEmpty()) return true;
            QString ext = "." + file.extension.toLower();
            for (const QString& custom_ext : custom_extensions_) {
                if (ext == custom_ext || file.extension.toLower() == custom_ext.mid(1)) {
                    return true;
                }
            }
            return false;
        }
        default:
            return true;
    }
}

// Animation implementation
void StandaloneFileTinderDialog::animate_swipe(bool forward) {
    if (!preview_label_) return;
    
    // Reuse existing effect or create new one
    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(preview_label_->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(preview_label_);
        preview_label_->setGraphicsEffect(effect);
    }
    
    // Stop any existing animation - deletion is handled by DeleteWhenStopped from previous start()
    if (swipe_animation_) {
        swipe_animation_->stop();
        swipe_animation_ = nullptr;
    }
    
    // Create new animation with 'this' as parent for backup cleanup on dialog destruction
    swipe_animation_ = new QPropertyAnimation(effect, "opacity", this);
    swipe_animation_->setDuration(150);
    
    if (forward) {
        swipe_animation_->setStartValue(1.0);
        swipe_animation_->setEndValue(0.3);
    } else {
        swipe_animation_->setStartValue(0.3);
        swipe_animation_->setEndValue(1.0);
    }
    
    swipe_animation_->setEasingCurve(QEasingCurve::OutQuad);
    
    // Use QPointer to safely access effect in callback - will be null if effect was deleted
    QPointer<QGraphicsOpacityEffect> weak_effect = effect;
    connect(swipe_animation_, &QPropertyAnimation::finished, this, [weak_effect]() {
        if (weak_effect) {
            weak_effect->setOpacity(1.0);
        }
    });
    
    // DeleteWhenStopped handles cleanup when animation finishes or is stopped
    swipe_animation_->start(QAbstractAnimation::DeleteWhenStopped);
}

// Shortcuts help dialog
void StandaloneFileTinderDialog::show_shortcuts_help() {
    QMessageBox help_dialog(this);
    help_dialog.setWindowTitle("Keyboard Shortcuts");
    help_dialog.setIcon(QMessageBox::Information);
    
    QString shortcuts = R"(
<style>
    table { border-collapse: collapse; width: 100%; }
    th, td { padding: 8px; text-align: left; border-bottom: 1px solid #444; }
    th { background-color: #34495e; color: white; }
    .key { font-family: monospace; background: #3d566e; padding: 2px 6px; border-radius: 3px; }
</style>
<table>
<tr><th>Key</th><th>Action</th></tr>
<tr><td><span class='key'>→</span> Right Arrow</td><td>Keep file in original location</td></tr>
<tr><td><span class='key'>←</span> Left Arrow</td><td>Mark file for deletion</td></tr>
<tr><td><span class='key'>↓</span> Down Arrow</td><td>Skip file (no action)</td></tr>
<tr><td><span class='key'>↑</span> Up Arrow</td><td>Go back to previous file</td></tr>
<tr><td><span class='key'>Z</span></td><td>Undo last action</td></tr>
<tr><td><span class='key'>P</span></td><td>Open image preview in separate window</td></tr>
<tr><td><span class='key'>Enter</span></td><td>Finish review and execute</td></tr>
<tr><td><span class='key'>?</span> or <span class='key'>Shift+/</span></td><td>Show this help</td></tr>
<tr><td><span class='key'>Esc</span></td><td>Close dialog</td></tr>
</table>
<br>
<b>Tip:</b> Use the filter dropdown to filter by file type (Images, Videos, etc.) or specify custom extensions.
)";
    
    help_dialog.setTextFormat(Qt::RichText);
    help_dialog.setText(shortcuts);
    help_dialog.exec();
}
