#ifndef APP_LOGGER_HPP
#define APP_LOGGER_HPP

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDir>
#include <QStandardPaths>

enum class LogSeverity {
    Trace,
    Debug, 
    Info,
    Warning,
    Error,
    Critical
};

class AppLogger {
public:
    static AppLogger& instance();
    
    void set_log_file(const QString& path);
    void set_minimum_severity(LogSeverity sev);
    void set_console_output(bool enabled);
    
    void log(LogSeverity sev, const QString& component, const QString& msg);
    
    void trace(const QString& component, const QString& msg);
    void debug(const QString& component, const QString& msg);
    void info(const QString& component, const QString& msg);
    void warning(const QString& component, const QString& msg);
    void error(const QString& component, const QString& msg);
    void critical(const QString& component, const QString& msg);
    
    QString log_file_path() const;
    QStringList recent_entries(int count) const;
    
private:
    AppLogger();
    ~AppLogger();
    AppLogger(const AppLogger&) = delete;
    AppLogger& operator=(const AppLogger&) = delete;
    
    QString severity_label(LogSeverity sev) const;
    void write_entry(const QString& entry);
    void ensure_log_dir();
    
    QFile log_file_;
    QTextStream log_stream_;
    LogSeverity min_severity_;
    bool console_enabled_;
    mutable QMutex mutex_;
    QStringList recent_buffer_;
    static constexpr int kRecentBufferMax = 500;
};

// Convenience macros
#define LOG_TRACE(comp, msg) AppLogger::instance().trace(comp, msg)
#define LOG_DEBUG(comp, msg) AppLogger::instance().debug(comp, msg)
#define LOG_INFO(comp, msg) AppLogger::instance().info(comp, msg)
#define LOG_WARN(comp, msg) AppLogger::instance().warning(comp, msg)
#define LOG_ERROR(comp, msg) AppLogger::instance().error(comp, msg)
#define LOG_CRITICAL(comp, msg) AppLogger::instance().critical(comp, msg)

#endif // APP_LOGGER_HPP
