#ifndef FOLDER_NODE_WIDGET_HPP
#define FOLDER_NODE_WIDGET_HPP

#include <QWidget>
#include <QLabel>
#include <QString>
#include <QPropertyAnimation>

class FolderNodeWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPoint position READ pos WRITE move)
    Q_PROPERTY(qreal scale READ get_scale WRITE set_scale)

public:
    explicit FolderNodeWidget(const QString& folder_path, const QString& display_name,
                              bool is_virtual = false, QWidget* parent = nullptr);
    ~FolderNodeWidget() override;
    
    QString folder_path() const { return folder_path_; }
    QString display_name() const { return display_name_; }
    bool is_virtual() const { return is_virtual_; }
    bool is_selected() const { return is_selected_; }
    bool is_connected() const { return is_connected_; }
    int file_count() const { return file_count_; }
    
    void set_selected(bool selected);
    void set_connected(bool connected);
    void set_file_count(int count);
    void set_virtual(bool is_virtual);
    
    qreal get_scale() const { return scale_; }
    void set_scale(qreal scale);
    
    QPoint center_point() const;
    
signals:
    void clicked(const QString& folder_path);
    void double_clicked(const QString& folder_path);
    void right_clicked(const QString& folder_path, const QPoint& pos);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    QString folder_path_;
    QString display_name_;
    bool is_virtual_;
    bool is_selected_;
    bool is_connected_;
    int file_count_;
    qreal scale_;
    bool is_hovered_;
    
    QPropertyAnimation* hover_animation_;
    
    void update_appearance();
    QColor get_background_color() const;
};

#endif // FOLDER_NODE_WIDGET_HPP
