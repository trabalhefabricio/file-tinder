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

// === AddNodeItem Implementation ===

AddNodeItem::AddNodeItem(QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , is_hovered_(false) {
    setAcceptHoverEvents(true);
    setCursor(Qt::PointingHandCursor);
}

QRectF AddNodeItem::boundingRect() const {
    return QRectF(0, 0, ui::dimensions::kAddNodeWidth, ui::dimensions::kAddNodeHeight);
}

void AddNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Circle with "+" inside
    QColor bg_color = is_hovered_ ? QColor(ui::colors::kAddNodeHover) : QColor(ui::colors::kAddNodeBg);
    
    QRectF rect = boundingRect().adjusted(2, 2, -2, -2);
    
    painter->setPen(Qt::NoPen);
    painter->setBrush(bg_color);
    painter->drawEllipse(rect);
    
    // Draw "+" text
    painter->setPen(Qt::white);
    QFont font;
    font.setPointSize(24);
    font.setBold(true);
    painter->setFont(font);
    painter->drawText(rect, Qt::AlignCenter, "+");
}

void AddNodeItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QGraphicsObject::mousePressEvent(event);
}

void AddNodeItem::hoverEnterEvent(QGraphicsSceneHoverEvent*) {
    is_hovered_ = true;
    update();
}

void AddNodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    is_hovered_ = false;
    update();
}

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
    
    // Windows-like sleek styling - light background with darker text
    QColor bg_color("#ffffff");
    QColor border_color("#cccccc");
    QColor text_color("#333333");
    
    if (is_selected_) {
        bg_color = QColor("#e8f4fc");
        border_color = QColor("#0078d4");  // Windows blue
        text_color = QColor("#0078d4");
    } else if (is_hovered_) {
        bg_color = QColor("#f5f5f5");
        border_color = QColor("#999999");
    } else if (!node_->exists) {
        bg_color = QColor("#fff8e1");  // Light yellow for virtual
        border_color = QColor("#ffc107");
    }
    
    // Main rectangle - clean, flat, Windows-like
    QRectF rect = boundingRect().adjusted(1, 1, -1, -1);
    
    // Simple border
    QPen border_pen(border_color, is_selected_ ? 2 : 1);
    if (!node_->exists) {
        border_pen.setStyle(Qt::DashLine);
    }
    painter->setPen(border_pen);
    painter->setBrush(bg_color);
    painter->drawRect(rect);
    
    // Folder icon (simple text-based)
    painter->setPen(text_color);
    QFont icon_font;
    icon_font.setPointSize(14);
    painter->setFont(icon_font);
    QString icon_text = node_->exists ? "[D]" : "[+]";
    painter->drawText(QRectF(6, 0, 30, rect.height()), Qt::AlignVCenter | Qt::AlignLeft, icon_text);
    
    // Display name - clean text
    QFont name_font;
    name_font.setPointSize(11);
    name_font.setBold(is_selected_);
    painter->setFont(name_font);
    QFontMetrics fm2(name_font);
    QString display_name = node_->display_name;
    if (display_name.isEmpty()) {
        display_name = QFileInfo(node_->path).fileName();
    }
    QString elided = fm2.elidedText(display_name, Qt::ElideMiddle, 
                                   static_cast<int>(rect.width()) - 80);
    painter->drawText(QRectF(36, 0, rect.width() - 80, rect.height()), 
                      Qt::AlignLeft | Qt::AlignVCenter, elided);
    
    // File count badge - simple text suffix
    if (node_->assigned_file_count > 0) {
        QString count_text = QString(" (%1)").arg(node_->assigned_file_count);
        QColor badge_color = is_selected_ ? QColor("#0078d4") : QColor("#666666");
        painter->setPen(badge_color);
        QFont count_font;
        count_font.setPointSize(9);
        painter->setFont(count_font);
        painter->drawText(QRectF(rect.width() - 40, 0, 36, rect.height()), 
                         Qt::AlignRight | Qt::AlignVCenter, count_text);
    }
    
    // Expand/collapse indicator for children - simplified [+]/[-] style
    if (!node_->children.empty()) {
        QString expand_text = is_expanded_ ? "[-]" : "[+]";
        painter->setPen(QColor("#666666"));
        QFont expand_font;
        expand_font.setPointSize(9);
        painter->setFont(expand_font);
        painter->drawText(QRectF(rect.width() - 25, 0, 22, rect.height()), 
                         Qt::AlignCenter, expand_text);
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
    
    QPen pen{QColor{QString{ui::colors::kConnectionLine}}};
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
    , add_node_(nullptr)
    , is_panning_(false)
    , current_scale_(1.0) {
    
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::NoDrag);
    
    // Light background for Windows-like look
    setBackgroundBrush(QColor(250, 250, 250));
    
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
    add_node_ = nullptr;
    
    // Build tree
    build_tree();
    
    // Layout nodes
    layout_nodes();
    
    // Create connections
    create_connections();
    
    // Create the "+" add node
    create_add_node();
    
    // Fit in view
    scene_->setSceneRect(scene_->itemsBoundingRect().adjusted(-50, -50, 50, 50));
}

void MindMapView::build_tree() {
    if (!model_ || !model_->root_node()) return;
    
    int y_offset = 30;
    create_node_item(model_->root_node(), 0, y_offset);
}

void MindMapView::create_add_node() {
    // Create "+" node connected to root
    add_node_ = new AddNodeItem();
    scene_->addItem(add_node_);
    
    // Position next to the last node or root
    qreal x = 30;
    qreal y = 30;
    
    if (model_ && model_->root_node()) {
        auto it = node_items_.find(model_->root_node()->path);
        if (it != node_items_.end()) {
            QPointF root_pos = it.value()->pos();
            x = root_pos.x() + ui::dimensions::kNodeWidth + ui::dimensions::kNodeSpacingHorizontal;
            y = root_pos.y();
        }
    }
    
    add_node_->setPos(x, y);
    
    // Connect signal
    connect(add_node_, &AddNodeItem::clicked, this, &MindMapView::add_folder_requested);
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
