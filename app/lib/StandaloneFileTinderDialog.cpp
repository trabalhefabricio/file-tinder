#include "StandaloneFileTinderDialog.hpp"
#include "DatabaseManager.hpp"
#include "FileTinderExecutor.hpp"
#include "ui_constants.hpp"
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QCloseEvent>
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

StandaloneFileTinderDialog::StandaloneFileTinderDialog(const QString& source_folder,
                                                       DatabaseManager& db,
                                                       QWidget* parent)
    : QDialog(parent)
    , current_filtered_index_(0)
    , source_folder_(source_folder)
    , db_(db)
    , current_filter_(FileFilterType::All)
    , keep_count_(0)
    , delete_count_(0)
    , skip_count_(0)
    , move_count_(0)
    , swipe_animation_(nullptr) {
    
    setWindowTitle("File Tinder - Basic Mode");
    setMinimumSize(ui::dimensions::kStandaloneFileTinderMinWidth,
                   ui::dimensions::kStandaloneFileTinderMinHeight);
    
    // Don't call setup_ui() here - it's virtual and could cause issues
    // with derived classes. Call initialize() after construction instead.
}

StandaloneFileTinderDialog::~StandaloneFileTinderDialog() = default;

void StandaloneFileTinderDialog::initialize() {
    setup_ui();
    scan_files();
    rebuild_filtered_indices();
    load_session_state();
    
    if (!filtered_indices_.empty()) {
        show_current_file();
    }
}

void StandaloneFileTinderDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 20, 20, 20);
    main_layout->setSpacing(15);
    
    // Title bar with filter and help
    auto* title_bar = new QWidget();
    auto* title_layout = new QHBoxLayout(title_bar);
    title_layout->setContentsMargins(0, 0, 0, 0);
    
    auto* title_label = new QLabel("📁 FILE TINDER - Basic Mode");
    title_label->setStyleSheet(QString(
        "font-size: %1px; font-weight: bold; color: %2; padding: 10px;"
    ).arg(ui::fonts::kHeaderSize).arg(ui::colors::kMoveColor));
    title_layout->addWidget(title_label);
    
    title_layout->addStretch();
    
    // File type filter
    auto* filter_label = new QLabel("Filter:");
    filter_label->setStyleSheet("font-weight: bold;");
    title_layout->addWidget(filter_label);
    
    filter_combo_ = new QComboBox();
    filter_combo_->addItem("📄 All Files", static_cast<int>(FileFilterType::All));
    filter_combo_->addItem("🖼️ Images", static_cast<int>(FileFilterType::Images));
    filter_combo_->addItem("🎬 Videos", static_cast<int>(FileFilterType::Videos));
    filter_combo_->addItem("🎵 Audio", static_cast<int>(FileFilterType::Audio));
    filter_combo_->addItem("📝 Documents", static_cast<int>(FileFilterType::Documents));
    filter_combo_->addItem("📦 Archives", static_cast<int>(FileFilterType::Archives));
    filter_combo_->addItem("❓ Other", static_cast<int>(FileFilterType::Other));
    filter_combo_->setMinimumWidth(130);
    filter_combo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; background-color: #34495e; "
        "border-radius: 4px; color: white; }"
        "QComboBox:hover { background-color: #3d566e; }"
        "QComboBox::drop-down { border: none; }"
    );
    connect(filter_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StandaloneFileTinderDialog::on_filter_changed);
    title_layout->addWidget(filter_combo_);
    
    // Help button
    help_btn_ = new QPushButton("❓");
    help_btn_->setFixedSize(36, 36);
    help_btn_->setToolTip("Keyboard Shortcuts");
    help_btn_->setStyleSheet(
        "QPushButton { font-size: 16px; background-color: #34495e; "
        "border-radius: 18px; }"
        "QPushButton:hover { background-color: #3d566e; }"
    );
    connect(help_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::show_shortcuts_help);
    title_layout->addWidget(help_btn_);
    
    main_layout->addWidget(title_bar);
    
    // Preview area
    auto* preview_group = new QGroupBox("File Preview");
    preview_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* preview_layout = new QVBoxLayout(preview_group);
    
    preview_label_ = new QLabel();
    preview_label_->setMinimumSize(ui::dimensions::kPreviewMaxWidth, 
                                    ui::dimensions::kPreviewMaxHeight);
    preview_label_->setAlignment(Qt::AlignCenter);
    preview_label_->setStyleSheet(
        "background-color: #2c3e50; border-radius: 8px; padding: 10px;"
    );
    preview_layout->addWidget(preview_label_);
    
    file_info_label_ = new QLabel();
    file_info_label_->setAlignment(Qt::AlignCenter);
    file_info_label_->setStyleSheet("color: #7f8c8d; padding: 5px;");
    preview_layout->addWidget(file_info_label_);
    
    main_layout->addWidget(preview_group, 1);
    
    // Progress section
    auto* progress_widget = new QWidget();
    auto* progress_vlayout = new QVBoxLayout(progress_widget);
    progress_vlayout->setContentsMargins(0, 0, 0, 0);
    
    progress_bar_ = new QProgressBar();
    progress_bar_->setTextVisible(true);
    progress_bar_->setStyleSheet(
        "QProgressBar { border: 2px solid #34495e; border-radius: 5px; text-align: center; }"
        "QProgressBar::chunk { background-color: #3498db; }"
    );
    progress_vlayout->addWidget(progress_bar_);
    
    progress_label_ = new QLabel();
    progress_label_->setAlignment(Qt::AlignCenter);
    progress_vlayout->addWidget(progress_label_);
    
    main_layout->addWidget(progress_widget);
    
    // Stats
    stats_label_ = new QLabel();
    stats_label_->setAlignment(Qt::AlignCenter);
    stats_label_->setStyleSheet(
        "font-size: 14px; padding: 10px; background-color: #34495e; border-radius: 5px;"
    );
    main_layout->addWidget(stats_label_);
    
    // Action buttons
    auto* buttons_widget = new QWidget();
    auto* buttons_layout = new QHBoxLayout(buttons_widget);
    buttons_layout->setSpacing(15);
    
    back_btn_ = new QPushButton("↑ Back");
    back_btn_->setMinimumSize(100, 50);
    back_btn_->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: bold; padding: 10px; "
        "background-color: #95a5a6; border-radius: 8px; }"
        "QPushButton:hover { background-color: #7f8c8d; }"
    );
    connect(back_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_back);
    buttons_layout->addWidget(back_btn_);
    
    delete_btn_ = new QPushButton("← Delete");
    delete_btn_->setMinimumSize(100, 50);
    delete_btn_->setStyleSheet(QString(
        "QPushButton { font-size: 14px; font-weight: bold; padding: 10px; "
        "background-color: %1; border-radius: 8px; color: white; }"
        "QPushButton:hover { background-color: #c0392b; }"
    ).arg(ui::colors::kDeleteColor));
    connect(delete_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_delete);
    buttons_layout->addWidget(delete_btn_);
    
    skip_btn_ = new QPushButton("↓ Skip");
    skip_btn_->setMinimumSize(100, 50);
    skip_btn_->setStyleSheet(QString(
        "QPushButton { font-size: 14px; font-weight: bold; padding: 10px; "
        "background-color: %1; border-radius: 8px; color: white; }"
        "QPushButton:hover { background-color: #e67e22; }"
    ).arg(ui::colors::kSkipColor));
    connect(skip_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_skip);
    buttons_layout->addWidget(skip_btn_);
    
    keep_btn_ = new QPushButton("→ Keep");
    keep_btn_->setMinimumSize(100, 50);
    keep_btn_->setStyleSheet(QString(
        "QPushButton { font-size: 14px; font-weight: bold; padding: 10px; "
        "background-color: %1; border-radius: 8px; color: white; }"
        "QPushButton:hover { background-color: #27ae60; }"
    ).arg(ui::colors::kKeepColor));
    connect(keep_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_keep);
    buttons_layout->addWidget(keep_btn_);
    
    main_layout->addWidget(buttons_widget);
    
    // Secondary actions
    auto* secondary_widget = new QWidget();
    auto* secondary_layout = new QHBoxLayout(secondary_widget);
    secondary_layout->setSpacing(15);
    
    move_btn_ = new QPushButton("📁 Move to Folder...");
    move_btn_->setMinimumSize(150, 40);
    move_btn_->setStyleSheet(QString(
        "QPushButton { font-size: 13px; padding: 8px; "
        "background-color: %1; border-radius: 6px; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }"
    ).arg(ui::colors::kMoveColor));
    connect(move_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_move_to_folder);
    secondary_layout->addWidget(move_btn_);
    
    switch_mode_btn_ = new QPushButton("🔄 Advanced Mode");
    switch_mode_btn_->setMinimumSize(150, 40);
    switch_mode_btn_->setStyleSheet(
        "QPushButton { font-size: 13px; padding: 8px; "
        "background-color: #9b59b6; border-radius: 6px; color: white; }"
        "QPushButton:hover { background-color: #8e44ad; }"
    );
    connect(switch_mode_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_switch_mode_clicked);
    secondary_layout->addWidget(switch_mode_btn_);
    
    finish_btn_ = new QPushButton("✓ Finish Review");
    finish_btn_->setMinimumSize(150, 40);
    finish_btn_->setStyleSheet(
        "QPushButton { font-size: 13px; padding: 8px; "
        "background-color: #1abc9c; border-radius: 6px; color: white; }"
        "QPushButton:hover { background-color: #16a085; }"
    );
    connect(finish_btn_, &QPushButton::clicked, this, &StandaloneFileTinderDialog::on_finish);
    secondary_layout->addWidget(finish_btn_);
    
    main_layout->addWidget(secondary_widget);
    
    // Keyboard shortcuts hint at bottom
    shortcuts_label_ = new QLabel("→ Keep | ← Delete | ↓ Skip | ↑ Back | M Move | ? Help");
    shortcuts_label_->setAlignment(Qt::AlignCenter);
    shortcuts_label_->setStyleSheet("color: #7f8c8d; font-size: 11px;");
    main_layout->addWidget(shortcuts_label_);
    
    update_stats();
}

void StandaloneFileTinderDialog::scan_files() {
    files_.clear();
    QMimeDatabase mime_db;
    
    QDir dir(source_folder_);
    if (!dir.exists()) {
        QMessageBox::warning(this, "Error", "Source folder does not exist.");
        return;
    }
    
    QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    
    for (const QString& entry : entries) {
        QString full_path = dir.absoluteFilePath(entry);
        QFileInfo info(full_path);
        QMimeType mime_type = mime_db.mimeTypeForFile(full_path);
        
        FileToProcess file;
        file.path = full_path;
        file.name = info.fileName();
        file.extension = info.suffix().toLower();
        file.size = info.size();
        file.modified_date = info.lastModified().toString("MMM d, yyyy HH:mm");
        file.decision = "pending";
        file.mime_type = mime_type.name();
        
        files_.push_back(file);
    }
    
    // Update progress
    if (!files_.empty()) {
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
        preview_label_->setText("No more files to review");
        file_info_label_->setText("");
        return;
    }
    
    const auto& file = files_[file_idx];
    update_preview(file.path);
    update_file_info(file);
    update_progress();
}

void StandaloneFileTinderDialog::update_preview(const QString& file_path) {
    QMimeDatabase mime_db;
    QMimeType mime_type = mime_db.mimeTypeForFile(file_path);
    QString type = mime_type.name();
    
    if (type.startsWith("image/")) {
        QPixmap pixmap(file_path);
        if (!pixmap.isNull()) {
            pixmap = pixmap.scaled(ui::dimensions::kPreviewMaxWidth - 20,
                                   ui::dimensions::kPreviewMaxHeight - 20,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
            preview_label_->setPixmap(pixmap);
            return;
        }
    } else if (type.startsWith("text/")) {
        QFile file(file_path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QString content = stream.read(2000);
            if (stream.status() == QTextStream::Ok && !stream.atEnd()) {
                content += "\n...(truncated)";
            }
            preview_label_->setText(content);
            preview_label_->setWordWrap(true);
            return;
        }
    }
    
    // Default: show file icon and info
    QString icon = "📄";
    if (type.startsWith("video/")) icon = "🎬";
    else if (type.startsWith("audio/")) icon = "🎵";
    else if (type.contains("pdf")) icon = "📕";
    else if (type.contains("zip") || type.contains("archive")) icon = "📦";
    else if (type.contains("spreadsheet") || type.contains("excel")) icon = "📊";
    else if (type.contains("document") || type.contains("word")) icon = "📝";
    
    preview_label_->setText(QString("<div style='text-align: center; font-size: 64px;'>%1</div>"
                                    "<div style='text-align: center; color: #bdc3c7; font-size: 14px;'>%2</div>")
                           .arg(icon, mime_type.comment()));
}

void StandaloneFileTinderDialog::update_file_info(const FileToProcess& file) {
    QString size_str;
    if (file.size < 1024) {
        size_str = QString("%1 B").arg(file.size);
    } else if (file.size < 1024 * 1024) {
        size_str = QString("%1 KB").arg(file.size / 1024.0, 0, 'f', 1);
    } else if (file.size < 1024 * 1024 * 1024) {
        size_str = QString("%1 MB").arg(file.size / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        size_str = QString("%1 GB").arg(file.size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
    
    file_info_label_->setText(QString("<b>%1</b><br>Size: %2 | Type: %3 | Modified: %4")
                              .arg(file.name, size_str, 
                                   file.extension.isEmpty() ? "Unknown" : file.extension.toUpper(),
                                   file.modified_date));
}

void StandaloneFileTinderDialog::update_progress() {
    int reviewed = keep_count_ + delete_count_ + skip_count_ + move_count_;
    int total = static_cast<int>(files_.size());
    int filtered_total = static_cast<int>(filtered_indices_.size());
    
    progress_bar_->setValue(reviewed);
    
    int percent = total > 0 ? (reviewed * 100 / total) : 0;
    QString filter_info = (current_filter_ != FileFilterType::All) 
        ? QString(" (showing %1 of %2)").arg(filtered_total).arg(total)
        : "";
    progress_label_->setText(QString("Progress: %1 / %2 files (%3%)%4")
                             .arg(reviewed).arg(total).arg(percent).arg(filter_info));
}

void StandaloneFileTinderDialog::update_stats() {
    stats_label_->setText(QString(
        "<span style='color: %1;'>✓ Keep: %2</span>  |  "
        "<span style='color: %3;'>✗ Delete: %4</span>  |  "
        "<span style='color: %5;'>↓ Skip: %6</span>  |  "
        "<span style='color: %7;'>📁 Move: %8</span>"
    ).arg(ui::colors::kKeepColor).arg(keep_count_)
     .arg(ui::colors::kDeleteColor).arg(delete_count_)
     .arg(ui::colors::kSkipColor).arg(skip_count_)
     .arg(ui::colors::kMoveColor).arg(move_count_));
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

void StandaloneFileTinderDialog::on_keep() {
    int file_idx = get_current_file_index();
    if (file_idx < 0) return;
    
    auto& file = files_[file_idx];
    if (file.decision != "pending") {
        update_decision_count(file.decision, -1);
    }
    
    file.decision = "keep";
    keep_count_++;
    
    animate_swipe(true);
    advance_to_next();
}

void StandaloneFileTinderDialog::on_delete() {
    int file_idx = get_current_file_index();
    if (file_idx < 0) return;
    
    auto& file = files_[file_idx];
    if (file.decision != "pending") {
        update_decision_count(file.decision, -1);
    }
    
    file.decision = "delete";
    delete_count_++;
    
    animate_swipe(true);
    advance_to_next();
}

void StandaloneFileTinderDialog::on_skip() {
    int file_idx = get_current_file_index();
    if (file_idx < 0) return;
    
    auto& file = files_[file_idx];
    if (file.decision != "pending") {
        update_decision_count(file.decision, -1);
    }
    
    file.decision = "skip";
    skip_count_++;
    
    animate_swipe(true);
    advance_to_next();
}

void StandaloneFileTinderDialog::on_back() {
    animate_swipe(false);
    go_to_previous();
}

void StandaloneFileTinderDialog::on_move_to_folder() {
    int file_idx = get_current_file_index();
    if (file_idx < 0) return;
    
    QString folder = show_folder_picker();
    if (folder.isEmpty()) return;
    
    auto& file = files_[file_idx];
    if (file.decision != "pending") {
        update_decision_count(file.decision, -1);
    }
    
    file.decision = "move";
    file.destination_folder = folder;
    move_count_++;
    
    db_.add_recent_folder(folder);
    
    advance_to_next();
}

void StandaloneFileTinderDialog::on_finish() {
    show_review_summary();
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
    preview_label_->setText("<div style='text-align: center; font-size: 48px;'>🎉</div>"
                           "<div style='text-align: center; font-size: 18px; color: #2ecc71;'>"
                           "All files reviewed!</div>");
    file_info_label_->setText("Click 'Finish Review' to execute your decisions.");
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
    dialog.setMinimumSize(400, 300);
    
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
    summary_dialog.setMinimumSize(700, 500);
    
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
        for (auto it = dest_stats.begin(); it != dest_stats.end(); ++it, ++row) {
            table->setItem(row, 0, new QTableWidgetItem(it.key()));
            table->setItem(row, 1, new QTableWidgetItem(QString::number(it.value().first)));
            
            qint64 size = it.value().second;
            QString size_str;
            if (size < 1024) size_str = QString("%1 B").arg(size);
            else if (size < 1024*1024) size_str = QString("%1 KB").arg(size/1024.0, 0, 'f', 1);
            else size_str = QString("%1 MB").arg(size/(1024.0*1024.0), 0, 'f', 2);
            table->setItem(row, 2, new QTableWidgetItem(size_str));
        }
        
        table->resizeColumnsToContents();
        layout->addWidget(table);
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
    
    for (const auto& file : files_) {
        if (file.decision == "delete") {
            plan.files_to_delete.push_back(file.path);
        } else if (file.decision == "move" && !file.destination_folder.isEmpty()) {
            plan.files_to_move.push_back({file.path, file.destination_folder});
        }
    }
    
    // Progress dialog
    QProgressDialog progress("Executing decisions...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    
    FileTinderExecutor executor;
    auto result = executor.execute(plan, [&](int current, int total, const QString& msg) {
        progress.setValue(current * 100 / total);
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
        case Qt::Key_M:
            on_move_to_folder();
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
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
            // Number keys 1-6 for filter types
            filter_combo_->setCurrentIndex(event->key() - Qt::Key_1);
            break;
        default:
            QDialog::keyPressEvent(event);
    }
}

void StandaloneFileTinderDialog::closeEvent(QCloseEvent* event) {
    save_session_state();
    QDialog::closeEvent(event);
}

void StandaloneFileTinderDialog::on_switch_mode_clicked() {
    save_session_state();
    emit switch_to_advanced_mode();
}

// Filter implementation
void StandaloneFileTinderDialog::on_filter_changed(int index) {
    FileFilterType filter = static_cast<FileFilterType>(filter_combo_->itemData(index).toInt());
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
        preview_label_->setText("<div style='text-align: center; font-size: 24px; color: #f39c12;'>"
                               "No files match this filter</div>");
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
            return mime.startsWith("image/");
        case FileFilterType::Videos:
            return mime.startsWith("video/");
        case FileFilterType::Audio:
            return mime.startsWith("audio/");
        case FileFilterType::Documents:
            return mime.startsWith("text/") || 
                   mime.contains("pdf") || 
                   mime.contains("document") ||
                   mime.contains("spreadsheet") ||
                   mime.contains("presentation");
        case FileFilterType::Archives:
            return mime.contains("zip") || 
                   mime.contains("tar") || 
                   mime.contains("archive") ||
                   mime.contains("compressed");
        case FileFilterType::Other:
            // Match anything not covered by other filters
            return !mime.startsWith("image/") && 
                   !mime.startsWith("video/") && 
                   !mime.startsWith("audio/") &&
                   !mime.startsWith("text/") &&
                   !mime.contains("pdf") &&
                   !mime.contains("document") &&
                   !mime.contains("spreadsheet") &&
                   !mime.contains("presentation") &&
                   !mime.contains("zip") &&
                   !mime.contains("archive");
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
    
    // Stop and delete any running animation before creating a new one
    if (swipe_animation_) {
        swipe_animation_->stop();
        delete swipe_animation_;
        swipe_animation_ = nullptr;
    }
    
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
    
    connect(swipe_animation_, &QPropertyAnimation::finished, this, [effect]() {
        // Reset opacity after animation
        effect->setOpacity(1.0);
    });
    
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
<tr><td><span class='key'>M</span></td><td>Move file to folder</td></tr>
<tr><td><span class='key'>Enter</span></td><td>Finish review and execute</td></tr>
<tr><td><span class='key'>1-6</span></td><td>Quick filter by file type</td></tr>
<tr><td><span class='key'>?</span> or <span class='key'>Shift+/</span></td><td>Show this help</td></tr>
<tr><td><span class='key'>Esc</span></td><td>Close dialog</td></tr>
</table>
<br>
<b>Filters:</b><br>
1 = All | 2 = Images | 3 = Videos | 4 = Audio | 5 = Documents | 6 = Archives<br>
(Use the dropdown to select 'Other' filter)
)";
    
    help_dialog.setTextFormat(Qt::RichText);
    help_dialog.setText(shortcuts);
    help_dialog.exec();
}
