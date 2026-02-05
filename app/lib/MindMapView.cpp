#include "MindMapView.hpp"
#include "FolderTreeModel.hpp"
#include "ui_constants.hpp"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QFileInfo>
#include <cmath>

// === FolderNodeItem Implementation ===

FolderNodeItem::FolderNodeItem(FolderNode* node, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , node_(node)
    , is_selected_(false)
    , is_expanded_(true)
    , is_hovered_(false)
    , file_count_(node->assigned_file_count) {
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setCursor(Qt::PointingHandCursor);
}

QRectF FolderNodeItem::boundingRect() const {
    return QRectF(0, 0, ui::dimensions::kNodeWidth, ui::dimensions::kNodeHeight);
}

void FolderNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Determine background color
    QColor bg_color(ui::colors::kNodeDefaultBg);
    if (is_selected_) {
        bg_color = QColor(ui::colors::kNodeSelectedBg);
    } else if (node_->is_connected) {
        bg_color = QColor(ui::colors::kNodeConnectedBg);
    } else if (!node_->exists) {
        bg_color = QColor(ui::colors::kNodeVirtualBg);
    }
    
    // Draw shadow if hovered
    if (is_hovered_) {
        painter->setBrush(QColor(0, 0, 0, 30));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(boundingRect().adjusted(3, 3, 3, 3), 
                                 ui::dimensions::kNodeBorderRadius,
                                 ui::dimensions::kNodeBorderRadius);
    }
    
    // Main rectangle
    QRectF rect = boundingRect().adjusted(1, 1, -1, -1);
    
    // Border style
    if (!node_->exists) {
        QPen pen(bg_color.darker(120));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(2);
        painter->setPen(pen);
    } else {
        painter->setPen(QPen(bg_color.darker(130), 2));
    }
    
    // Scale effect on hover
    if (is_hovered_) {
        qreal scale = 1.03;
        painter->save();
        painter->translate(rect.center());
        painter->scale(scale, scale);
        painter->translate(-rect.center());
    }
    
    painter->setBrush(bg_color);
    painter->drawRoundedRect(rect, ui::dimensions::kNodeBorderRadius, ui::dimensions::kNodeBorderRadius);
    
    // Folder icon
    painter->setPen(Qt::white);
    QFont icon_font("Segoe UI Symbol", 22);
    painter->setFont(icon_font);
    QString icon = node_->exists ? "📁" : "📂";
    painter->drawText(QRectF(8, 8, 45, 45), Qt::AlignCenter, icon);
    
    // Display name
    QFont name_font("Segoe UI", ui::fonts::kNodeTitleSize, QFont::Bold);
    painter->setFont(name_font);
    QFontMetrics fm(name_font);
    QString display_name = node_->display_name;
    if (display_name.isEmpty()) {
        display_name = QFileInfo(node_->path).fileName();
    }
    QString elided = fm.elidedText(display_name, Qt::ElideMiddle, 
                                   static_cast<int>(rect.width()) - 65);
    painter->drawText(QRectF(55, 10, rect.width() - 65, 28), 
                      Qt::AlignLeft | Qt::AlignVCenter, elided);
    
    // Path hint
    QFont path_font("Segoe UI", ui::fonts::kNodeSubtitleSize);
    painter->setFont(path_font);
    painter->setPen(QColor(255, 255, 255, 170));
    QFontMetrics fm2(path_font);
    QString short_path = node_->path;
    if (short_path.length() > 35) {
        short_path = "..." + short_path.right(32);
    }
    QString elided_path = fm2.elidedText(short_path, Qt::ElideMiddle, 
                                         static_cast<int>(rect.width()) - 65);
    painter->drawText(QRectF(55, 38, rect.width() - 65, 22), 
                      Qt::AlignLeft | Qt::AlignVCenter, elided_path);
    
    // File count badge
    if (node_->assigned_file_count > 0) {
        QRectF badge(rect.width() - 38, 8, 32, 22);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 255, 255, 220));
        painter->drawRoundedRect(badge, 11, 11);
        
        painter->setPen(bg_color.darker(140));
        QFont badge_font("Segoe UI", 11, QFont::Bold);
        painter->setFont(badge_font);
        painter->drawText(badge, Qt::AlignCenter, 
                          QString::number(node_->assigned_file_count));
    }
    
    // Connection indicator
    if (node_->is_connected) {
        painter->setBrush(Qt::white);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(rect.width() - 14, rect.height() - 14), 7, 7);
        
        // Link icon inside
        painter->setPen(QPen(bg_color, 1.5));
        painter->drawLine(QPointF(rect.width() - 17, rect.height() - 14),
                         QPointF(rect.width() - 11, rect.height() - 14));
    }
    
    // Expand/collapse indicator if has children
    if (!node_->children.empty()) {
        painter->setBrush(QColor(255, 255, 255, 200));
        painter->setPen(Qt::NoPen);
        QRectF exp_rect(rect.width() - 22, rect.height() / 2 - 8, 16, 16);
        painter->drawEllipse(exp_rect);
        
        QPen indicator_pen(bg_color.darker(120), 2);
        painter->setPen(indicator_pen);
        // Draw horizontal line for both states (the minus part)
        painter->drawLine(exp_rect.center() + QPointF(-4, 0),
                        exp_rect.center() + QPointF(4, 0));
        // Add vertical line only when collapsed to form a plus sign
        if (!is_expanded_) {
            painter->drawLine(exp_rect.center() + QPointF(0, -4),
                            exp_rect.center() + QPointF(0, 4));
        }
    }
    
    if (is_hovered_) {
        painter->restore();
    }
}

void FolderNodeItem::set_selected(bool selected) {
    is_selected_ = selected;
    update();
}

void FolderNodeItem::set_expanded(bool expanded) {
    is_expanded_ = expanded;
    update();
}

void FolderNodeItem::update_file_count(int count) {
    file_count_ = count;
    update();
}

QPointF FolderNodeItem::connection_point_left() const {
    return scenePos() + QPointF(0, boundingRect().height() / 2);
}

QPointF FolderNodeItem::connection_point_right() const {
    return scenePos() + QPointF(boundingRect().width(), boundingRect().height() / 2);
}

void FolderNodeItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(node_->path);
    } else if (event->button() == Qt::RightButton) {
        emit right_clicked(node_->path, event->scenePos());
    }
    QGraphicsObject::mousePressEvent(event);
}

void FolderNodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit double_clicked(node_->path);
    }
    QGraphicsObject::mouseDoubleClickEvent(event);
}

void FolderNodeItem::hoverEnterEvent(QGraphicsSceneHoverEvent*) {
    is_hovered_ = true;
    update();
}

void FolderNodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    is_hovered_ = false;
    update();
}

// === ConnectionLineItem Implementation ===

ConnectionLineItem::ConnectionLineItem(FolderNodeItem* from, FolderNodeItem* to, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , from_(from)
    , to_(to) {
    setZValue(-1); // Draw behind nodes
}

QRectF ConnectionLineItem::boundingRect() const {
    QPointF p1 = from_->connection_point_right();
    QPointF p2 = to_->connection_point_left();
    return QRectF(p1, p2).normalized().adjusted(-5, -5, 5, 5);
}

void ConnectionLineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setRenderHint(QPainter::Antialiasing);
    
    QPointF start = from_->connection_point_right();
    QPointF end = to_->connection_point_left();
    
    // Draw curved bezier line
    QPainterPath path;
    path.moveTo(start);
    
    qreal ctrl_offset = std::abs(end.x() - start.x()) * 0.5;
    QPointF ctrl1(start.x() + ctrl_offset, start.y());
    QPointF ctrl2(end.x() - ctrl_offset, end.y());
    
    path.cubicTo(ctrl1, ctrl2, end);
    
    QPen pen(QColor(ui::colors::kConnectionLine));
    pen.setWidth(2);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->drawPath(path);
    
    // Draw arrow at end
    QPointF arrow_tip = end;
    qreal arrow_size = 8;
    qreal angle = std::atan2(end.y() - ctrl2.y(), end.x() - ctrl2.x());
    
    QPointF arrow_p1 = arrow_tip - QPointF(
        std::cos(angle - M_PI / 6) * arrow_size,
        std::sin(angle - M_PI / 6) * arrow_size
    );
    QPointF arrow_p2 = arrow_tip - QPointF(
        std::cos(angle + M_PI / 6) * arrow_size,
        std::sin(angle + M_PI / 6) * arrow_size
    );
    
    painter->setBrush(QColor(ui::colors::kConnectionLine));
    QPolygonF arrow;
    arrow << arrow_tip << arrow_p1 << arrow_p2;
    painter->drawPolygon(arrow);
}

void ConnectionLineItem::update_positions() {
    prepareGeometryChange();
    update();
}

// === MindMapView Implementation ===

MindMapView::MindMapView(QWidget* parent)
    : QGraphicsView(parent)
    , scene_(new QGraphicsScene(this))
    , model_(nullptr)
    , is_panning_(false)
    , current_scale_(1.0) {
    
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::NoDrag);
    
    // Dark background
    setBackgroundBrush(QColor(45, 52, 60));
    
    // Set minimum size
    setMinimumSize(ui::dimensions::kFolderTreePanelMinWidth, 400);
}

MindMapView::~MindMapView() = default;

void MindMapView::set_model(FolderTreeModel* model) {
    model_ = model;
    
    if (model_) {
        connect(model_, &FolderTreeModel::folder_structure_changed, 
                this, &MindMapView::refresh_layout);
        connect(model_, &FolderTreeModel::dataChanged,
                this, [this]() { scene_->update(); });
        
        refresh_layout();
    }
}

void MindMapView::refresh_layout() {
    if (!model_) return;
    
    // Clear existing items
    scene_->clear();
    node_items_.clear();
    connection_lines_.clear();
    
    // Build tree
    build_tree();
    
    // Layout nodes
    layout_nodes();
    
    // Create connections
    create_connections();
    
    // Fit in view
    scene_->setSceneRect(scene_->itemsBoundingRect().adjusted(-50, -50, 50, 50));
}

void MindMapView::build_tree() {
    if (!model_ || !model_->root_node()) return;
    
    int y_offset = 30;
    create_node_item(model_->root_node(), 0, y_offset);
}

FolderNodeItem* MindMapView::create_node_item(FolderNode* node, int depth, int& y_offset) {
    auto* item = new FolderNodeItem(node);
    scene_->addItem(item);
    node_items_[node->path] = item;
    connect_signals(item);
    
    // Position based on depth and y_offset
    qreal x = depth * (ui::dimensions::kNodeWidth + ui::dimensions::kNodeSpacingHorizontal) + 30;
    item->setPos(x, y_offset);
    
    y_offset += ui::dimensions::kNodeHeight + ui::dimensions::kNodeSpacingVertical;
    
    // Create child items
    for (const auto& child : node->children) {
        create_node_item(child.get(), depth + 1, y_offset);
    }
    
    return item;
}

void MindMapView::layout_nodes() {
    // Already done in create_node_item for simplicity
    // Could be enhanced for more complex layouts
}

void MindMapView::create_connections() {
    if (!model_ || !model_->root_node()) return;
    
    // Create connection lines between parent and children
    std::function<void(FolderNode*)> connect_recursive = [&](FolderNode* node) {
        auto it = node_items_.find(node->path);
        if (it == node_items_.end()) return;
        
        FolderNodeItem* parent_item = it.value();
        
        for (const auto& child : node->children) {
            auto child_it = node_items_.find(child->path);
            if (child_it != node_items_.end()) {
                auto* line = new ConnectionLineItem(parent_item, child_it.value());
                scene_->addItem(line);
                connection_lines_.push_back(line);
            }
            connect_recursive(child.get());
        }
    };
    
    connect_recursive(model_->root_node());
}

void MindMapView::connect_signals(FolderNodeItem* item) {
    connect(item, &FolderNodeItem::clicked, this, &MindMapView::folder_clicked);
    connect(item, &FolderNodeItem::double_clicked, this, &MindMapView::folder_double_clicked);
    connect(item, &FolderNodeItem::right_clicked, 
            this, [this](const QString& path, const QPointF& scene_pos) {
                QPoint global_pos = mapToGlobal(mapFromScene(scene_pos));
                emit folder_context_menu(path, global_pos);
            });
}

void MindMapView::zoom_in() {
    current_scale_ *= 1.2;
    current_scale_ = std::min(current_scale_, 3.0);
    setTransform(QTransform::fromScale(current_scale_, current_scale_));
}

void MindMapView::zoom_out() {
    current_scale_ /= 1.2;
    current_scale_ = std::max(current_scale_, 0.3);
    setTransform(QTransform::fromScale(current_scale_, current_scale_));
}

void MindMapView::zoom_fit() {
    fitInView(scene_->itemsBoundingRect(), Qt::KeepAspectRatio);
    current_scale_ = transform().m11();
}

void MindMapView::set_selected_folder(const QString& path) {
    for (auto it = node_items_.begin(); it != node_items_.end(); ++it) {
        it.value()->set_selected(it.key() == path);
    }
}

void MindMapView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom
        if (event->angleDelta().y() > 0) {
            zoom_in();
        } else {
            zoom_out();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void MindMapView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier)) {
        is_panning_ = true;
        last_pan_point_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void MindMapView::mouseMoveEvent(QMouseEvent* event) {
    if (is_panning_) {
        QPoint delta = event->pos() - last_pan_point_;
        last_pan_point_ = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void MindMapView::mouseReleaseEvent(QMouseEvent* event) {
    if (is_panning_) {
        is_panning_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void MindMapView::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.addAction("Add Root Folder", this, &MindMapView::add_folder_requested);
    menu.addSeparator();
    menu.addAction("Zoom In", this, &MindMapView::zoom_in);
    menu.addAction("Zoom Out", this, &MindMapView::zoom_out);
    menu.addAction("Fit View", this, &MindMapView::zoom_fit);
    menu.exec(event->globalPos());
}
