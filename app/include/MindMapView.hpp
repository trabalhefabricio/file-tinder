#ifndef MIND_MAP_VIEW_HPP
#define MIND_MAP_VIEW_HPP

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QString>

class FolderTreeModel;
class FolderNode;

// Lightweight folder button used in the mind map list
class FolderButton : public QPushButton {
    Q_OBJECT

public:
    FolderButton(FolderNode* node, int depth, QWidget* parent = nullptr);
    
    FolderNode* node() const { return node_; }
    void set_selected(bool selected);
    void update_display();
    
signals:
    void folder_clicked(const QString& path);
    void folder_right_clicked(const QString& path, const QPoint& global_pos);
    
protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    
private:
    FolderNode* node_;
    int depth_;
    bool is_selected_;
    
    void update_style();
};

// Lightweight mind map view using native widgets instead of QGraphicsView
class MindMapView : public QWidget {
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
    
private:
    QScrollArea* scroll_area_;
    QWidget* content_widget_;
    QVBoxLayout* content_layout_;
    FolderTreeModel* model_;
    QMap<QString, FolderButton*> buttons_;
    QPushButton* add_button_;
    
    void build_folder_list();
    void add_folder_node(FolderNode* node, int depth);
};

#endif // MIND_MAP_VIEW_HPP
