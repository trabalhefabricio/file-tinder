#include "FileTinderExecutor.hpp"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#elif defined(Q_OS_MACOS)
#include <QProcess>
#elif defined(Q_OS_LINUX)
#include <QProcess>
#include <QStandardPaths>
#endif

FileTinderExecutor::FileTinderExecutor()
    : use_trash_(true)
    , overwrite_existing_(false) {
}

FileTinderExecutor::~FileTinderExecutor() = default;

ExecutionResult FileTinderExecutor::execute(const ExecutionPlan& plan, ProgressCallback progress_callback) {
    ExecutionResult result;
    
    int total_operations = static_cast<int>(
        plan.folders_to_create.size() + 
        plan.files_to_move.size() + 
        plan.files_to_delete.size()
    );
    
    if (total_operations == 0) {
        result.success = true;
        return result;
    }
    
    int current_progress = 0;
    
    // Step 1: Create folders
    if (!create_folders(plan.folders_to_create, result, progress_callback, 
                        current_progress, total_operations)) {
        result.success = false;
    }
    
    // Step 2: Move files
    if (!move_files(plan.files_to_move, result, progress_callback,
                   current_progress, total_operations)) {
        result.success = false;
    }
    
    // Step 3: Delete files
    if (!delete_files(plan.files_to_delete, result, progress_callback,
                     current_progress, total_operations)) {
        result.success = false;
    }
    
    return result;
}

bool FileTinderExecutor::create_folders(const std::vector<QString>& folders,
                                        ExecutionResult& result,
                                        ProgressCallback callback,
                                        int& progress, int total) {
    bool all_success = true;
    
    for (const QString& folder_path : folders) {
        if (callback) {
            callback(progress, total, QString("Creating folder: %1").arg(folder_path));
        }
        
        QDir dir;
        if (dir.mkpath(folder_path)) {
            result.folders_created++;
            result.log.push_back({"folder_create", folder_path, folder_path, true});
        } else {
            result.errors++;
            result.error_messages.append(QString("Failed to create folder: %1").arg(folder_path));
            result.log.push_back({"folder_create", folder_path, "", false});
            all_success = false;
        }
        
        progress++;
    }
    
    return all_success;
}

bool FileTinderExecutor::move_files(const std::vector<std::pair<QString, QString>>& moves,
                                    ExecutionResult& result,
                                    ProgressCallback callback,
                                    int& progress, int total) {
    bool all_success = true;
    
    for (const auto& [source, dest] : moves) {
        if (callback) {
            callback(progress, total, QString("Moving: %1").arg(QFileInfo(source).fileName()));
        }
        
        // Verify source file still exists before attempting move
        if (!QFile::exists(source)) {
            result.errors++;
            result.error_messages.append(QString("Source file no longer exists: %1").arg(source));
            result.log.push_back({"move", source, "", false});
            all_success = false;
            progress++;
            continue;
        }
        
        QString dest_path = dest;
        
        // If dest is an existing directory or ends with separator, treat as directory
        QFileInfo dest_info(dest);
        if (dest_info.isDir() || dest.endsWith('/') || dest.endsWith('\\')) {
            QDir dest_dir(dest);
            if (!dest_dir.exists()) {
                dest_dir.mkpath(".");
            }
            dest_path = dest_dir.absoluteFilePath(QFileInfo(source).fileName());
        }
        
        // Handle existing file
        if (QFile::exists(dest_path)) {
            if (overwrite_existing_) {
                QFile::remove(dest_path);
            } else {
                // Generate unique name with max attempts to prevent infinite loop
                QString base = QFileInfo(dest_path).completeBaseName();
                QString ext = QFileInfo(dest_path).suffix();
                QString dir_path = QFileInfo(dest_path).absolutePath();
                int counter = 1;
                const int max_attempts = 10000;
                while (QFile::exists(dest_path) && counter <= max_attempts) {
                    if (ext.isEmpty()) {
                        dest_path = QString("%1/%2_%3")
                            .arg(dir_path, base, QString::number(counter));
                    } else {
                        dest_path = QString("%1/%2_%3.%4")
                            .arg(dir_path, base, QString::number(counter), ext);
                    }
                    counter++;
                }
                if (counter > max_attempts) {
                    result.errors++;
                    result.error_messages.append(QString("Failed to generate unique name for: %1").arg(source));
                    result.log.push_back({"move", source, "", false});
                    all_success = false;
                    progress++;
                    continue;
                }
            }
        }
        
        if (QFile::rename(source, dest_path)) {
            result.files_moved++;
            result.log.push_back({"move", source, dest_path, true});
        } else {
            // Try copy and delete as fallback
            if (QFile::copy(source, dest_path)) {
                if (QFile::remove(source)) {
                    result.files_moved++;
                    result.log.push_back({"move", source, dest_path, true});
                } else {
                    result.error_messages.append(QString("Moved but failed to remove source: %1").arg(source));
                    result.files_moved++;
                    result.log.push_back({"move", source, dest_path, true});
                }
            } else {
                result.errors++;
                result.error_messages.append(QString("Failed to move: %1 to %2").arg(source, dest_path));
                result.log.push_back({"move", source, dest_path, false});
                all_success = false;
            }
        }
        
        progress++;
    }
    
    return all_success;
}

bool FileTinderExecutor::delete_files(const std::vector<QString>& files,
                                      ExecutionResult& result,
                                      ProgressCallback callback,
                                      int& progress, int total) {
    bool all_success = true;
    
    for (const QString& file_path : files) {
        if (callback) {
            callback(progress, total, QString("Deleting: %1").arg(QFileInfo(file_path).fileName()));
        }
        
        // Verify file still exists before attempting delete
        if (!QFile::exists(file_path)) {
            result.errors++;
            result.error_messages.append(QString("File no longer exists (already deleted?): %1").arg(file_path));
            result.log.push_back({"delete", file_path, "", false});
            all_success = false;
            progress++;
            continue;
        }
        
        bool deleted = false;
        QString trash_path;
        
        if (use_trash_) {
            deleted = move_to_trash(file_path, trash_path);
        }
        
        if (!deleted) {
            // Fallback to permanent delete (no undo possible)
            if (QFile::remove(file_path)) {
                deleted = true;
                trash_path.clear();  // No trash path for permanent delete
            }
        }
        
        if (deleted) {
            result.files_deleted++;
            result.log.push_back({"delete", file_path, trash_path, true});
        } else {
            result.errors++;
            result.error_messages.append(QString("Failed to delete: %1").arg(file_path));
            result.log.push_back({"delete", file_path, "", false});
            all_success = false;
        }
        
        progress++;
    }
    
    return all_success;
}

bool FileTinderExecutor::move_to_trash(const QString& file_path, QString& trash_path) {
    trash_path.clear();
#ifdef Q_OS_WIN
    // Windows: Use SHFileOperation (trash_path not trackable via this API)
    QString native_path = QDir::toNativeSeparators(file_path);
    native_path.append(QChar(0)).append(QChar(0)); // Double null-terminated as required by API
    
    SHFILEOPSTRUCTW op;
    memset(&op, 0, sizeof(op));
    op.wFunc = FO_DELETE;
    op.pFrom = reinterpret_cast<LPCWSTR>(native_path.utf16());
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
    
    return SHFileOperationW(&op) == 0;
    
#elif defined(Q_OS_MACOS)
    // macOS: Use osascript (trash_path not trackable via this API)
    QProcess process;
    QString script = QString("tell application \"Finder\" to delete POSIX file \"%1\"")
                        .arg(file_path);
    process.start("osascript", QStringList() << "-e" << script);
    process.waitForFinished(5000);
    return process.exitCode() == 0;
    
#elif defined(Q_OS_LINUX)
    // Linux: Use gio trash or move to ~/.local/share/Trash
    QProcess process;
    process.start("gio", QStringList() << "trash" << file_path);
    process.waitForFinished(5000);
    
    if (process.exitCode() == 0) {
        // gio trash doesn't tell us the exact trash path, but we can guess
        QString trash_dir_path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Trash/files";
        trash_path = trash_dir_path + "/" + QFileInfo(file_path).fileName();
        return true;
    }
    
    // Fallback: Move to XDG trash directory
    QString trash_dir_path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Trash/files";
    QDir trash_dir(trash_dir_path);
    if (!trash_dir.exists()) {
        trash_dir.mkpath(".");
    }
    
    QString dest = trash_dir.absoluteFilePath(QFileInfo(file_path).fileName());
    QString base = QFileInfo(file_path).completeBaseName();
    QString ext = QFileInfo(file_path).suffix();
    int counter = 1;
    const int max_attempts = 10000;
    while (QFile::exists(dest) && counter <= max_attempts) {
        QString candidate_name;
        if (ext.isEmpty()) {
            candidate_name = QString("%1_%2").arg(base, QString::number(counter));
        } else {
            candidate_name = QString("%1_%2.%3").arg(base, QString::number(counter), ext);
        }
        dest = trash_dir.absoluteFilePath(candidate_name);
        counter++;
    }
    
    if (counter > max_attempts) {
        return false;  // Failed to generate unique name
    }
    
    if (QFile::rename(file_path, dest)) {
        trash_path = dest;
        return true;
    }
    return false;
#else
    // Unsupported platform - just delete
    return QFile::remove(file_path);
#endif
}

bool FileTinderExecutor::undo_action(const ExecutionLogEntry& entry) {
    if (!entry.success) return false;
    
    if (entry.action == "move") {
        // Move file back to original location
        if (entry.dest_path.isEmpty() || entry.source_path.isEmpty()) return false;
        if (!QFile::exists(entry.dest_path)) return false;
        
        // Ensure the original directory exists
        QDir().mkpath(QFileInfo(entry.source_path).absolutePath());
        
        if (QFile::rename(entry.dest_path, entry.source_path)) {
            return true;
        }
        // Fallback: copy + delete
        if (QFile::copy(entry.dest_path, entry.source_path)) {
            QFile::remove(entry.dest_path);
            return true;
        }
        return false;
    }
    
    if (entry.action == "delete") {
        // Restore from trash if we have a trash path
        if (entry.dest_path.isEmpty() || entry.source_path.isEmpty()) return false;
        if (!QFile::exists(entry.dest_path)) return false;
        
        // Ensure the original directory exists
        QDir().mkpath(QFileInfo(entry.source_path).absolutePath());
        
        if (QFile::rename(entry.dest_path, entry.source_path)) {
            return true;
        }
        // Fallback: copy + delete
        if (QFile::copy(entry.dest_path, entry.source_path)) {
            QFile::remove(entry.dest_path);
            return true;
        }
        return false;
    }
    
    if (entry.action == "folder_create") {
        // Remove empty folder (only if it's now empty)
        QDir dir(entry.source_path);
        if (dir.exists() && dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
            return dir.rmdir(".");
        }
        return false;
    }
    
    return false;
}
