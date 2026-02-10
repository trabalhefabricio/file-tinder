#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QStyleFactory>
#include <QDir>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

#include "DatabaseManager.hpp"
#include "StandaloneFileTinderDialog.hpp"
#include "AdvancedFileTinderDialog.hpp"
#include "AppLogger.hpp"
#include "DiagnosticTool.hpp"

class FileTinderLauncher : public QDialog {
    Q_OBJECT
    
public:
    FileTinderLauncher(QWidget* parent_widget = nullptr) 
        : QDialog(parent_widget)
        , db_manager_()
        , chosen_path_() {
        
        setWindowTitle("File Tinder Launcher");
        setMinimumSize(550, 450);
        
        LOG_INFO("Launcher", "Application starting");
        
        if (!db_manager_.initialize()) {
            LOG_ERROR("Launcher", "Database initialization failed");
            QMessageBox::critical(this, "Database Error", "Could not initialize the database.");
        }
        
        build_interface();
    }
    
private:
    DatabaseManager db_manager_;
    QString chosen_path_;
    QLabel* path_indicator_;
    
    void build_interface() {
        auto* root_layout = new QVBoxLayout(this);
        root_layout->setContentsMargins(25, 25, 25, 25);
        root_layout->setSpacing(18);
        
        // App header
        auto* app_title = new QLabel("FILE TINDER");
        app_title->setStyleSheet("font-size: 28px; font-weight: bold; color: #0078d4;");
        app_title->setAlignment(Qt::AlignCenter);
        root_layout->addWidget(app_title);
        
        auto* app_desc = new QLabel("Organize files with swipe-style sorting");
        app_desc->setStyleSheet("font-size: 13px; color: #888888;");
        app_desc->setAlignment(Qt::AlignCenter);
        root_layout->addWidget(app_desc);
        
        root_layout->addSpacing(15);
        
        // Folder picker section
        auto* picker_label = new QLabel("Choose folder to organize:");
        picker_label->setStyleSheet("font-weight: bold; font-size: 12px;");
        root_layout->addWidget(picker_label);
        
        auto* picker_row = new QHBoxLayout();
        
        path_indicator_ = new QLabel("(none selected)");
        path_indicator_->setStyleSheet(
            "padding: 8px 12px; background-color: #2d2d2d; border: 1px solid #404040; color: #aaaaaa;"
        );
        path_indicator_->setWordWrap(true);
        picker_row->addWidget(path_indicator_, 1);
        
        auto* pick_btn = new QPushButton("Select...");
        pick_btn->setStyleSheet(
            "QPushButton { padding: 8px 16px; background-color: #0078d4; color: white; border: none; }"
            "QPushButton:hover { background-color: #106ebe; }"
        );
        connect(pick_btn, &QPushButton::clicked, this, &FileTinderLauncher::pick_folder);
        picker_row->addWidget(pick_btn);
        
        root_layout->addLayout(picker_row);
        
        root_layout->addStretch();
        
        // Mode buttons
        auto* modes_label = new QLabel("Choose mode:");
        modes_label->setStyleSheet("font-weight: bold; font-size: 12px;");
        root_layout->addWidget(modes_label);
        
        auto* modes_row = new QHBoxLayout();
        modes_row->setSpacing(12);
        
        auto* basic_mode_btn = new QPushButton("Basic Mode\n(Simple sorting)");
        basic_mode_btn->setMinimumSize(180, 70);
        basic_mode_btn->setStyleSheet(
            "QPushButton { padding: 12px; background-color: #107c10; color: white; border: none; font-size: 13px; }"
            "QPushButton:hover { background-color: #0e6b0e; }"
        );
        connect(basic_mode_btn, &QPushButton::clicked, this, &FileTinderLauncher::launch_basic);
        modes_row->addWidget(basic_mode_btn);
        
        auto* adv_mode_btn = new QPushButton("Advanced Mode\n(Folder tree view)");
        adv_mode_btn->setMinimumSize(180, 70);
        adv_mode_btn->setStyleSheet(
            "QPushButton { padding: 12px; background-color: #5c2d91; color: white; border: none; font-size: 13px; }"
            "QPushButton:hover { background-color: #4a2473; }"
        );
        connect(adv_mode_btn, &QPushButton::clicked, this, &FileTinderLauncher::launch_advanced);
        modes_row->addWidget(adv_mode_btn);
        
        root_layout->addLayout(modes_row);
        
        // Diagnostics button
        auto* tools_row = new QHBoxLayout();
        tools_row->addStretch();
        
        auto* diag_btn = new QPushButton("Diagnostics");
        diag_btn->setStyleSheet(
            "QPushButton { padding: 6px 12px; background-color: #4a4a4a; color: #cccccc; border: 1px solid #555555; }"
            "QPushButton:hover { background-color: #555555; }"
        );
        connect(diag_btn, &QPushButton::clicked, this, &FileTinderLauncher::open_diagnostics);
        tools_row->addWidget(diag_btn);
        
        root_layout->addLayout(tools_row);
        
        // Hotkey hint
        auto* hint_text = new QLabel("Keys: Right=Keep | Left=Delete | Down=Skip | Up=Back | Z=Undo");
        hint_text->setStyleSheet("color: #666666; font-size: 10px; padding-top: 8px;");
        hint_text->setAlignment(Qt::AlignCenter);
        root_layout->addWidget(hint_text);
    }
    
    void pick_folder() {
        QString path = QFileDialog::getExistingDirectory(this, "Pick Folder", QDir::homePath());
        if (!path.isEmpty()) {
            chosen_path_ = path;
            path_indicator_->setText(path);
            path_indicator_->setStyleSheet(
                "padding: 8px 12px; background-color: #1a3a1a; border: 1px solid #2a5a2a; color: #88cc88;"
            );
            LOG_INFO("Launcher", QString("Folder selected: %1").arg(path));
        }
    }
    
    bool validate_folder() {
        if (chosen_path_.isEmpty()) {
            QMessageBox::warning(this, "No Folder", "Please select a folder first.");
            return false;
        }
        
        QDir folder(chosen_path_);
        if (folder.entryList(QDir::Files).isEmpty()) {
            QMessageBox::information(this, "Empty Folder", "This folder has no files to sort.");
            return false;
        }
        
        return true;
    }
    
    void launch_basic() {
        if (!validate_folder()) return;
        
        LOG_INFO("Launcher", "Starting basic mode");
        
        auto* dlg = new StandaloneFileTinderDialog(chosen_path_, db_manager_, this);
        dlg->initialize();
        
        connect(dlg, &StandaloneFileTinderDialog::switch_to_advanced_mode, this, [this, dlg]() {
            dlg->close();
            launch_advanced();
        });
        
        dlg->exec();
        dlg->deleteLater();
    }
    
    void launch_advanced() {
        if (!validate_folder()) return;
        
        LOG_INFO("Launcher", "Starting advanced mode");
        
        auto* dlg = new AdvancedFileTinderDialog(chosen_path_, db_manager_, this);
        dlg->initialize();
        
        connect(dlg, &AdvancedFileTinderDialog::switch_to_basic_mode, this, [this, dlg]() {
            dlg->close();
            launch_basic();
        });
        
        dlg->exec();
        dlg->deleteLater();
    }
    
    void open_diagnostics() {
        LOG_INFO("Launcher", "Opening diagnostic tool");
        DiagnosticTool diag(db_manager_, this);
        diag.exec();
    }
};

int main(int argc, char* argv[]) {
    QApplication qt_app(argc, argv);
    
    qt_app.setApplicationName("File Tinder");
    qt_app.setApplicationVersion("1.0.0");
    qt_app.setOrganizationName("FileTinderApp");
    
    // Initialize logging
    AppLogger::instance().set_minimum_severity(LogSeverity::Debug);
    LOG_INFO("Main", "File Tinder application started");
    
    // Use Fusion style for consistent look
    qt_app.setStyle(QStyleFactory::create("Fusion"));
    
    // Custom color scheme
    QPalette app_colors;
    app_colors.setColor(QPalette::Window, QColor(45, 45, 45));
    app_colors.setColor(QPalette::WindowText, QColor(230, 230, 230));
    app_colors.setColor(QPalette::Base, QColor(35, 35, 35));
    app_colors.setColor(QPalette::AlternateBase, QColor(55, 55, 55));
    app_colors.setColor(QPalette::Text, QColor(230, 230, 230));
    app_colors.setColor(QPalette::Button, QColor(50, 50, 50));
    app_colors.setColor(QPalette::ButtonText, QColor(230, 230, 230));
    app_colors.setColor(QPalette::Highlight, QColor(0, 120, 212));
    app_colors.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    qt_app.setPalette(app_colors);
    
    FileTinderLauncher launcher_window;
    launcher_window.show();
    
    int exit_code = qt_app.exec();
    
    LOG_INFO("Main", "Application exiting");
    return exit_code;
}

#include "main.moc"
