#ifndef DIAGNOSTIC_TOOL_HPP
#define DIAGNOSTIC_TOOL_HPP

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QString>
#include <QStringList>
#include <functional>

class DatabaseManager;

struct DiagnosticTestResult {
    QString test_name;
    bool passed;
    QString details;
    int duration_ms;
};

class DiagnosticTool : public QDialog {
    Q_OBJECT

public:
    explicit DiagnosticTool(DatabaseManager& db, QWidget* parent = nullptr);
    ~DiagnosticTool() override;
    
public slots:
    void run_all_tests();
    void run_selected_test(int index);
    void export_report();
    void show_log_viewer();
    
signals:
    void test_completed(const DiagnosticTestResult& result);
    
private:
    void setup_ui();
    void append_output(const QString& text, const QString& color = "white");
    void report_result(const DiagnosticTestResult& result);
    
    // Individual tests
    DiagnosticTestResult test_database_connection();
    DiagnosticTestResult test_database_operations();
    DiagnosticTestResult test_file_operations();
    DiagnosticTestResult test_folder_creation();
    DiagnosticTestResult test_ui_components();
    DiagnosticTestResult test_mime_detection();
    DiagnosticTestResult test_memory_usage();
    
    DatabaseManager& db_;
    QTextEdit* output_display_;
    QProgressBar* progress_bar_;
    QPushButton* run_all_btn_;
    QPushButton* export_btn_;
    QPushButton* log_btn_;
    QStringList test_names_;
    std::vector<DiagnosticTestResult> results_;
};

#endif // DIAGNOSTIC_TOOL_HPP
