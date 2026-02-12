#include "MindMapView.hpp"
#include "FolderTreeModel.hpp"
#include "ui_constants.hpp"
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>

// === FolderButton Implementation ===

FolderButton::FolderButton(FolderNode* node, QWidget* parent)
    : QPushButton(parent)
    , node_(node)
    , is_selected_(false) {
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(ui::scaling::scaled(50));
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
    
    // Build display text with icon and count
    QString icon;
    if (node_->is_external) {
        icon = node_->exists ? "📁" : "📂";
    } else {
        icon = node_->exists ? "📁" : "✨";
    }
    QString count_str;
    if (node_->assigned_file_count > 0) {
        count_str = QString(" (%1)").arg(node_->assigned_file_count);
    }
    
    setText(icon + " " + name + count_str);
    setToolTip(node_->path);
}

void FolderButton::set_selected(bool selected) {
    is_selected_ = selected;
    update_style();
}

void FolderButton::update_style() {
    if (is_selected_) {
        setStyleSheet(
            "QPushButton { text-align: center; padding: 8px; "
            "background-color: #1a3a5c; border: 2px solid #3498db; "
            "border-radius: 6px; color: #3498db; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background-color: #1e4a6e; }"
        );
    } else if (!node_->exists) {
        // Virtual folder — dashed border
        setStyleSheet(
            "QPushButton { text-align: center; padding: 8px; "
            "background-color: #3a3520; border: 1px dashed #f39c12; "
            "border-radius: 6px; color: #f39c12; font-size: 12px; }"
            "QPushButton:hover { background-color: #4a4530; }"
        );
    } else if (node_->is_external) {
        // External folder — purple accent
        setStyleSheet(
            "QPushButton { text-align: center; padding: 8px; "
            "background-color: #2d1f3d; border: 1px solid #9b59b6; "
            "border-radius: 6px; color: #bb6bd9; font-size: 12px; }"
            "QPushButton:hover { background-color: #3d2f4d; }"
        );
    } else {
        // Normal folder — dark theme
        setStyleSheet(
            "QPushButton { text-align: center; padding: 8px; "
            "background-color: #34495e; border: 1px solid #4a6078; "
            "border-radius: 6px; color: #ecf0f1; font-size: 12px; }"
            "QPushButton:hover { background-color: #3d566e; border-color: #5a7a98; }"
        );
    }
}

void FolderButton::contextMenuEvent(QContextMenuEvent* event) {
    emit folder_right_clicked(node_->path, event->globalPos());
    event->accept();
}

void FolderButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        drag_start_pos_ = event->pos();
    }
    QPushButton::mousePressEvent(event);
}

void FolderButton::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton)) return;
    if ((event->pos() - drag_start_pos_).manhattanLength() < 20) return;
    
    auto* drag = new QDrag(this);
    auto* mime = new QMimeData();
    mime->setText(node_->path);
    drag->setMimeData(mime);
    drag->exec(Qt::MoveAction);
}

// === MindMapView Implementation ===

MindMapView::MindMapView(QWidget* parent)
    : QWidget(parent)
    , scroll_area_(new QScrollArea(this))
    , content_widget_(nullptr)
    , grid_layout_(nullptr)
    , model_(nullptr)
    , add_button_(nullptr)
    , next_row_(0)
    , next_col_(0) {
    
    auto* outer_layout = new QVBoxLayout(this);
    outer_layout->setContentsMargins(0, 0, 0, 0);
    outer_layout->setSpacing(0);
    
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setStyleSheet("QScrollArea { background-color: #2c3e50; border-radius: 4px; }");
    outer_layout->addWidget(scroll_area_);
    
    setAcceptDrops(true);
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
    grid_positions_.clear();
    next_row_ = 0;
    next_col_ = 0;
    
    // Create new content widget
    content_widget_ = new QWidget();
    content_widget_->setStyleSheet("background-color: #2c3e50;");
    grid_layout_ = new QGridLayout(content_widget_);
    grid_layout_->setContentsMargins(10, 10, 10, 10);
    grid_layout_->setSpacing(8);
    
    // Build the grid
    build_grid();
    
    // Add the "+" button in the next available slot
    add_button_ = new QPushButton("+ Add Folder");
    add_button_->setFixedHeight(ui::scaling::scaled(50));
    add_button_->setCursor(Qt::PointingHandCursor);
    add_button_->setStyleSheet(
        "QPushButton { text-align: center; padding: 8px; "
        "background-color: #27ae60; border: none; "
        "border-radius: 6px; color: white; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #2ecc71; }"
    );
    connect(add_button_, &QPushButton::clicked, this, &MindMapView::add_folder_requested);
    grid_layout_->addWidget(add_button_, next_row_, next_col_);
    
    // Set the new content widget
    scroll_area_->setWidget(content_widget_);
}

void MindMapView::build_grid() {
    if (!model_ || !model_->root_node()) return;
    
    FolderNode* root = model_->root_node();
    
    // Root folder gets its own row spanning all columns
    auto* root_btn = new FolderButton(root, content_widget_);
    root_btn->setStyleSheet(
        "QPushButton { text-align: center; padding: 10px; "
        "background-color: #1a252f; border: 2px solid #3498db; "
        "border-radius: 8px; color: #3498db; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #1e2f3d; }"
    );
    root_btn->setFixedHeight(ui::scaling::scaled(55));
    buttons_[root->path] = root_btn;
    grid_positions_[root->path] = {0, 0};
    grid_layout_->addWidget(root_btn, 0, 0, 1, kGridColumns);
    
    connect(root_btn, &FolderButton::folder_clicked, this, &MindMapView::folder_clicked);
    connect(root_btn, &FolderButton::folder_right_clicked, this, &MindMapView::folder_context_menu);
    
    // Child folders in a grid starting from row 1
    next_row_ = 1;
    next_col_ = 0;
    
    for (const auto& child : root->children) {
        place_folder_node(child.get());
    }
}

void MindMapView::place_folder_node(FolderNode* node) {
    auto* btn = new FolderButton(node, content_widget_);
    buttons_[node->path] = btn;
    grid_positions_[node->path] = {next_row_, next_col_};
    grid_layout_->addWidget(btn, next_row_, next_col_);
    
    connect(btn, &FolderButton::folder_clicked, this, &MindMapView::folder_clicked);
    connect(btn, &FolderButton::folder_right_clicked, this, &MindMapView::folder_context_menu);
    
    // Advance to next grid position
    next_col_++;
    if (next_col_ >= kGridColumns) {
        next_col_ = 0;
        next_row_++;
    }
    
    // Place children recursively (they go into the grid too, no indenting)
    for (const auto& child : node->children) {
        place_folder_node(child.get());
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

void MindMapView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void MindMapView::dropEvent(QDropEvent* event) {
    if (!event->mimeData()->hasText() || !model_ || !grid_layout_) return;
    
    QString dragged_path = event->mimeData()->text();
    
    // Find the button under the drop position to swap with
    QWidget* target_widget = content_widget_->childAt(
        content_widget_->mapFrom(this, event->position().toPoint()));
    auto* target_btn = qobject_cast<FolderButton*>(target_widget);
    
    if (!target_btn || target_btn->node()->path == dragged_path) return;
    if (target_btn->node()->path == model_->root_node()->path) return;  // Can't swap with root
    
    QString target_path = target_btn->node()->path;
    
    // Swap positions in the grid
    if (grid_positions_.contains(dragged_path) && grid_positions_.contains(target_path)) {
        auto pos_a = grid_positions_[dragged_path];
        auto pos_b = grid_positions_[target_path];
        
        // Remove both from layout
        grid_layout_->removeWidget(buttons_[dragged_path]);
        grid_layout_->removeWidget(buttons_[target_path]);
        
        // Re-add in swapped positions
        grid_layout_->addWidget(buttons_[dragged_path], pos_b.first, pos_b.second);
        grid_layout_->addWidget(buttons_[target_path], pos_a.first, pos_a.second);
        
        // Update position map
        grid_positions_[dragged_path] = pos_b;
        grid_positions_[target_path] = pos_a;
    }
    
    event->acceptProposedAction();
}
