// ImagePreviewWindow.hpp
// Popup window for viewing images separately

#ifndef IMAGE_PREVIEW_WINDOW_HPP
#define IMAGE_PREVIEW_WINDOW_HPP

#include <QDialog>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QPixmap>

class ImagePreviewWindow : public QDialog {
    Q_OBJECT

public:
    explicit ImagePreviewWindow(QWidget* parent = nullptr);

    // Set the image to display
    void set_image(const QString& file_path);
    
    // Get current file path
    QString get_current_path() const { return current_path_; }

signals:
    void next_requested();
    void previous_requested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void on_zoom_in();
    void on_zoom_out();
    void on_fit_to_window();
    void on_actual_size();
    void on_zoom_slider_changed(int value);

private:
    void setup_ui();
    void update_image_display();
    void set_zoom(double factor);

    QLabel* image_label_;
    QScrollArea* scroll_area_;
    QPushButton* zoom_in_btn_;
    QPushButton* zoom_out_btn_;
    QPushButton* fit_btn_;
    QPushButton* actual_btn_;
    QPushButton* prev_btn_;
    QPushButton* next_btn_;
    QSlider* zoom_slider_;
    QLabel* zoom_label_;
    QLabel* file_info_label_;

    QString current_path_;
    QPixmap original_pixmap_;
    double zoom_factor_;
    
    static constexpr double kMinZoom = 0.1;
    static constexpr double kMaxZoom = 5.0;
    static constexpr double kZoomStep = 0.1;
};

#endif // IMAGE_PREVIEW_WINDOW_HPP
