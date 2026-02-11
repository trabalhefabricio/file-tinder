#include "MindMapView.hpp"
#include "FolderTreeModel.hpp"
#include "ui_constants.hpp"
#include <QContextMenuEvent>
#include <QFileInfo>

// === FolderButton Implementation ===

FolderButton::FolderButton(FolderNode* node, int depth, QWidget* parent)
    : QPushButton(parent)
    , node_(node)
    , depth_(depth)
    , is_selected_(false) {
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(ui::scaling::scaled(38));
    update_display();
    update_style();
    
    connect(this, &QPushButton::clicked, this, [this]() {
        emit folder_clicked(node_->path);
    });
}

void FolderButton::update_display() {
    QString name = node_->display_name;
    if (name.isEmpty()) {
        name = QFileInfo(node_->path).fileName();
    }
    
    // Build display text with indent, icon, name, and count
    QString indent;
    for (int i = 0; i < depth_; ++i) indent += "    ";
    
    QString icon = node_->exists ? "[D]" : "[+]";
    QString count_str;
    if (node_->assigned_file_count > 0) {
        count_str = QString(" (%1)").arg(node_->assigned_file_count);
    }
    
    setText(indent + icon + " " + name + count_str);
    setToolTip(node_->path);
}

void FolderButton::set_selected(bool selected) {
    is_selected_ = selected;
    update_style();
}

void FolderButton::update_style() {
    int left_margin = ui::scaling::scaled(depth_ * 20);
    
    if (is_selected_) {
        setStyleSheet(QString(
            "QPushButton { text-align: left; padding: 6px 10px 6px %1px; "
            "background-color: #e8f4fc; border: 2px solid #0078d4; "
            "border-radius: 4px; color: #0078d4; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background-color: #d0ebf9; }"
        ).arg(left_margin + 10));
    } else if (!node_->exists) {
        setStyleSheet(QString(
            "QPushButton { text-align: left; padding: 6px 10px 6px %1px; "
            "background-color: #fff8e1; border: 1px dashed #ffc107; "
            "border-radius: 4px; color: #333; font-size: 12px; }"
            "QPushButton:hover { background-color: #fff3cd; border-color: #e0a800; }"
        ).arg(left_margin + 10));
    } else {
        setStyleSheet(QString(
            "QPushButton { text-align: left; padding: 6px 10px 6px %1px; "
            "background-color: #ffffff; border: 1px solid #cccccc; "
            "border-radius: 4px; color: #333; font-size: 12px; }"
            "QPushButton:hover { background-color: #f5f5f5; border-color: #999; }"
        ).arg(left_margin + 10));
    }
}

void FolderButton::contextMenuEvent(QContextMenuEvent* event) {
    emit folder_right_clicked(node_->path, event->globalPos());
    event->accept();
}

// === MindMapView Implementation ===

MindMapView::MindMapView(QWidget* parent)
    : QWidget(parent)
    , scroll_area_(new QScrollArea(this))
    , content_widget_(nullptr)
    , content_layout_(nullptr)
    , model_(nullptr)
    , add_button_(nullptr) {
    
    auto* outer_layout = new QVBoxLayout(this);
    outer_layout->setContentsMargins(0, 0, 0, 0);
    outer_layout->setSpacing(0);
    
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setStyleSheet("QScrollArea { background-color: #fafafa; }");
    outer_layout->addWidget(scroll_area_);
    
    setMinimumHeight(ui::scaling::scaled(200));
}

MindMapView::~MindMapView() = default;

void MindMapView::set_model(FolderTreeModel* model) {
    model_ = model;
    
    if (model_) {
        connect(model_, &FolderTreeModel::folder_structure_changed, 
                this, &MindMapView::refresh_layout);
        
        refresh_layout();
    }
}

void MindMapView::refresh_layout() {
    if (!model_) return;
    
    // Clear existing
    buttons_.clear();
    add_button_ = nullptr;
    
    // Create new content widget
    content_widget_ = new QWidget();
    content_layout_ = new QVBoxLayout(content_widget_);
    content_layout_->setContentsMargins(8, 8, 8, 8);
    content_layout_->setSpacing(4);
    
    // Build the folder list
    build_folder_list();
    
    // Add the "+" button
    add_button_ = new QPushButton("+ Add Folder");
    add_button_->setFixedHeight(ui::scaling::scaled(34));
    add_button_->setCursor(Qt::PointingHandCursor);
    add_button_->setStyleSheet(
        "QPushButton { text-align: center; padding: 6px; "
        "background-color: #27ae60; border: none; "
        "border-radius: 4px; color: white; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #2ecc71; }"
    );
    connect(add_button_, &QPushButton::clicked, this, &MindMapView::add_folder_requested);
    content_layout_->addWidget(add_button_);
    
    content_layout_->addStretch();
    
    // Set the new content widget
    scroll_area_->setWidget(content_widget_);
}

void MindMapView::build_folder_list() {
    if (!model_ || !model_->root_node()) return;
    add_folder_node(model_->root_node(), 0);
}

void MindMapView::add_folder_node(FolderNode* node, int depth) {
    auto* btn = new FolderButton(node, depth, content_widget_);
    buttons_[node->path] = btn;
    content_layout_->addWidget(btn);
    
    connect(btn, &FolderButton::folder_clicked, this, &MindMapView::folder_clicked);
    connect(btn, &FolderButton::folder_right_clicked, this, &MindMapView::folder_context_menu);
    
    // Add children recursively
    for (const auto& child : node->children) {
        add_folder_node(child.get(), depth + 1);
    }
}

void MindMapView::zoom_in() {
    // No-op for widget-based view
}

void MindMapView::zoom_out() {
    // No-op for widget-based view
}

void MindMapView::zoom_fit() {
    // No-op for widget-based view
}

void MindMapView::set_selected_folder(const QString& path) {
    for (auto it = buttons_.begin(); it != buttons_.end(); ++it) {
        it.value()->set_selected(it.key() == path);
    }
}
