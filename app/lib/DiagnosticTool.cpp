#include "DiagnosticTool.hpp"
#include "DatabaseManager.hpp"
#include "AppLogger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QApplication>
#include <QTextStream>
#include <QDateTime>

DiagnosticTool::DiagnosticTool(DatabaseManager& db, QWidget* parent)
    : QDialog(parent)
    , db_(db) {
    setWindowTitle("File Tinder - Diagnostic Tool");
    setMinimumSize(700, 500);
    setup_ui();
    
    LOG_INFO("Diagnostics", "Diagnostic tool opened");
}

DiagnosticTool::~DiagnosticTool() = default;

void DiagnosticTool::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(15, 15, 15, 15);
    main_layout->setSpacing(12);
    
    // Header
    auto* header = new QLabel("System Diagnostics");
    header->setStyleSheet("font-size: 18px; font-weight: bold; color: #3498db;");
    main_layout->addWidget(header);
    
    // Output area
    auto* output_group = new QGroupBox("Test Results");
    auto* output_layout = new QVBoxLayout(output_group);
    
    output_display_ = new QTextEdit();
    output_display_->setReadOnly(true);
    output_display_->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; color: #d4d4d4; "
        "font-family: 'Consolas', 'Courier New', monospace; font-size: 11px; "
        "border: 1px solid #3c3c3c; padding: 8px; }");
    output_layout->addWidget(output_display_);
    
    main_layout->addWidget(output_group, 1);
    
    // Progress bar
    progress_bar_ = new QProgressBar();
    progress_bar_->setTextVisible(true);
    progress_bar_->setStyleSheet(
        "QProgressBar { border: 1px solid #3c3c3c; border-radius: 3px; "
        "text-align: center; background-color: #2d2d2d; }"
        "QProgressBar::chunk { background-color: #27ae60; }");
    main_layout->addWidget(progress_bar_);
    
    // Buttons
    auto* btn_layout = new QHBoxLayout();
    
    run_all_btn_ = new QPushButton("Run All Tests");
    run_all_btn_->setStyleSheet(
        "QPushButton { padding: 10px 20px; background-color: #27ae60; "
        "color: white; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #2ecc71; }");
    connect(run_all_btn_, &QPushButton::clicked, this, &DiagnosticTool::run_all_tests);
    btn_layout->addWidget(run_all_btn_);
    
    log_btn_ = new QPushButton("View Logs");
    log_btn_->setStyleSheet(
        "QPushButton { padding: 10px 20px; background-color: #3498db; "
        "color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: #2980b9; }");
    connect(log_btn_, &QPushButton::clicked, this, &DiagnosticTool::show_log_viewer);
    btn_layout->addWidget(log_btn_);
    
    export_btn_ = new QPushButton("Export Report");
    export_btn_->setStyleSheet(
        "QPushButton { padding: 10px 20px; background-color: #9b59b6; "
        "color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: #8e44ad; }");
    connect(export_btn_, &QPushButton::clicked, this, &DiagnosticTool::export_report);
    btn_layout->addWidget(export_btn_);
    
    btn_layout->addStretch();
    
    auto* close_btn = new QPushButton("Close");
    close_btn->setStyleSheet(
        "QPushButton { padding: 10px 20px; background-color: #7f8c8d; "
        "color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: #95a5a6; }");
    connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
    btn_layout->addWidget(close_btn);
    
    main_layout->addLayout(btn_layout);
    
    // Initialize test list
    test_names_ = {
        "Database Connection",
        "Database Operations",
        "File Operations",
        "Folder Creation",
        "UI Components",
        "MIME Detection",
        "Memory Usage"
    };
}

void DiagnosticTool::append_output(const QString& text, const QString& color) {
    output_display_->append(QString("<span style='color: %1;'>%2</span>").arg(color, text));
    QApplication::processEvents();
}

void DiagnosticTool::report_result(const DiagnosticTestResult& result) {
    QString status_icon = result.passed ? "PASS" : "FAIL";
    QString status_color = result.passed ? "#27ae60" : "#e74c3c";
    
    append_output(QString("[%1] %2 (%3ms)")
                 .arg(status_icon, result.test_name, QString::number(result.duration_ms)),
                 status_color);
    
    if (!result.details.isEmpty()) {
        append_output(QString("    Details: %1").arg(result.details), "#7f8c8d");
    }
    
    results_.push_back(result);
}

void DiagnosticTool::run_all_tests() {
    results_.clear();
    output_display_->clear();
    
    append_output("=== File Tinder Diagnostic Report ===", "#3498db");
    append_output(QString("Started: %1").arg(QDateTime::currentDateTime().toString()), "#7f8c8d");
    append_output("", "white");
    
    progress_bar_->setMaximum(static_cast<int>(test_names_.size()));
    progress_bar_->setValue(0);
    
    LOG_INFO("Diagnostics", "Running all diagnostic tests");
    
    // Run each test
    report_result(test_database_connection());
    progress_bar_->setValue(1);
    
    report_result(test_database_operations());
    progress_bar_->setValue(2);
    
    report_result(test_file_operations());
    progress_bar_->setValue(3);
    
    report_result(test_folder_creation());
    progress_bar_->setValue(4);
    
    report_result(test_ui_components());
    progress_bar_->setValue(5);
    
    report_result(test_mime_detection());
    progress_bar_->setValue(6);
    
    report_result(test_memory_usage());
    progress_bar_->setValue(7);
    
    // Summary
    int passed = 0, failed = 0;
    for (const auto& r : results_) {
        if (r.passed) passed++; else failed++;
    }
    
    append_output("", "white");
    append_output("=== Summary ===", "#3498db");
    append_output(QString("Passed: %1 | Failed: %2 | Total: %3")
                 .arg(passed).arg(failed).arg(passed + failed),
                 failed > 0 ? "#e74c3c" : "#27ae60");
    
    LOG_INFO("Diagnostics", QString("Tests complete: %1 passed, %2 failed").arg(passed).arg(failed));
}

void DiagnosticTool::run_selected_test(int index) {
    if (index < 0 || index >= test_names_.size()) return;
    
    output_display_->clear();
    append_output(QString("Running: %1").arg(test_names_[index]), "#3498db");
    
    DiagnosticTestResult result;
    switch (index) {
        case 0: result = test_database_connection(); break;
        case 1: result = test_database_operations(); break;
        case 2: result = test_file_operations(); break;
        case 3: result = test_folder_creation(); break;
        case 4: result = test_ui_components(); break;
        case 5: result = test_mime_detection(); break;
        case 6: result = test_memory_usage(); break;
        default: return;
    }
    
    report_result(result);
}

DiagnosticTestResult DiagnosticTool::test_database_connection() {
    DiagnosticTestResult result;
    result.test_name = "Database Connection";
    
    QElapsedTimer timer;
    timer.start();
    
    bool connected = db_.is_connected();
    
    result.duration_ms = static_cast<int>(timer.elapsed());
    result.passed = connected;
    result.details = connected ? "SQLite database accessible" : "Cannot connect to database";
    
    return result;
}

DiagnosticTestResult DiagnosticTool::test_database_operations() {
    DiagnosticTestResult result;
    result.test_name = "Database Operations";
    
    QElapsedTimer timer;
    timer.start();
    
    // Try a simple operation
    bool success = true;
    QString test_folder = "/tmp/diagnostic_test_folder";
    QString test_file = "/tmp/diagnostic_test.txt";
    
    db_.save_file_decision(test_folder, test_file, "keep", "");
    auto decisions = db_.get_session_decisions(test_folder);
    
    success = !decisions.empty();
    
    result.duration_ms = static_cast<int>(timer.elapsed());
    result.passed = success;
    result.details = success ? "Read/write operations functional" : "Database operations failed";
    
    return result;
}

DiagnosticTestResult DiagnosticTool::test_file_operations() {
    DiagnosticTestResult result;
    result.test_name = "File Operations";
    
    QElapsedTimer timer;
    timer.start();
    
    QString test_path = QDir::temp().filePath("filetinder_diag_test.txt");
    bool success = true;
    
    // Write test
    QFile test_file(test_path);
    if (test_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&test_file);
        stream << "Diagnostic test content";
        test_file.close();
    } else {
        success = false;
    }
    
    // Read test
    if (success && test_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&test_file);
        QString content = stream.readAll();
        test_file.close();
        success = content.contains("Diagnostic");
    } else {
        success = false;
    }
    
    // Cleanup
    QFile::remove(test_path);
    
    result.duration_ms = static_cast<int>(timer.elapsed());
    result.passed = success;
    result.details = success ? "File read/write working" : "File operations failed";
    
    return result;
}

DiagnosticTestResult DiagnosticTool::test_folder_creation() {
    DiagnosticTestResult result;
    result.test_name = "Folder Creation";
    
    QElapsedTimer timer;
    timer.start();
    
    QString test_dir = QDir::temp().filePath("filetinder_diag_folder");
    QDir dir;
    bool created = dir.mkpath(test_dir);
    bool exists = QDir(test_dir).exists();
    bool removed = dir.rmdir(test_dir);
    
    result.duration_ms = static_cast<int>(timer.elapsed());
    result.passed = created && exists && removed;
    result.details = result.passed ? "Folder create/remove working" : "Folder operations failed";
    
    return result;
}

DiagnosticTestResult DiagnosticTool::test_ui_components() {
    DiagnosticTestResult result;
    result.test_name = "UI Components";
    
    QElapsedTimer timer;
    timer.start();
    
    // Test that we can create UI elements without crashing
    bool success = true;
    
    auto* test_btn = new QPushButton("Test");
    auto* test_label = new QLabel("Test");
    auto* test_progress = new QProgressBar();
    
    success = (test_btn != nullptr && test_label != nullptr && test_progress != nullptr);
    
    delete test_btn;
    delete test_label;
    delete test_progress;
    
    result.duration_ms = static_cast<int>(timer.elapsed());
    result.passed = success;
    result.details = success ? "UI widgets instantiate correctly" : "UI component creation failed";
    
    return result;
}

DiagnosticTestResult DiagnosticTool::test_mime_detection() {
    DiagnosticTestResult result;
    result.test_name = "MIME Detection";
    
    QElapsedTimer timer;
    timer.start();
    
    QMimeDatabase mime_db;
    
    // Test common file types
    QStringList test_files = {"test.jpg", "test.pdf", "test.txt", "test.mp4"};
    QStringList expected_prefixes = {"image/", "application/pdf", "text/", "video/"};
    
    bool all_ok = true;
    for (int i = 0; i < test_files.size(); ++i) {
        QMimeType mime = mime_db.mimeTypeForFile(test_files[i], QMimeDatabase::MatchExtension);
        if (!mime.name().startsWith(expected_prefixes[i].split("/")[0])) {
            all_ok = false;
            break;
        }
    }
    
    result.duration_ms = static_cast<int>(timer.elapsed());
    result.passed = all_ok;
    result.details = all_ok ? "MIME type detection working" : "MIME detection issues found";
    
    return result;
}

DiagnosticTestResult DiagnosticTool::test_memory_usage() {
    DiagnosticTestResult result;
    result.test_name = "Memory Usage";
    
    QElapsedTimer timer;
    timer.start();
    
    // Simple memory allocation test
    bool success = true;
    
    std::vector<char> test_buffer;
    test_buffer.resize(1024 * 1024); // 1MB allocation
    test_buffer[0] = 'x';
    test_buffer[test_buffer.size() - 1] = 'y';
    
    success = (test_buffer[0] == 'x' && test_buffer[test_buffer.size() - 1] == 'y');
    
    result.duration_ms = static_cast<int>(timer.elapsed());
    result.passed = success;
    result.details = success ? "Memory allocation working" : "Memory allocation issues";
    
    return result;
}

void DiagnosticTool::export_report() {
    QString filename = QFileDialog::getSaveFileName(
        this, "Export Diagnostic Report", 
        QDir::homePath() + "/filetinder_diagnostic_report.txt",
        "Text Files (*.txt)");
    
    if (filename.isEmpty()) return;
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << "File Tinder Diagnostic Report\n";
        stream << "Generated: " << QDateTime::currentDateTime().toString() << "\n";
        stream << "=================================\n\n";
        
        for (const auto& r : results_) {
            stream << (r.passed ? "[PASS]" : "[FAIL]") << " " << r.test_name;
            stream << " (" << r.duration_ms << "ms)\n";
            if (!r.details.isEmpty()) {
                stream << "    " << r.details << "\n";
            }
        }
        
        file.close();
        
        QMessageBox::information(this, "Export Complete", 
                                QString("Report saved to:\n%1").arg(filename));
        
        LOG_INFO("Diagnostics", QString("Report exported to %1").arg(filename));
    }
}

void DiagnosticTool::show_log_viewer() {
    QStringList recent = AppLogger::instance().recent_entries(100);
    
    QDialog* log_dialog = new QDialog(this);
    log_dialog->setWindowTitle("Application Logs");
    log_dialog->setMinimumSize(800, 500);
    
    auto* layout = new QVBoxLayout(log_dialog);
    
    auto* log_text = new QTextEdit();
    log_text->setReadOnly(true);
    log_text->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; color: #d4d4d4; "
        "font-family: 'Consolas', 'Courier New', monospace; font-size: 10px; }");
    log_text->setText(recent.join("\n"));
    layout->addWidget(log_text);
    
    auto* info_label = new QLabel(QString("Log file: %1").arg(AppLogger::instance().log_file_path()));
    info_label->setStyleSheet("color: #7f8c8d; font-size: 10px;");
    layout->addWidget(info_label);
    
    auto* close_btn = new QPushButton("Close");
    connect(close_btn, &QPushButton::clicked, log_dialog, &QDialog::accept);
    layout->addWidget(close_btn);
    
    log_dialog->exec();
    delete log_dialog;
}
