// ImagePreviewWindow.cpp
// Popup window for viewing images separately

#include "ImagePreviewWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>

ImagePreviewWindow::ImagePreviewWindow(QWidget* parent)
    : QDialog(parent)
    , zoom_factor_(1.0)
{
    setWindowTitle("Image Preview");
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    // Default size - 80% of screen
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int width = static_cast<int>(screenGeometry.width() * 0.8);
        int height = static_cast<int>(screenGeometry.height() * 0.8);
        resize(width, height);
    } else {
        resize(1024, 768);
    }
    
    setup_ui();
}

void ImagePreviewWindow::setup_ui() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    
    // Navigation
    prev_btn_ = new QPushButton("< Prev");
    prev_btn_->setToolTip("Previous image (Left Arrow)");
    toolbar->addWidget(prev_btn_);
    
    next_btn_ = new QPushButton("Next >");
    next_btn_->setToolTip("Next image (Right Arrow)");
    toolbar->addWidget(next_btn_);
    
    toolbar->addSpacing(20);
    
    // Zoom controls
    zoom_out_btn_ = new QPushButton("-");
    zoom_out_btn_->setFixedWidth(30);
    zoom_out_btn_->setToolTip("Zoom out");
    toolbar->addWidget(zoom_out_btn_);
    
    zoom_slider_ = new QSlider(Qt::Horizontal);
    zoom_slider_->setRange(10, 500);  // 10% to 500%
    zoom_slider_->setValue(100);
    zoom_slider_->setFixedWidth(150);
    toolbar->addWidget(zoom_slider_);
    
    zoom_in_btn_ = new QPushButton("+");
    zoom_in_btn_->setFixedWidth(30);
    zoom_in_btn_->setToolTip("Zoom in");
    toolbar->addWidget(zoom_in_btn_);
    
    zoom_label_ = new QLabel("100%");
    zoom_label_->setFixedWidth(50);
    toolbar->addWidget(zoom_label_);
    
    toolbar->addSpacing(10);
    
    fit_btn_ = new QPushButton("Fit");
    fit_btn_->setToolTip("Fit image to window");
    toolbar->addWidget(fit_btn_);
    
    actual_btn_ = new QPushButton("1:1");
    actual_btn_->setToolTip("Actual size (100%)");
    toolbar->addWidget(actual_btn_);
    
    toolbar->addStretch();
    
    // File info
    file_info_label_ = new QLabel();
    file_info_label_->setStyleSheet("color: #666;");
    toolbar->addWidget(file_info_label_);
    
    layout->addLayout(toolbar);

    // Image display
    scroll_area_ = new QScrollArea();
    scroll_area_->setWidgetResizable(false);
    scroll_area_->setAlignment(Qt::AlignCenter);
    scroll_area_->setStyleSheet("QScrollArea { background-color: #1a1a1a; }");
    
    image_label_ = new QLabel();
    image_label_->setAlignment(Qt::AlignCenter);
    image_label_->setStyleSheet("background-color: transparent;");
    scroll_area_->setWidget(image_label_);
    
    layout->addWidget(scroll_area_, 1);

    // Connections
    connect(prev_btn_, &QPushButton::clicked, this, &ImagePreviewWindow::previous_requested);
    connect(next_btn_, &QPushButton::clicked, this, &ImagePreviewWindow::next_requested);
    connect(zoom_in_btn_, &QPushButton::clicked, this, &ImagePreviewWindow::on_zoom_in);
    connect(zoom_out_btn_, &QPushButton::clicked, this, &ImagePreviewWindow::on_zoom_out);
    connect(fit_btn_, &QPushButton::clicked, this, &ImagePreviewWindow::on_fit_to_window);
    connect(actual_btn_, &QPushButton::clicked, this, &ImagePreviewWindow::on_actual_size);
    connect(zoom_slider_, &QSlider::valueChanged, this, &ImagePreviewWindow::on_zoom_slider_changed);
}

void ImagePreviewWindow::set_image(const QString& file_path) {
    current_path_ = file_path;
    
    if (!original_pixmap_.load(file_path)) {
        image_label_->setText("Failed to load image");
        file_info_label_->setText(file_path);
        return;
    }
    
    // Update file info
    QFileInfo info(file_path);
    QString size_str;
    qint64 size = info.size();
    if (size < 1024) {
        size_str = QString::number(size) + " B";
    } else if (size < 1024 * 1024) {
        size_str = QString::number(size / 1024.0, 'f', 1) + " KB";
    } else {
        size_str = QString::number(size / (1024.0 * 1024.0), 'f', 1) + " MB";
    }
    
    file_info_label_->setText(QString("%1  |  %2 x %3  |  %4")
        .arg(info.fileName())
        .arg(original_pixmap_.width())
        .arg(original_pixmap_.height())
        .arg(size_str));
    
    // Fit to window by default
    on_fit_to_window();
}

void ImagePreviewWindow::update_image_display() {
    if (original_pixmap_.isNull()) return;
    
    QSize new_size = original_pixmap_.size() * zoom_factor_;
    QPixmap scaled = original_pixmap_.scaled(new_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    image_label_->setPixmap(scaled);
    image_label_->resize(scaled.size());
    
    // Update zoom label
    zoom_label_->setText(QString::number(static_cast<int>(zoom_factor_ * 100)) + "%");
    
    // Update slider without triggering signal
    zoom_slider_->blockSignals(true);
    zoom_slider_->setValue(static_cast<int>(zoom_factor_ * 100));
    zoom_slider_->blockSignals(false);
}

void ImagePreviewWindow::set_zoom(double factor) {
    zoom_factor_ = qBound(kMinZoom, factor, kMaxZoom);
    update_image_display();
}

void ImagePreviewWindow::on_zoom_in() {
    set_zoom(zoom_factor_ + kZoomStep);
}

void ImagePreviewWindow::on_zoom_out() {
    set_zoom(zoom_factor_ - kZoomStep);
}

void ImagePreviewWindow::on_fit_to_window() {
    if (original_pixmap_.isNull()) return;
    
    QSize viewport_size = scroll_area_->viewport()->size();
    double width_ratio = static_cast<double>(viewport_size.width()) / original_pixmap_.width();
    double height_ratio = static_cast<double>(viewport_size.height()) / original_pixmap_.height();
    
    set_zoom(qMin(width_ratio, height_ratio) * 0.95);  // 95% to leave some margin
}

void ImagePreviewWindow::on_actual_size() {
    set_zoom(1.0);
}

void ImagePreviewWindow::on_zoom_slider_changed(int value) {
    set_zoom(value / 100.0);
}

void ImagePreviewWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Left:
            emit previous_requested();
            break;
        case Qt::Key_Right:
            emit next_requested();
            break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            on_zoom_in();
            break;
        case Qt::Key_Minus:
            on_zoom_out();
            break;
        case Qt::Key_0:
            on_actual_size();
            break;
        case Qt::Key_F:
            on_fit_to_window();
            break;
        case Qt::Key_Escape:
            close();
            break;
        default:
            QDialog::keyPressEvent(event);
    }
}

void ImagePreviewWindow::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom with Ctrl+Wheel
        double delta = event->angleDelta().y() / 1200.0;  // Small increments
        set_zoom(zoom_factor_ + delta);
        event->accept();
    } else {
        QDialog::wheelEvent(event);
    }
}

void ImagePreviewWindow::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    // Re-fit the image when resizing to ensure it displays correctly
    // Only re-fit if we have a valid pixmap and the window is visible
    if (!original_pixmap_.isNull() && isVisible()) {
        on_fit_to_window();
    }
}
