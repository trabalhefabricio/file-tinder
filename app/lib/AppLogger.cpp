#include "AppLogger.hpp"
#include <QDebug>
#include <iostream>

AppLogger& AppLogger::instance() {
    static AppLogger logger_instance;
    return logger_instance;
}

AppLogger::AppLogger()
    : min_severity_(LogSeverity::Info)
    , console_enabled_(true) {
    ensure_log_dir();
}

AppLogger::~AppLogger() {
    if (log_file_.isOpen()) {
        log_stream_.flush();
        log_file_.close();
    }
}

void AppLogger::ensure_log_dir() {
    QString app_data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(app_data);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString log_path = dir.filePath("file_tinder.log");
    set_log_file(log_path);
}

void AppLogger::set_log_file(const QString& path) {
    QMutexLocker locker(&mutex_);
    
    if (log_file_.isOpen()) {
        log_stream_.flush();
        log_file_.close();
    }
    
    log_file_.setFileName(path);
    if (log_file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        log_stream_.setDevice(&log_file_);
        write_entry(QString("=== Log session started at %1 ===")
                   .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    }
}

void AppLogger::set_minimum_severity(LogSeverity sev) {
    min_severity_ = sev;
}

void AppLogger::set_console_output(bool enabled) {
    console_enabled_ = enabled;
}

QString AppLogger::severity_label(LogSeverity sev) const {
    switch (sev) {
        case LogSeverity::Trace:    return "TRACE";
        case LogSeverity::Debug:    return "DEBUG";
        case LogSeverity::Info:     return "INFO";
        case LogSeverity::Warning:  return "WARN";
        case LogSeverity::Error:    return "ERROR";
        case LogSeverity::Critical: return "CRIT";
        default:                    return "???";
    }
}

void AppLogger::write_entry(const QString& entry) {
    if (log_stream_.device()) {
        log_stream_ << entry << "\n";
        log_stream_.flush();
    }
    
    // Keep in memory buffer
    recent_buffer_.append(entry);
    while (recent_buffer_.size() > kRecentBufferMax) {
        recent_buffer_.removeFirst();
    }
}

void AppLogger::log(LogSeverity sev, const QString& component, const QString& msg) {
    if (sev < min_severity_) return;
    
    QMutexLocker locker(&mutex_);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString sev_str = severity_label(sev);
    QString entry = QString("[%1] [%2] [%3] %4")
                   .arg(timestamp, sev_str, component, msg);
    
    write_entry(entry);
    
    if (console_enabled_) {
        std::cerr << entry.toStdString() << std::endl;
    }
}

void AppLogger::trace(const QString& component, const QString& msg) {
    log(LogSeverity::Trace, component, msg);
}

void AppLogger::debug(const QString& component, const QString& msg) {
    log(LogSeverity::Debug, component, msg);
}

void AppLogger::info(const QString& component, const QString& msg) {
    log(LogSeverity::Info, component, msg);
}

void AppLogger::warning(const QString& component, const QString& msg) {
    log(LogSeverity::Warning, component, msg);
}

void AppLogger::error(const QString& component, const QString& msg) {
    log(LogSeverity::Error, component, msg);
}

void AppLogger::critical(const QString& component, const QString& msg) {
    log(LogSeverity::Critical, component, msg);
}

QString AppLogger::log_file_path() const {
    return log_file_.fileName();
}

QStringList AppLogger::recent_entries(int count) const {
    QMutexLocker locker(&mutex_);
    
    if (count >= recent_buffer_.size()) {
        return recent_buffer_;
    }
    
    return recent_buffer_.mid(recent_buffer_.size() - count);
}
