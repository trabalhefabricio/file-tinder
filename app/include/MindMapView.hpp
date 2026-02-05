#ifndef MIND_MAP_VIEW_HPP
#define MIND_MAP_VIEW_HPP

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QGraphicsItem>
#include <QScrollBar>
#include <QMap>
#include <QString>
#include <vector>

class FolderTreeModel;
class FolderNode;

class FolderNodeItem : public QGraphicsObject {
    Q_OBJECT

public:
    FolderNodeItem(FolderNode* node, QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    FolderNode* node() const { return node_; }
    void set_selected(bool selected);
    void set_expanded(bool expanded);
    bool is_expanded() const { return is_expanded_; }
    void update_file_count(int count);
    
    QPointF connection_point_left() const;
    QPointF connection_point_right() const;
    
signals:
    void clicked(const QString& path);
    void double_clicked(const QString& path);
    void right_clicked(const QString& path, const QPointF& scene_pos);
    
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    
private:
    FolderNode* node_;
    bool is_selected_;
    bool is_expanded_;
    bool is_hovered_;
    int file_count_;
};

class ConnectionLineItem : public QGraphicsItem {
public:
    ConnectionLineItem(FolderNodeItem* from, FolderNodeItem* to, QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void update_positions();
    
private:
    FolderNodeItem* from_;
    FolderNodeItem* to_;
};

class MindMapView : public QGraphicsView {
    Q_OBJECT

public:
    explicit MindMapView(QWidget* parent = nullptr);
    ~MindMapView() override;
    
    void set_model(FolderTreeModel* model);
    void refresh_layout();
    void zoom_in();
    void zoom_out();
    void zoom_fit();
    void set_selected_folder(const QString& path);
    
signals:
    void folder_clicked(const QString& path);
    void folder_double_clicked(const QString& path);
    void folder_context_menu(const QString& path, const QPoint& global_pos);
    void add_folder_requested();
    void add_subfolder_requested(const QString& parent_path);
    
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    
private:
    QGraphicsScene* scene_;
    FolderTreeModel* model_;
    QMap<QString, FolderNodeItem*> node_items_;
    std::vector<ConnectionLineItem*> connection_lines_;
    
    bool is_panning_;
    QPoint last_pan_point_;
    qreal current_scale_;
    
    void build_tree();
    void layout_nodes();
    void create_connections();
    FolderNodeItem* create_node_item(FolderNode* node, int depth, int& y_offset);
    void connect_signals(FolderNodeItem* item);
};

#endif // MIND_MAP_VIEW_HPP
