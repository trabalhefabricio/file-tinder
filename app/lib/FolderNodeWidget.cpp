#include "FolderNodeWidget.hpp"
#include "ui_constants.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <QFontMetrics>

FolderNodeWidget::FolderNodeWidget(const QString& folder_path, const QString& display_name,
                                   bool is_virtual, QWidget* parent)
    : QWidget(parent)
    , folder_path_(folder_path)
    , display_name_(display_name)
    , is_virtual_(is_virtual)
    , is_selected_(false)
    , is_connected_(false)
    , file_count_(0)
    , scale_(1.0)
    , is_hovered_(false)
    , hover_animation_(nullptr) {
    
    setFixedSize(ui::dimensions::kNodeWidth, ui::dimensions::kNodeHeight);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    
    // Setup hover animation
    hover_animation_ = new QPropertyAnimation(this, "scale");
    hover_animation_->setDuration(150);
    hover_animation_->setEasingCurve(QEasingCurve::OutCubic);
}

FolderNodeWidget::~FolderNodeWidget() = default;

void FolderNodeWidget::set_selected(bool selected) {
    if (is_selected_ != selected) {
        is_selected_ = selected;
        update();
    }
}

void FolderNodeWidget::set_connected(bool connected) {
    if (is_connected_ != connected) {
        is_connected_ = connected;
        update();
    }
}

void FolderNodeWidget::set_file_count(int count) {
    if (file_count_ != count) {
        file_count_ = count;
        update();
    }
}

void FolderNodeWidget::set_virtual(bool is_virtual) {
    if (is_virtual_ != is_virtual) {
        is_virtual_ = is_virtual;
        update();
    }
}

void FolderNodeWidget::set_scale(qreal scale) {
    if (!qFuzzyCompare(scale_, scale)) {
        scale_ = scale;
        update();
    }
}

QPoint FolderNodeWidget::center_point() const {
    return QPoint(x() + width() / 2, y() + height() / 2);
}

QColor FolderNodeWidget::get_background_color() const {
    if (is_selected_) {
        return QColor(ui::colors::kNodeSelectedBg);
    }
    if (is_connected_) {
        return QColor(ui::colors::kNodeConnectedBg);
    }
    if (is_virtual_) {
        return QColor(ui::colors::kNodeVirtualBg);
    }
    return QColor(ui::colors::kNodeDefaultBg);
}

void FolderNodeWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Apply scale transform
    QPointF center(width() / 2.0, height() / 2.0);
    painter.translate(center);
    painter.scale(scale_, scale_);
    painter.translate(-center);
    
    // Draw shadow if hovered
    if (is_hovered_) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 40));
        painter.drawRoundedRect(rect().adjusted(4, 4, 4, 4), 
                                ui::dimensions::kNodeBorderRadius,
                                ui::dimensions::kNodeBorderRadius);
    }
    
    // Draw main rectangle
    QColor bg_color = get_background_color();
    painter.setBrush(bg_color);
    
    // Border style based on virtual status
    if (is_virtual_) {
        QPen pen(bg_color.darker(120));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(2);
        painter.setPen(pen);
    } else {
        painter.setPen(QPen(bg_color.darker(120), 2));
    }
    
    painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2),
                            ui::dimensions::kNodeBorderRadius,
                            ui::dimensions::kNodeBorderRadius);
    
    // Draw folder icon
    painter.setPen(Qt::white);
    QFont icon_font("Segoe UI Symbol", 20);
    painter.setFont(icon_font);
    painter.drawText(QRect(10, 10, 40, 40), Qt::AlignCenter, is_virtual_ ? "[D]" : "[F]");
    
    // Draw display name
    QFont name_font("Segoe UI", ui::fonts::kNodeTitleSize, QFont::Bold);
    painter.setFont(name_font);
    QFontMetrics fm(name_font);
    QString elided_name = fm.elidedText(display_name_, Qt::ElideMiddle, width() - 60);
    painter.drawText(QRect(50, 8, width() - 60, 25), Qt::AlignLeft | Qt::AlignVCenter, elided_name);
    
    // Draw path hint (shortened)
    QFont path_font("Segoe UI", ui::fonts::kNodeSubtitleSize);
    painter.setFont(path_font);
    painter.setPen(QColor(255, 255, 255, 180));
    QFontMetrics fm2(path_font);
    QString short_path = folder_path_;
    if (short_path.length() > 30) {
        short_path = "..." + short_path.right(27);
    }
    QString elided_path = fm2.elidedText(short_path, Qt::ElideMiddle, width() - 60);
    painter.drawText(QRect(50, 32, width() - 60, 18), Qt::AlignLeft | Qt::AlignVCenter, elided_path);
    
    // Draw file count badge if any
    if (file_count_ > 0) {
        QRect badge_rect(width() - 35, 5, 30, 20);
        painter.setBrush(QColor(255, 255, 255, 230));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(badge_rect, 10, 10);
        
        painter.setPen(bg_color.darker(120));
        QFont badge_font("Segoe UI", 10, QFont::Bold);
        painter.setFont(badge_font);
        painter.drawText(badge_rect, Qt::AlignCenter, QString::number(file_count_));
    }
    
    // Draw connection indicator
    if (is_connected_) {
        painter.setBrush(QColor(255, 255, 255));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPoint(width() - 12, height() - 12), 6, 6);
    }
}

void FolderNodeWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(folder_path_);
    } else if (event->button() == Qt::RightButton) {
        emit right_clicked(folder_path_, event->globalPosition().toPoint());
    }
    QWidget::mousePressEvent(event);
}

void FolderNodeWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit double_clicked(folder_path_);
    }
    QWidget::mouseDoubleClickEvent(event);
}

void FolderNodeWidget::enterEvent(QEnterEvent*) {
    is_hovered_ = true;
    hover_animation_->stop();
    hover_animation_->setStartValue(scale_);
    hover_animation_->setEndValue(1.05);
    hover_animation_->start();
}

void FolderNodeWidget::leaveEvent(QEvent*) {
    is_hovered_ = false;
    hover_animation_->stop();
    hover_animation_->setStartValue(scale_);
    hover_animation_->setEndValue(1.0);
    hover_animation_->start();
}

void FolderNodeWidget::update_appearance() {
    update();
}
