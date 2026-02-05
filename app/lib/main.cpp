#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QStyleFactory>
#include <QDir>

#include "DatabaseManager.hpp"
#include "StandaloneFileTinderDialog.hpp"
#include "AdvancedFileTinderDialog.hpp"

class MainWindow : public QDialog {
    Q_OBJECT
    
public:
    MainWindow(QWidget* parent = nullptr) : QDialog(parent), db_() {
        setWindowTitle("File Tinder");
        setMinimumSize(500, 400);
        
        if (!db_.initialize()) {
            QMessageBox::critical(this, "Error", "Failed to initialize database.");
        }
        
        setup_ui();
    }
    
private:
    DatabaseManager db_;
    QString selected_folder_;
    
    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(30, 30, 30, 30);
        layout->setSpacing(20);
        
        // Title
        auto* title = new QLabel("📁 FILE TINDER");
        title->setStyleSheet(
            "font-size: 32px; font-weight: bold; color: #3498db;"
        );
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);
        
        // Subtitle
        auto* subtitle = new QLabel("Swipe-style file organization tool");
        subtitle->setStyleSheet("font-size: 14px; color: #7f8c8d;");
        subtitle->setAlignment(Qt::AlignCenter);
        layout->addWidget(subtitle);
        
        layout->addSpacing(20);
        
        // Folder selection
        auto* folder_group = new QWidget();
        auto* folder_layout = new QVBoxLayout(folder_group);
        folder_layout->setContentsMargins(0, 0, 0, 0);
        
        auto* folder_label = new QLabel("Select a folder to organize:");
        folder_label->setStyleSheet("font-weight: bold;");
        folder_layout->addWidget(folder_label);
        
        auto* folder_btn_layout = new QHBoxLayout();
        
        auto* folder_display = new QLabel("No folder selected");
        folder_display->setStyleSheet(
            "padding: 10px; background-color: #34495e; border-radius: 5px; color: #bdc3c7;"
        );
        folder_display->setWordWrap(true);
        folder_btn_layout->addWidget(folder_display, 1);
        
        auto* browse_btn = new QPushButton("Browse...");
        browse_btn->setStyleSheet(
            "QPushButton { padding: 10px 20px; background-color: #3498db; "
            "border-radius: 5px; color: white; }"
            "QPushButton:hover { background-color: #2980b9; }"
        );
        folder_btn_layout->addWidget(browse_btn);
        
        folder_layout->addLayout(folder_btn_layout);
        layout->addWidget(folder_group);
        
        connect(browse_btn, &QPushButton::clicked, this, [this, folder_display]() {
            QString folder = QFileDialog::getExistingDirectory(
                this, "Select Folder to Organize", QDir::homePath()
            );
            if (!folder.isEmpty()) {
                selected_folder_ = folder;
                folder_display->setText(folder);
                folder_display->setStyleSheet(
                    "padding: 10px; background-color: #27ae60; border-radius: 5px; color: white;"
                );
            }
        });
        
        layout->addStretch();
        
        // Mode selection buttons
        auto* mode_label = new QLabel("Select Mode:");
        mode_label->setStyleSheet("font-weight: bold;");
        layout->addWidget(mode_label);
        
        auto* mode_layout = new QHBoxLayout();
        
        auto* basic_btn = new QPushButton("🎯 Basic Mode\nSimple swipe sorting");
        basic_btn->setMinimumSize(200, 80);
        basic_btn->setStyleSheet(
            "QPushButton { padding: 15px; background-color: #3498db; "
            "border-radius: 10px; color: white; font-size: 14px; }"
            "QPushButton:hover { background-color: #2980b9; }"
        );
        mode_layout->addWidget(basic_btn);
        
        auto* advanced_btn = new QPushButton("🌳 Advanced Mode\nVisual folder tree");
        advanced_btn->setMinimumSize(200, 80);
        advanced_btn->setStyleSheet(
            "QPushButton { padding: 15px; background-color: #9b59b6; "
            "border-radius: 10px; color: white; font-size: 14px; }"
            "QPushButton:hover { background-color: #8e44ad; }"
        );
        mode_layout->addWidget(advanced_btn);
        
        layout->addLayout(mode_layout);
        
        connect(basic_btn, &QPushButton::clicked, this, &MainWindow::start_basic_mode);
        connect(advanced_btn, &QPushButton::clicked, this, &MainWindow::start_advanced_mode);
        
        // Instructions
        auto* instructions = new QLabel(
            "Keyboard Shortcuts:\n"
            "→ Keep  |  ← Delete  |  ↓ Skip  |  ↑ Back  |  M Move to folder"
        );
        instructions->setStyleSheet("color: #7f8c8d; font-size: 11px; padding: 10px;");
        instructions->setAlignment(Qt::AlignCenter);
        layout->addWidget(instructions);
    }
    
    void start_basic_mode() {
        if (selected_folder_.isEmpty()) {
            QMessageBox::warning(this, "No Folder Selected", 
                                "Please select a folder to organize first.");
            return;
        }
        
        QDir dir(selected_folder_);
        QStringList files = dir.entryList(QDir::Files);
        if (files.isEmpty()) {
            QMessageBox::information(this, "No Files", 
                                    "The selected folder contains no files to organize.");
            return;
        }
        
        auto* dialog = new StandaloneFileTinderDialog(selected_folder_, db_, this);
        
        connect(dialog, &StandaloneFileTinderDialog::switch_to_advanced_mode, this, [this, dialog]() {
            dialog->close();
            start_advanced_mode();
        });
        
        dialog->exec();
        delete dialog;
    }
    
    void start_advanced_mode() {
        if (selected_folder_.isEmpty()) {
            QMessageBox::warning(this, "No Folder Selected", 
                                "Please select a folder to organize first.");
            return;
        }
        
        QDir dir(selected_folder_);
        QStringList files = dir.entryList(QDir::Files);
        if (files.isEmpty()) {
            QMessageBox::information(this, "No Files", 
                                    "The selected folder contains no files to organize.");
            return;
        }
        
        auto* dialog = new AdvancedFileTinderDialog(selected_folder_, db_, this);
        
        connect(dialog, &AdvancedFileTinderDialog::switch_to_basic_mode, this, [this, dialog]() {
            dialog->close();
            start_basic_mode();
        });
        
        dialog->exec();
        delete dialog;
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Set application info
    app.setApplicationName("File Tinder");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("FileTinder");
    
    // Apply dark fusion style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette dark_palette;
    dark_palette.setColor(QPalette::Window, QColor(53, 53, 53));
    dark_palette.setColor(QPalette::WindowText, Qt::white);
    dark_palette.setColor(QPalette::Base, QColor(42, 42, 42));
    dark_palette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    dark_palette.setColor(QPalette::ToolTipBase, Qt::white);
    dark_palette.setColor(QPalette::ToolTipText, Qt::white);
    dark_palette.setColor(QPalette::Text, Qt::white);
    dark_palette.setColor(QPalette::Button, QColor(53, 53, 53));
    dark_palette.setColor(QPalette::ButtonText, Qt::white);
    dark_palette.setColor(QPalette::BrightText, Qt::red);
    dark_palette.setColor(QPalette::Link, QColor(42, 130, 218));
    dark_palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    dark_palette.setColor(QPalette::HighlightedText, Qt::black);
    
    app.setPalette(dark_palette);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

#include "main.moc"
